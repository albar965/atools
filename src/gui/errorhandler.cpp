/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "gui/errorhandler.h"

#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>
#include <QFileDevice>

namespace atools {

namespace gui {

void ErrorHandler::handleException(const std::exception& e, const QString& message)
{
  qCritical() << "Caught exception:" << e.what();

  QMessageBox::critical(parent, QApplication::applicationName(),
                        tr("%1\nCaught exception\n\n%2").arg(message).arg(e.what()),
                        QMessageBox::Close, QMessageBox::NoButton);
}

void ErrorHandler::handleUnknownException(const QString& message)
{
  qCritical() << "Caught unknown exception";

  QMessageBox::critical(parent, QApplication::applicationName(),
                        tr("%1\nCaught unknown exception").arg(message),
                        QMessageBox::Close, QMessageBox::NoButton);

}

void ErrorHandler::handleSqlError(const QSqlError& error, const QString& message)
{
  qCritical() << "Sql error occured:" << error.text();

  QMessageBox::critical(parent, QApplication::applicationName(),
                        tr("%1\nSql error occured\n\"%2\"").arg(message).arg(error.text()),
                        QMessageBox::Close, QMessageBox::NoButton);
}

void ErrorHandler::handleIOError(const QFileDevice& device, const QString& message)
{
  qCritical().nospace() << "IO error occured: " << device.errorString() << " (" << device.error() << ") "
                        << " file: " << device.fileName();

  QMessageBox::critical(parent, QApplication::applicationName(),
                        tr("%1\nIO error occured\nFile: \"%2\"\n\"%3\" (%4)").arg(message).
                        arg(device.fileName()).arg(device.errorString()).arg(device.error()),
                        QMessageBox::Close, QMessageBox::NoButton);
}

} // namespace gui
} // namespace atools
