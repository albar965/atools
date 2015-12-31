/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef LOGGINGDEFS_H
#define LOGGINGDEFS_H

#include <QDebug>
#include <QtGlobal>

/* Include this file instead QDebug to avoid Qt version dependent
 * incompatibilities.
 * For older Qt versions the info channel will be directed into the
 * debug channel.
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qInfo qDebug
#define QtInfoMsg QtDebugMsg
#endif

#endif // LOGGINGDEFS_H
