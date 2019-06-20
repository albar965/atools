/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_GUI_CONSOLEAPPLICATION_H
#define ATOOLS_GUI_CONSOLEAPPLICATION_H

#include <QCoreApplication>

/* Use this macro to report an fatal error after an exception including location information */
#define ATOOLS_HANDLE_CONSOLE_EXCEPTION(e) (atools::gui::ConsoleApplication::handleException(__FILE__, __LINE__, e))

/* Use this macro to report an fatal error after an unknownexception including location information */
#define ATOOLS_HANDLE_UNKNOWN_CONSOLE_EXCEPTION (atools::gui::ConsoleApplication::handleException(__FILE__, __LINE__))

namespace atools {
namespace gui {

/*
 * Adds an exception handler for the event queue and provides methods to handle fatal errors.
 * Catches any exceptions that was thrown in an event handler and shows an error message on the console.
 */
class ConsoleApplication :
  public QCoreApplication
{
  Q_OBJECT

public:
  ConsoleApplication(int& argc, char **argv, int = ApplicationFlags);
  virtual ~ConsoleApplication() override;

#if defined(Q_CC_MSVC)
  // MSVC cannot deal with newer C++ features
  static void handleException(const char *file, int line, const std::exception& e);
  static void handleException(const char *file, int line);

#else
  /*
   * Shows an error dialog with the exception message and after that exits the application with code 1.
   */
  [[noreturn]] static void handleException(const char *file, int line, const std::exception& e);

  /*
   * Shows an error dialog and after that exits the application with code 1.
   */
  [[noreturn]] static void handleException(const char *file, int line);

#endif

private:
  virtual bool notify(QObject *receiver, QEvent *event) override;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_CONSOLEAPPLICATION_H
