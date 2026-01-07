/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "atools.h"
#include "fs/scenery/manifestjson.h"
#include "fs/scenery/layoutjson.h"
#include "fs/util/fsutil.h"

#include <QDir>
#include <QTextStream>
#include <QStringBuilder>
#include <QDebug>

namespace atools {
namespace fs {
namespace scenery {

AircraftIndex::AircraftIndex(bool verboseParm) :
  verbose(verboseParm)
{
}

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
      const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
      for(const QFileInfo& addonDir : entries)
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
            for(QString layoutPath : layout.getAircraftCfgPaths())
            {
              // This is the hashmap key returned by SimConnect_RequestSystemState(EVENT_AIRCRAFT_LOADED, ...)
              // SimObjects/Airplanes/Asobo_208B_GRAND_CARAVAN_EX/aircraft.cfg
              QString cfgPathKey = layoutPath.replace('\\', '/').toLower(); // Clean path needs an existing path
              QFileInfo fullCfgPathValue(addonDir.filePath() + QDir::separator() + layoutPath);

              if(fullCfgPathValue.exists() && fullCfgPathValue.isFile())
                aircraftShortToFullPathMap.insert(cfgPathKey.toLower(), atools::cleanPath(fullCfgPathValue.canonicalFilePath()));
            }
          }
        }
      }
    }
    qDebug() << Q_FUNC_INFO << "loading done.";

    if(verbose)
      qDebug() << Q_FUNC_INFO << "aircraftShortToFullPathMap" << aircraftShortToFullPathMap;
  }
}

const QString& AircraftIndex::getIcaoTypeDesignator(const QString& aircraftCfgFilepath)
{
  return fetchProperties(aircraftCfgFilepath).icaoTypeDesignator;
}

const QString& AircraftIndex::getCategory(const QString& aircraftCfgFilepath)
{
  return fetchProperties(aircraftCfgFilepath).category;
}

const AircraftIndex::AircraftProperties& AircraftIndex::fetchProperties(const QString& aircraftCfgFilepath)
{
  QString aircraftCfgKey = aircraftCfgFilepath;
  aircraftCfgKey = aircraftCfgKey.replace('\\', '/').toLower(); // Clean path needs an existing path

  auto it = shortPathToPropertiesMap.constFind(aircraftCfgKey);

  if(it != shortPathToPropertiesMap.constEnd())
    // Already in index - either empty (indicator for nothing found) or not empty (found)
    return it.value();
  else
  {
    if(verbose)
      qDebug() << Q_FUNC_INFO << "aircraftCfgKey" << aircraftCfgKey;

    // Nothing in index yet - read aircraft.cfg file
    AircraftProperties properties;

    // Get full filename for key
    QString aircraftCfgFullPath = aircraftShortToFullPathMap.value(aircraftCfgKey);
    if(!aircraftCfgFullPath.isEmpty() && QFile::exists(aircraftCfgFullPath))
    {
      // Read type designator from aircraft.cfg file
      QFile file(aircraftCfgFullPath);
      if(file.open(QIODevice::ReadOnly))
      {
        QTextStream stream(&file);
        bool generalSection = false;
        QString icaoTypeDesignator, icaoModel;

        while(!stream.atEnd())
        {
          QString line = stream.readLine().trimmed();

          if(line.contains("[General]", Qt::CaseInsensitive))
          {
            generalSection = true;
            continue;
          }

          if(generalSection)
          {
            if(line.startsWith("icao_type_designator", Qt::CaseInsensitive)) // icao_type_designator = "A20N"
              icaoTypeDesignator = line.section('=', 1).remove('"').trimmed();

            if(line.startsWith("icao_model", Qt::CaseInsensitive)) // icao_model = "TBM-930"
              icaoModel = line.section('=', 1).remove('"').trimmed();

            if(line.startsWith("Category", Qt::CaseInsensitive)) // Category = "Helicopter"
              properties.category = line.section('=', 1).remove('"').trimmed();

            // Either all fields populated or next section after [General] - break out
            if(line.startsWith('[') ||
               (!icaoTypeDesignator.isEmpty() && !icaoModel.isEmpty() && !properties.category.isEmpty()))
              break;
          }
        }

        file.close();

        if(!atools::fs::util::isAircraftTypeDesignatorValid(icaoTypeDesignator) &&
           atools::fs::util::isAircraftTypeDesignatorValid(icaoModel))
          // ICAO type designator not valid but mode. Use model instead.
          properties.icaoTypeDesignator = icaoModel;
        else
          properties.icaoTypeDesignator = icaoTypeDesignator;
      }
    }

    // Log only once after loading
    qDebug() << Q_FUNC_INFO << "Loaded" << aircraftCfgFullPath << "found" << properties.icaoTypeDesignator << properties.category;

    // Add designator to index or empty value in case of missing file to avoid re-reading
    return shortPathToPropertiesMap.insert(aircraftCfgKey, properties).value();
  }
}

void AircraftIndex::clear()
{
  shortPathToPropertiesMap.clear();
  aircraftShortToFullPathMap.clear();
  loadedBasePaths.clear();
}

} // namespace scenery
} // namespace fs
} // namespace atools
