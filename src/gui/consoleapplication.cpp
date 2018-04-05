/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "gui/consoleapplication.h"

#include <cstdlib>
#include <QDebug>
#include <QMessageBox>
#include <QUrl>
#include <QThread>

namespace atools {
namespace gui {

ConsoleApplication::ConsoleApplication(int& argc, char **argv, int)
  : QCoreApplication(argc, argv)
{

}

ConsoleApplication::~ConsoleApplication()
{

}

bool ConsoleApplication::notify(QObject *receiver, QEvent *event)
{
  try
  {
    return QCoreApplication::notify(receiver, event);
  }
  catch(std::exception& e)
  {
    ATOOLS_HANDLE_CONSOLE_EXCEPTION(e);
  }
  catch(...)
  {
    ATOOLS_HANDLE_UNKNOWN_CONSOLE_EXCEPTION;
  }
}

void ConsoleApplication::handleException(const char *file, int line, const std::exception& e)
{
  qCritical() << "Caught exception in file" << file << "line" << line << "what" << e.what();

  std::exit(1);
}

void ConsoleApplication::handleException(const char *file, int line)
{
  qCritical() << "Caught unknown exception in file" << file << "line" << line;

  std::exit(1);
}

} // namespace gui
} // namespace atools
