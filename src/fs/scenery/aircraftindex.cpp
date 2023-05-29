/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "fs/scenery/aircraftindex.h"
#include "fs/scenery/manifestjson.h"
#include "fs/scenery/layoutjson.h"

#include <QDir>
#include <QTextStream>
#include <QStringBuilder>
#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

void AircraftIndex::loadIndex(const QStringList& basePaths)
{
  if(loadedBasePaths != basePaths || aircraftShortToFullPathMap.isEmpty())
  {
    clear();
    loadedBasePaths = basePaths;

    qDebug() << Q_FUNC_INFO << "Loading from" << basePaths << "...";

    for(const QString& path : basePaths)
    {
      // dir = .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore
      QDir dir(path);
      for(const QFileInfo& addonDir : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
      {
        // addonDir = .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore/asobo-aircraft-208b-grand-caravan-ex

        // Read manifest and check for aircraft
        ManifestJson manifest;
        manifest.read(addonDir.filePath() + QDir::separator() + "manifest.json");
        if(manifest.isValid() && manifest.isAircraft())
        {
          // Find aircraft.cfg relative location in manifest
          LayoutJson layout;
          layout.read(addonDir.filePath() + QDir::separator() + "layout.json");
          if(layout.isValid())
          {
            // There may be more than one aircraft.cfg, e.g. for wheeled and floats
            for(const QString& layoutPath : layout.getAircraftCfgPaths())
            {
              // This is the hashmap key returned by SimConnect_RequestSystemState(EVENT_AIRCRAFT_LOADED, ...)
              // SimObjects/Airplanes/Asobo_208B_GRAND_CARAVAN_EX/aircraft.cfg
              QString cfgPathKey = QDir::cleanPath(layoutPath);
              QFileInfo fullCfgPathValue(addonDir.filePath() + QDir::separator() + cfgPathKey);

              if(fullCfgPathValue.exists() && fullCfgPathValue.isFile())
                aircraftShortToFullPathMap.insert(cfgPathKey.toLower(), QDir::cleanPath(fullCfgPathValue.canonicalFilePath()));
            }
          }
        }
      }
    }
    qDebug() << Q_FUNC_INFO << "loading done.";
  }
}

QString AircraftIndex::getIcaoTypeDesignator(const QString& aircraftCfgFilepath)
{
  QString aircraftCfgKey = QDir::cleanPath(aircraftCfgFilepath).toLower();
  QString typeDesignator = shortPathToTypeDesMap.value(aircraftCfgKey);

  if(typeDesignator.isEmpty())
  {
    // Nothing in index yet - read aircraft.cfg file

    // Get full filename for key
    QString aircraftCfgPath = aircraftShortToFullPathMap.value(aircraftCfgKey);

    // Read type designator from aircraft.cfg file
    QFile file(aircraftCfgPath);
    if(file.open(QIODevice::ReadOnly))
    {
      QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
      stream.setCodec("UTF-8");
#endif
      stream.setAutoDetectUnicode(true);

      while(!stream.atEnd())
      {
        QString line = stream.readLine().trimmed();

        if(line.startsWith("icao_type_designator", Qt::CaseInsensitive)) // icao_type_designator = "A20N"
        {
          typeDesignator = line.section('=', 1).remove('"').trimmed();
          break;
        }
      }
      file.close();
    }

    // Add designator to index or empty value in case of missing file to avoid re-reading
    shortPathToTypeDesMap.insert(aircraftCfgKey, typeDesignator);
  }

  return typeDesignator;
}

void AircraftIndex::clear()
{
  shortPathToTypeDesMap.clear();
  aircraftShortToFullPathMap.clear();
  loadedBasePaths.clear();
}

} // namespace scenery
} // namespace fs
} // namespace atools
