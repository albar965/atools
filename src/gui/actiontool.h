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

#ifndef ATOOLS_GUI_ACTIONTOOL_H
#define ATOOLS_GUI_ACTIONTOOL_H

#include <QCoreApplication>
#include <QAction>

namespace atools {
namespace gui {

class ActionIconSaver;
class ActionTextSaver;
class ActionStateSaver;

/*
 * Combines the functionality of ActionTextSaver, ActionStateSaver and ActionIconSaver
 * and provides additional methods to modify actions for context menus.
 */
class ActionTool
{
  Q_DECLARE_TR_FUNCTIONS(ActionTool)

public:
  /* All texts and disabled/enabled state is restored on destruction of this object */
  explicit ActionTool(QList<QAction *> actions);
  ~ActionTool();

  /* Inititialize translations */
  static void initTranslateableTexts();

  /* Do not allow copying */
  ActionTool(const ActionTool& other) = delete;
  ActionTool& operator=(const ActionTool& other) = delete;

  /* Sets the %1 placeholder with arg. An empty placeholder is used if the
   * action is disabled and the suffix is attached for disabled. */
  static void setText(QAction *action, const QString& arg = QString(), const QString& suffix = QString())
  {
    setText(action, action->isEnabled(), arg, suffix);
  }

  /* Sets the %1 placeholder with arg. An empty placeholder is used if enabled is false
   *  and the suffix is attached for disabled. Suffix is added before ellipsis " ...". */
  static void setText(QAction *action, bool enabled, const QString& arg = QString(), const QString& suffix = QString());

  /* Look for placeholders %1 in all texts and replace these with objectText if
   * the action is enabled. Uses empty placeholder for disabled actions */
  void finishTexts(const QString& objectText);

  /* Enable or disable all actions */
  void enableAll();
  void disableAll();

  /* Add text before " ..."  */
  static QString addTextBeforeEllipsis(const QString& str, const QString& suffix);

  /* Add " ..." if string does not contain it */
  static QString addEllipsis(const QString& str)
  {
    return hasEllipsis(str) ? str : str % ellipsisStr;
  }

  /* Remove " ..." */
  static QString removeEllipsis(const QString& str)
  {
    return hasEllipsis(str) ? str.chopped(ellipsisStr.size()) : str;
  }

  /* True if string ends with " ..." */
  static bool hasEllipsis(const QString& str)
  {
    return str.endsWith(ellipsisStr);
  }

  /* Usually " ..." depending on translation. Call initTranslateableTexts() before. */
  static const QString& getEllipsisStr()
  {
    return ellipsisStr;
  }

private:
  atools::gui::ActionTextSaver *textSaver;
  atools::gui::ActionStateSaver *stateSaver;
  atools::gui::ActionIconSaver *iconSaver;

  static QString ellipsisStr;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_ACTIONTOOL_H
