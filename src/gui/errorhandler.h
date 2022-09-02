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

#ifndef ATOOLS_GUI_ERRORHANDLER_H
#define ATOOLS_GUI_ERRORHANDLER_H

#include <QString>
#include <QCoreApplication>

namespace std {
class exception;
} // namespace std

class QWidget;
class QSqlError;
class QFileDevice;

namespace atools {
namespace gui {

/*
 * Provides utility dialog methods that display errors from the SQL, I/O or
 * standard exceptions and derived. None of these exits the application.
 */
class ErrorHandler
{
  Q_DECLARE_TR_FUNCTIONS(ErrorHandler)

public:
  /*
   * @param parentWidget widget for all dialogs
   */
  explicit ErrorHandler(QWidget *parentWidget)
    : parent(parentWidget)
  {
  }

  /*
   * Display an error dialog for std::exception and derived.
   * The e.what() method is used to create a message together with the given
   * message.
   * @param e Exception used to extract a message
   * @param message Shown together with the exception message
   */
  void handleException(const std::exception& e, const QString& message = QString());

  /*
   * Display an error dialog for an unknown exception.
   * @param message Message to be shown.
   */
  void handleUnknownException(const QString& message = QString());

  /*
   * Display an error dialog for QSqlErrors.
   * A message from the SQL error is composed together with the given
   * message.
   * @param error SQL eror used to extract a message
   * @param message Show together with the SQL error
   */
  void handleSqlError(const QSqlError& error, const QString& message = QString());

  /*
   * Display an error dialog for file I/O errors.
   * A message from the file device is composed together with the given
   * message.
   * @param device Device used to extract a message
   * @param message Show together with the file device error
   */
  void handleIOError(const QFileDevice& device, const QString& message = QString());

private:
  QWidget *parent = nullptr;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_ERRORHANDLER_H
