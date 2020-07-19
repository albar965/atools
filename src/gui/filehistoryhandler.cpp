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

#include "gui/filehistoryhandler.h"
#include "settings/settings.h"

#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QDir>

namespace atools {
namespace gui {

FileHistoryHandler::FileHistoryHandler(QObject *parent, const QString& settingsName,
                                       QMenu *recentMenuList, QAction *clearMenuAction) :
  QObject(parent), recentMenu(recentMenuList), clearAction(clearMenuAction), settings(settingsName)
{
  connect(recentMenu, &QMenu::triggered, this, &FileHistoryHandler::itemTriggered);
  recentMenuList->setToolTipsVisible(true);
}

FileHistoryHandler::~FileHistoryHandler()
{

}

void FileHistoryHandler::saveState()
{
  atools::settings::Settings::instance().setValue(settings, filePaths);
}

void FileHistoryHandler::restoreState()
{
  filePaths = atools::settings::Settings::instance().valueStrList(settings);

  // Convert all loaded paths to native
  for(int i = 0; i < filePaths.size(); i++)
    filePaths[i] = QDir::toNativeSeparators(filePaths.at(i));

  updateMenu();
}

void FileHistoryHandler::addFile(const QString& filename)
{
  if(!filename.isEmpty())
  {
    QString nativeFilename = QDir::toNativeSeparators(filename);

    // Remove file from list
    filePaths.removeAll(nativeFilename);

    // and prepend
    filePaths.prepend(nativeFilename);

    // Remove all above max entries
    while(filePaths.size() > maxEntries)
      filePaths.removeLast();

    // Update menu actions
    updateMenu();
  }
}

void FileHistoryHandler::removeFile(const QString& filename)
{
  filePaths.removeAll(QDir::toNativeSeparators(filename));
  updateMenu();
}

void FileHistoryHandler::enableAll()
{
  for(QAction *a : recentMenu->actions())
    a->setEnabled(true);
}

void FileHistoryHandler::disableAll()
{
  for(QAction *a : recentMenu->actions())
  {
    if(a != clearAction)
      a->setEnabled(false);
  }
}

void FileHistoryHandler::itemTriggered(QAction *action)
{
  if(action == clearAction)
    clearMenu();
  else
  {
    // Move file up in the list
    QString fname = action->data().toString();
    filePaths.removeAll(fname);
    filePaths.prepend(fname);
    updateMenu();

    emit fileSelected(fname);
  }
}

void FileHistoryHandler::updateMenu()
{
  recentMenu->clear();

  int i = 1;
  for(const QString& filepath : filePaths)
  {
    QString fname = QFileInfo(filepath).fileName();
    // Add number for selection
    if(i < 10)
      fname = "&" + QString::number(i) + " " + fname;
    else if(i == 10)
      fname = "&0 " + fname;

#ifdef Q_OS_UNIX
    // Single underscores are removed by the menu action
    fname.replace("_", " ");
#endif

    QAction *fileAction = recentMenu->addAction(fname);
    fileAction->setToolTip(filepath);
    fileAction->setStatusTip(filepath);
    fileAction->setData(filepath);
    i++;
  }

  recentMenu->addSeparator();
  clearAction->setEnabled(!filePaths.isEmpty());
  recentMenu->addAction(clearAction);
}

void FileHistoryHandler::clearMenu()
{
  filePaths.clear();
  updateMenu();
}

} // namespace gui
} // namespace atools
