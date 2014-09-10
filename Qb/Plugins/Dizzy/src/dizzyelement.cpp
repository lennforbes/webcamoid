/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "dizzyelement.h"

DizzyElement::DizzyElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetPhaseIncrement();
    this->resetZoomRate();
}

float DizzyElement::phaseIncrement() const
{
    return this->m_phaseIncrement;
}

float DizzyElement::zoomRate() const
{
    return this->m_zoomRate;
}

void DizzyElement::setParams(int *dx, int *dy,
                             int *sx, int *sy,
                             int width, int height,
                             float phase, float zoomRate)
{
    float dizz = 10 * sin(phase)
                 + 5 * sin(1.9 * phase + 5);

    int x = width >> 1;
    int y = height >> 1;
    float t = zoomRate * (x * x + y * y);

    float vx;
    float vy;

    if (width > height) {
        if (dizz >= 0) {
            if(dizz > x)
                dizz = x;

            vx = (x * (x - dizz) + y * y) / t;
        }
        else {
            if(dizz < -x)
                dizz = -x;

            vx = (x * (x + dizz) + y * y) / t;
        }

        vy = dizz * y / t;
    }
    else {
        if (dizz >= 0) {
            if (dizz > y)
                dizz = y;

            vx = (x * x + y * (y - dizz)) / t;
        }
        else {
            if (dizz < -y)
                dizz = -y;

            vx = (x * x + y * (y + dizz)) / t;
        }

        vy = dizz * x / t;
    }

    *dx = 65536 * vx;
    *dy = 65536 * vy;
    *sx = 65536 * (-vx * x + vy * y + x + 2 * cos(5 * phase));
    *sy = 65536 * (-vx * y - vy * x + y + 2 * sin(6 * phase));
}

void DizzyElement::setPhaseIncrement(float phaseIncrement)
{
    this->m_phaseIncrement = phaseIncrement;
}

void DizzyElement::setZoomRate(float zoomRate)
{
    this->m_zoomRate = zoomRate;
}

void DizzyElement::resetPhaseIncrement()
{
    this->setPhaseIncrement(0.02);
}

void DizzyElement::resetZoomRate()
{
    this->setZoomRate(1.01);
}

void DizzyElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() == "video/x-raw")
        this->m_convert->iStream(packet);
}

void DizzyElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void DizzyElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage src = QImage((const uchar *) packet.buffer().data(),
                        width,
                        height,
                        QImage::Format_ARGB32);

    int videoArea = width * height;

    QImage oFrame(src.size(), src.format());

    QRgb *srcBits = (QRgb *) src.bits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    if (packet.caps() != this->m_caps) {
        this->m_prevFrame = QImage();
        this->m_phase = 0;

        this->m_caps = packet.caps();
    }

    if (this->m_prevFrame.isNull())
        oFrame = src;
    else {
        int dx;
        int dy;
        int sx;
        int sy;

        this->setParams(&dx, &dy, &sx, &sy,
                        width, height,
                        this->m_phase, this->m_zoomRate);

        this->m_phase += this->m_phaseIncrement;

        if (this->m_phase > 5700000)
            this->m_phase = 0;

        QRgb *prevFrameBits = (QRgb *) this->m_prevFrame.bits();

        for (int y = 0, i = 0; y < height; y++) {
            int ox = sx;
            int oy = sy;

            for (int x = 0; x < width; x++, i++) {
                int j = (oy >> 16) * width + (ox >> 16);

                if (j < 0)
                    j = 0;

                if (j >= videoArea)
                    j = videoArea;

                QRgb v = prevFrameBits[j] & 0xfcfcff;
                v = 3 * v + (srcBits[i] & 0xfcfcff);
                destBits[i] = (v >> 2) | 0xff000000;
                ox += dx;
                oy += dy;
            }

            sx -= dy;
            sy += dx;
        }
    }

    this->m_prevFrame = oFrame.copy();

    QbBufferPtr oBuffer(new char[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", "bgra");
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}