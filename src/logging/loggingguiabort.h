/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_LOGGINGGUIABORT_H
#define ATOOLS_LOGGINGGUIABORT_H

#include <QObject>

class QWidget;

namespace atools {
namespace logging {

/* Separate handler to show a dialog if an fatal logging event or assert appears
 * Moved GUI elements out to avoid linking to qwidgets */
class LoggingGuiAbortHandler
  : public QObject
{
  Q_OBJECT

public:
  LoggingGuiAbortHandler();
  virtual ~LoggingGuiAbortHandler() override;

  static void setGuiAbortFunction(QWidget *parent);
  static void resetGuiAbortFunction();

private:
  /* Connected to signal guiAbortSignal to allow GUI handling */
  void guiAbortFunction(const QString& msg);

  static atools::logging::LoggingGuiAbortHandler *instance;
};

} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGINGGUIABORT_H
