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

#ifndef ATOOLS_GUI_FILEHISTORYHANDLER_H
#define ATOOLS_GUI_FILEHISTORYHANDLER_H

#include <QObject>

class QMenu;
class QAction;

namespace atools {
namespace gui {

/*
 * General handler for a recent files menu. Takes care of the actions, saving and loading state,
 * resorting of the list on selection and the clear list action item.
 */
class FileHistoryHandler :
  public QObject
{
  Q_OBJECT

public:
  /*
   *
   * @param parent Parent menu
   * @param settingsNamePrefix prefix for saving the entries into a settings file
   * @param recentMenuList Menu that will get the file list appended. Should contain a
   * separator and a clear menu action initially.
   * @param clearMenuAction The clear menu action. Will remove all recent entries.
   */
  FileHistoryHandler(QObject *parent, const QString& settingsNamePrefix, QMenu *recentMenuList,
                     QAction *clearMenuAction);
  virtual ~FileHistoryHandler();

  /* Save state and all entries to settings */
  void saveState();

  /* Restore state and all entries from settings */
  void restoreState();

  /* Add a file that will be prependend to the list of recent files */
  void addFile(const QString& filename);

  /* Remove a file from the list of recent files because it does not exist anymore. */
  void removeFile(const QString& filename);

  int getMaxEntries() const
  {
    return maxEntries;
  }

  void setMaxEntries(int value)
  {
    maxEntries = value;
  }

  /* Enable or disable all file menu entries - not the clear action */
  void enableAll();
  void disableAll();

signals:
  /* Emitted when the user selects a recent file action */
  void fileSelected(const QString& filename);

private:
  void clearMenu();
  void itemTriggered(QAction *action);
  void updateMenu();

  QMenu *recentMenu;
  QAction *clearAction;

  /* Native full filepaths */
  QStringList filePaths;

  /* Name prefix for settings file */
  QString settings;

  int maxEntries = 20;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_FILEHISTORYHANDLER_H
