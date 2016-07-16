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

#ifndef ATOOLS_WIDGETSTATESAVER_H
#define ATOOLS_WIDGETSTATESAVER_H

#include <QObject>

namespace atools {
namespace settings {
class Settings;
}

namespace gui {

/* Allows to save the state of many differnet widgets and actions to the global settings instance.
 * Key names are automatically genreated by object names. */
class WidgetState
{

public:
  WidgetState(const QString& settingsKeyPrefix, bool saveVisibility = true, bool blockSignals = false);

  void save(const QList<QObject *>& widgets) const;
  void restore(const QList<QObject *>& widgets) const;

  void save(const QObject *widget) const;
  void restore(QObject *widget) const;

  void syncSettings();

  bool getSaveVisibility() const
  {
    return visibility;
  }

  void setSaveVisibility(bool value)
  {
    visibility = value;
  }

  bool getBlockSignals() const
  {
    return block;
  }

  void setBlockSignals(bool value)
  {
    block = value;
  }

  void setMainWindowsRestoreOptions(bool position, bool size, bool state);

private:
  QString keyPrefix;
  bool visibility = true, block = false;

  void saveWidget(atools::settings::Settings& settings, const QObject *w, const QVariant& value) const;
  QVariant loadWidget(atools::settings::Settings& settings, QObject *w) const;

  void saveWidgetVisible(atools::settings::Settings& settings, const QWidget *w) const;
  void loadWidgetVisible(atools::settings::Settings& settings, QWidget *w) const;

  bool positionRestoreMainWindow = true, sizeRestoreMainWindow = true, stateRestoreMainWindow = true;
};

} // namespace gui
} // namespace atools

#endif // ATOOLS_WIDGETSTATESAVER_H
