/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_AIRCRAFTINDEX_H
#define ATOOLS_AIRCRAFTINDEX_H

#include <QHash>
#include <QStringList>

namespace atools {
namespace fs {
namespace scenery {

struct AircraftProperties;

/* .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Official/OneStore/asobo-aircraft-208b-grand-caravan-ex/
 * .../Microsoft.FlightSimulator_8wekyb3d8bbwe/LocalCache/Packages/Community
 * "SimObjects/Airplanes/Asobo_B787_10/aircraft.cfg"
 *
 * [GENERAL]
 * atc_type ="Bell Helicopter"
 * atc_model ="Model 407"
 * Category = "Helicopter"
 *
 * [GENERAL]
 * atc_type = "TT:ATCCOM.ATC_NAME_BEECHCRAFT.0.text"
 * atc_model = "TT:ATCCOM.AC_MODEL_BE58.0.text"
 * Category = "airplane"
 * icao_type_designator = "BE58"
 * icao_manufacturer = "HAWKER BEECHCRAFT"
 * icao_model = "58 Baron"
 * icao_engine_type = "Piston"
 * icao_engine_count = 2
 * icao_WTC = "L"
 *
 */
class AircraftIndex
{
public:
  explicit AircraftIndex(bool verboseParm);

  /* Load manifest and layout JSON and look for type AIRCRAFT in manifest and aircraft.cfg location in layout.
   * Store aircraft.cfg location in index but do not read aircraft.cfg.
   * layout.json "path": "SimObjects/Airplanes/Asobo_B787_10/aircraft.cfg",
   * manifest.json   "content_type": "AIRCRAFT",
   *
   * Only for user aircraft.
   */
  void loadIndex(const QStringList& paths);

  /* "SimObjects/Airplanes/Asobo_B787_10/aircraft.cfg". Read aircraft.cfg and look for "icao_type_designator".
   * icao_type_designator = "A20N". Best guess from icao_type_designator and icao_model. */
  const QString& getIcaoTypeDesignator(const QString& aircraftCfgFilepath);

  /* Category = "Helicopter" */
  const QString& getCategory(const QString& aircraftCfgFilepath);

  void clear();

  bool isEmpty() const
  {
    return aircraftShortToFullPathMap.isEmpty();
  }

  int size() const
  {
    return aircraftShortToFullPathMap.size();
  }

private:
  struct AircraftProperties
  {
    QString category, icaoTypeDesignator;
  };

  const AircraftProperties& fetchProperties(const QString& aircraftCfgFilepath);

  /* Maps short path "SimObjects/Airplanes/Asobo_B787_10/aircraft.cfg" to aircraft type "B787" */
  QHash<QString, AircraftProperties> shortPathToPropertiesMap;

  /* Maps short path to full canonical path */
  QHash<QString, QString> aircraftShortToFullPathMap;

  /* Used by load index to avoid unneeded reload */
  QStringList loadedBasePaths;

  bool verbose = false;

};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRCRAFTINDEX_H
