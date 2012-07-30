#!/usr/bin/python2 -B
# -*- coding: utf-8 -*-
#
# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

import os

from PyQt4 import QtCore
from PyKDE4 import plasmascript


class Translator(QtCore.QTranslator):
    def __init__(self, context='', parent=None):
        QtCore.QTranslator.__init__(self, parent)

        locale = QtCore.QLocale.system().name()

        if isinstance(parent, plasmascript.Applet):
            i18nDir = os.path.join(str(parent.package().path()),
                                   'contents',
                                   'ts')
        else:
            i18nDir = self.resolvePath('../ts')

        self.load(locale + '.qm', i18nDir)
        self.context = context

    def resolvePath(self, relpath=''):
        return os.path.normpath(os.path.join(os.path.\
                                dirname(os.path.realpath(__file__)), relpath))

    def tr(self, sourceText):
        res = self.translate(self.context, sourceText)

        return QtCore.QString(sourceText) if len(res) == 0 else res
