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

#ifndef ATOOLS_GUI_PALETTESETTINGS_H
#define ATOOLS_GUI_PALETTESETTINGS_H

#include <QString>

class QSettings;
class QPalette;

namespace atools {
namespace gui {

/* Provides functions to load and save a palette to a config file in "ini" format */
class PaletteSettings
{
public:
  PaletteSettings(const QString& settingsFile, const QString& groupName, const QString& keyPrefix = QString());
  virtual ~PaletteSettings();

  void savePalette(const QPalette& palette);

  void loadPalette(QPalette& palette);

  void syncPalette(QPalette& palette);

private:
  QSettings *settings = nullptr;
  QString group, prefix;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_PALETTESETTINGS_H
