/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/xp/xpairportwriter.h"

#include "sql/sqlutil.h"
#include "geo/calculations.h"
#include "fs/util/fsutil.h"
#include "fs/progresshandler.h"
#include "fs/xp/xpairportindex.h"

#include <QDebug>
#include <QRegularExpression>

using atools::sql::SqlQuery;
using atools::sql::SqlUtil;
using atools::sql::SqlRecord;
using atools::geo::Pos;
using atools::geo::Rect;
using atools::geo::meterToFeet;

namespace atools {
namespace fs {
namespace xp {

namespace ap {
enum AirportFieldIndex
{
  ROWCODE = 0,
  ELEVATION = 1,
  // 2x unused
  ICAO = 4,
  NAME = 5
};

}

namespace vp {
enum ViewpointFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  HEIGHT = 3,
};

}

namespace s {
enum StartFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  HEADING = 3,
  NAME = 4
};

}

namespace v {
enum VasiFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  TYPE = 3,
  ORIENT = 4,
  ANGLE = 5,
  RUNWAY = 6,
  DESCRIPTION = 7
};

}

namespace sl {
enum StartupLocationFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  HEADING = 3,
  TYPE = 4, // gate, hangar, misc or tie-down
  AIRPLANE_TYPE = 5, // Pipe-separated list (|). heavy, jets, turboprops, props and helos (or just all for all types)
  NAME = 6,
};

}

namespace m {
enum MetaFieldIndex
{
  ROWCODE = 0,
  KEY = 1,
  VALUE = 2
};

}
namespace com {
enum ComFieldIndex
{
  ROWCODE = 0,
  FREQUENCY = 1,
  NAME = 2
};

}

namespace hp {
enum HelipadFieldIndex
{
  ROWCODE = 0,
  DESIGNATOR = 1,
  LATY = 2,
  LONX = 3,
  ORIENTATION = 4,
  LENGTH = 5,
  WIDTH = 6,
  SURFACE = 7
};

}

namespace rw {
enum RunwayFieldIndex
{
  ROWCODE = 0,
  WIDTH = 1,

  // Indexes for water runways
  WATER_PRIMARY_NUMBER = 3,
  WATER_PRIMARY_LATY = 4,
  WATER_PRIMARY_LONX = 5,
  WATER_SECONDARY_NUMBER = 6,
  WATER_SECONDARY_LATY = 7,
  WATER_SECONDARY_LONX = 8,

  SURFACE = 2,
  SHOULDER_SURFACE = 3,
  SMOOTHNESS = 4,
  CENTER_LIGHTS = 5,
  EDGE_LIGHTS = 6,
  DISTANCE_REMAINING_SIGNS = 7,
  PRIMARY_NUMBER = 8,
  PRIMARY_LATY = 9,
  PRIMARY_LONX = 10,
  PRIMARY_DISPLACED_THRESHOLD = 11,
  PRIMARY_OVERRUN_BLASTPAD = 12,
  PRIMARY_MARKINGS = 13,
  PRIMARY_ALS = 14,
  PRIMARY_TDZ_LIGHT = 15,
  PRIMARY_REIL = 16,
  SECONDARY_NUMBER = 17,
  SECONDARY_LATY = 18,
  SECONDARY_LONX = 19,
  SECONDARY_DISPLACED_THRESHOLD = 20,
  SECONDARY_OVERRUN_BLASTPAD = 21,
  SECONDARY_MARKINGS = 22,
  SECONDARY_ALS = 23,
  SECONDARY_TDZ_LIGHT = 24,
  SECONDARY_REIL = 25,
};

}

XpAirportWriter::XpAirportWriter(atools::sql::SqlDatabase& sqlDb, XpAirportIndex *xpAirportIndex,
                                 const NavDatabaseOptions& opts, ProgressHandler *progressHandler)
  : XpWriter(sqlDb, opts, progressHandler), airportIndex(xpAirportIndex)
{
  initRunwayEndRecord();
  initQueries();
}

XpAirportWriter::~XpAirportWriter()
{
  deInitQueries();
}

void XpAirportWriter::write(const QStringList& line, const XpWriterContext& context)
{
  AirportRowCode rowCode = static_cast<AirportRowCode>(line.at(ap::ROWCODE).toInt());

  switch(rowCode)
  {
    case atools::fs::xp::LAND_AIRPORT_HEADER:
    case atools::fs::xp::SEAPLANE_BASE_HEADER:
    case atools::fs::xp::HELIPORT_HEADER:
      finishAirport();
      bindAirport(line, rowCode, context);
      break;

    case atools::fs::xp::LAND_RUNWAY:
    case atools::fs::xp::WATER_RUNWAY:
      writeRunway(line, rowCode);
      break;

    case atools::fs::xp::HELIPAD:
      writeHelipad(line);
      break;

    case atools::fs::xp::PAVEMENT_HEADER:
    case atools::fs::xp::LINEAR_FEATURE_HEADER:
    case atools::fs::xp::AIRPORT_BOUNDARY_HEADER:
    case atools::fs::xp::NODE:
    case atools::fs::xp::NODE_WITH_BEZIER_CONTROL_POINT:
    case atools::fs::xp::NODE_WITH_IMPLICIT_CLOSE_OF_LOOP:
    case atools::fs::xp::NODE_WITH_BEZIER_CONTROL_POINT_CLOSE:
    case atools::fs::xp::NODE_TERMINATING_A_STRING:
    case atools::fs::xp::NODE_WITH_BEZIER_CONTROL_POINT_NO_CLOSE:
      break;

    case atools::fs::xp::AIRPORT_VIEWPOINT:
      bindViewpoint(line);
      break;

    case atools::fs::xp::AEROPLANE_STARTUP_LOCATION:
      writeStart(line);
      break;

    case atools::fs::xp::AIRPORT_LIGHT_BEACON:
    case atools::fs::xp::WINDSOCK:
    case atools::fs::xp::TAXIWAY_SIGN:
      break;

    case atools::fs::xp::LIGHTING_OBJECT:
      bindVasi(line);
      break;

    case atools::fs::xp::AIRPORT_TRAFFIC_FLOW:
    case atools::fs::xp::TRAFFIC_FLOW_WIND_RULE:
    case atools::fs::xp::TRAFFIC_FLOW_MINIMUM_CEILING_RULE:
    case atools::fs::xp::TRAFFIC_FLOW_MINIMUM_VISIBILITY_RULE:
    case atools::fs::xp::TRAFFIC_FLOW_TIME_RULE:
    case atools::fs::xp::RUNWAY_IN_USE:
    case atools::fs::xp::VFR_TRAFFIC_PATTERN:
    case atools::fs::xp::HEADER_INDICATING_THAT_TAXI_ROUTE_NETWORK_DATA_FOLLOWS:
    case atools::fs::xp::TAXI_ROUTE_NETWORK_NODE:
    case atools::fs::xp::TAXI_ROUTE_NETWORK_EDGE:
    case atools::fs::xp::TAXI_ROUTE_EDGE_ACTIVE_ZONE:
      break;

    case atools::fs::xp::AIRPORT_LOCATION:
      writeStartupLocation(line);
      break;

    case atools::fs::xp::RAMP_START_METADATA:
      break;

    case atools::fs::xp::METADATA_RECORDS:
      bindMetadata(line);
      break;

    case atools::fs::xp::TRUCK_PARKING_LOCATION:
    case atools::fs::xp::TRUCK_DESTINATION_LOCATION:
      bindFuel(line);
      break;

    case atools::fs::xp::COM_WEATHER:
    case atools::fs::xp::COM_UNICOM:
    case atools::fs::xp::COM_CLEARANCE:
    case atools::fs::xp::COM_GROUND:
    case atools::fs::xp::COM_TOWER:
    case atools::fs::xp::COM_APPROACH:
    case atools::fs::xp::COM_DEPARTURE:
      writeCom(line, rowCode);
      break;

    case atools::fs::xp::NO_ROWCODE:
      break;

  }
}

void XpAirportWriter::finish()
{
  finishAirport();
}

void XpAirportWriter::bindVasi(const QStringList& line)
{
  if(ignoringAirport)
    return;

  ApproachIndicator type = static_cast<ApproachIndicator>(line.at(v::TYPE).toInt());
  if(type == NO_APPR_INDICATOR)
    return;

  QString rwName = line.at(v::RUNWAY);
  SqlRecord *rwEnd = nullptr;
  for(SqlRecord& rec:runwayEndRecords)
  {
    if(rec.valueStr(":name") == rwName)
    {
      rwEnd = &rec;
      break;
    }
  }

  if(rwEnd != nullptr)
  {
    rwEnd->setValue(":left_vasi_type", approachIndicatorToDb(type));
    rwEnd->setValue(":left_vasi_pitch", line.at(v::ANGLE).toFloat());
    rwEnd->setValue(":right_vasi_type", "UNKN");
    rwEnd->setValue(":right_vasi_pitch", 0.f);
  }
}

void XpAirportWriter::bindViewpoint(const QStringList& line)
{
  if(ignoringAirport)
    return;

  Pos pos(line.at(vp::LONX).toFloat(), line.at(vp::LATY).toFloat());
  airportRect.extend(pos);
  insertAirportQuery->bindValue(":tower_laty", pos.getLatY());
  insertAirportQuery->bindValue(":tower_lonx", pos.getLonX());
  insertAirportQuery->bindValue(":tower_altitude", airportAltitude + line.at(vp::HEIGHT).toFloat());
  insertAirportQuery->bindValue(":has_tower_object", 1);
}

void XpAirportWriter::writeStartupLocation(const QStringList& line)
{
  if(ignoringAirport)
    return;

  numStart++;
  insertStartQuery->bindValue(":start_id", ++curStartId);
  insertStartQuery->bindValue(":airport_id", curAirportId);

  Pos pos(line.at(sl::LONX).toFloat(), line.at(sl::LATY).toFloat());
  airportRect.extend(pos);
  insertStartQuery->bindValue(":laty", pos.getLatY());
  insertStartQuery->bindValue(":lonx", pos.getLonX());
  insertStartQuery->bindValue(":altitude", airportAltitude);
  insertStartQuery->bindValue(":heading", line.at(sl::HEADING).toFloat());
  insertStartQuery->exec();
}

void XpAirportWriter::writeStart(const QStringList& line)
{
  if(ignoringAirport)
    return;

  // Obsolete type 15
  numStart++;
  insertStartQuery->bindValue(":start_id", ++curStartId);
  insertStartQuery->bindValue(":airport_id", curAirportId);

  Pos pos(line.at(s::LONX).toFloat(), line.at(s::LATY).toFloat());
  airportRect.extend(pos);
  insertStartQuery->bindValue(":laty", pos.getLatY());
  insertStartQuery->bindValue(":lonx", pos.getLonX());
  insertStartQuery->bindValue(":altitude", airportAltitude);
  insertStartQuery->bindValue(":heading", line.at(s::HEADING).toFloat());
  // insertAirportQuery->bindValue(":name", line.at(s::NAME));

  // runway_end_id
  // runway_name // helipad and runway
  // type // R, W, H
  // number // only helipad
  insertStartQuery->exec();
}

void XpAirportWriter::writeCom(const QStringList& line, AirportRowCode rowCode)
{
  if(ignoringAirport)
    return;

  numCom++;
  insertComQuery->bindValue(":com_id", ++curComId);
  insertComQuery->bindValue(":airport_id", curAirportId);

  int frequency = line.at(com::FREQUENCY).toInt() * 10;
  QString name = line.mid(com::NAME).join(" ");
  insertComQuery->bindValue(":name", name);
  insertComQuery->bindValue(":frequency", frequency);
  insertComQuery->bindValue(":type", "NONE");

  if(rowCode == atools::fs::xp::COM_WEATHER)
  {
    if(name.contains("atis", Qt::CaseInsensitive))
    {
      insertAirportQuery->bindValue(":atis_frequency", frequency);
      insertComQuery->bindValue(":type", "ATIS");
    }
    else if(name.contains("awos", Qt::CaseInsensitive))
    {
      insertAirportQuery->bindValue(":awos_frequency", frequency);
      insertComQuery->bindValue(":type", "AWOS");
    }
    else if(name.contains("asos", Qt::CaseInsensitive))
    {
      insertAirportQuery->bindValue(":asos_frequency", frequency);
      insertComQuery->bindValue(":type", "ASOS");
    }
    else
    {
      insertAirportQuery->bindValue(":atis_frequency", frequency);
      insertComQuery->bindValue(":type", "ATIS");
    }
  }
  else if(rowCode == atools::fs::xp::COM_UNICOM)
  {
    insertAirportQuery->bindValue(":unicom_frequency", frequency);
    insertComQuery->bindValue(":type", "UC");
  }
  else if(rowCode == atools::fs::xp::COM_TOWER)
  {
    insertAirportQuery->bindValue(":tower_frequency", frequency);
    insertComQuery->bindValue(":type", "T");
  }
  else if(rowCode == atools::fs::xp::COM_CLEARANCE)
    insertComQuery->bindValue(":type", "C");
  else if(rowCode == atools::fs::xp::COM_GROUND)
    insertComQuery->bindValue(":type", "G");
  else if(rowCode == atools::fs::xp::COM_APPROACH)
    insertComQuery->bindValue(":type", "A");
  else if(rowCode == atools::fs::xp::COM_DEPARTURE)
    insertComQuery->bindValue(":type", "D");

  insertComQuery->exec();
}

void XpAirportWriter::bindFuel(const QStringList& line)
{
  if(ignoringAirport)
    return;

  QString type = line.at(4);
  // Pipe separated list (“|”). Include 1 or more of the following baggage_loader,
  // baggage_train, crew_car, crew_ferrari, crew_limo, pushback, fuel_liners, fuel_jets, fuel_props, food, gpu

  if(type.contains("fuel_props"))
    insertAirportQuery->bindValue(":has_avgas", 1);

  if(type.contains("fuel_liners") || type.contains("fuel_jets"))
    insertAirportQuery->bindValue(":has_jetfuel", 1);
}

void XpAirportWriter::bindMetadata(const QStringList& line)
{
  if(ignoringAirport)
    return;

  QString key = line.at(m::KEY).toLower();
  QString value = line.mid(m::VALUE).join(" ");

  if(key == "city")
    insertAirportQuery->bindValue(":city", value);
  else if(key == "country")
    insertAirportQuery->bindValue(":country", value);
  else if(key == "datum_lat" && atools::almostNotEqual(value.toFloat(), 0.f))
    airportPos.setLatY(value.toFloat());
  else if(key == "datum_lon" && atools::almostNotEqual(value.toFloat(), 0.f))
    airportPos.setLonX(value.toFloat());

  // 1302 city Seattle
  // 1302 country United States
  // 1302 datum_lat 47.449888889
  // 1302 datum_lon -122.311777778
  // 1302 faa_code SEA
  // 1302 iata_code SEA
  // 1302 icao_code KSEA
}

void XpAirportWriter::writeHelipad(const QStringList& line)
{
  if(ignoringAirport)
    return;

  numHelipad++;
  insertHelipadQuery->bindValue(":helipad_id", ++curHelipadId);
  insertHelipadQuery->bindValue(":airport_id", curAirportId);
  // start_id
  insertHelipadQuery->bindValue(":surface", surfaceToDb(static_cast<Surface>(line.at(rw::SURFACE).toInt())));

  insertHelipadQuery->bindValue(":length", meterToFeet(line.at(hp::LENGTH).toFloat()));
  insertHelipadQuery->bindValue(":width", meterToFeet(line.at(hp::WIDTH).toFloat()));
  insertHelipadQuery->bindValue(":heading", line.at(hp::ORIENTATION).toFloat());

  insertHelipadQuery->bindValue(":type", "H");
  insertHelipadQuery->bindValue(":is_transparent", 0);
  insertHelipadQuery->bindValue(":is_closed", 0);

  insertHelipadQuery->bindValue(":altitude", airportAltitude);

  Pos pos(line.at(hp::LONX).toFloat(), line.at(hp::LATY).toFloat());
  airportRect.extend(pos);
  insertHelipadQuery->bindValue(":laty", pos.getLatY());
  insertHelipadQuery->bindValue(":lonx", pos.getLonX());

  insertHelipadQuery->exec();
}

void XpAirportWriter::writeRunway(const QStringList& line, AirportRowCode rowCode)
{
  if(ignoringAirport)
    return;

  Pos primaryPos, secondaryPos;
  QString primaryName, secondaryName;
  Surface surface(UNKNOWN);
  if(rowCode == LAND_RUNWAY)
  {
    primaryPos = Pos(line.at(rw::PRIMARY_LONX).toFloat(), line.at(rw::PRIMARY_LATY).toFloat());
    secondaryPos = Pos(line.at(rw::SECONDARY_LONX).toFloat(), line.at(rw::SECONDARY_LATY).toFloat());
    primaryName = line.at(rw::PRIMARY_NUMBER);
    secondaryName = line.at(rw::SECONDARY_NUMBER);
    surface = static_cast<Surface>(line.at(rw::SURFACE).toInt());
  }
  else if(rowCode == WATER_RUNWAY)
  {
    primaryPos = Pos(line.at(rw::WATER_PRIMARY_LONX).toFloat(), line.at(rw::WATER_PRIMARY_LATY).toFloat());
    secondaryPos = Pos(line.at(rw::WATER_SECONDARY_LONX).toFloat(), line.at(rw::WATER_SECONDARY_LATY).toFloat());
    primaryName = line.at(rw::WATER_PRIMARY_NUMBER);
    secondaryName = line.at(rw::WATER_SECONDARY_NUMBER);
    surface = WATER;
  }
  else
    throw Exception(tr("Invalid runway code"));

  int primRwEndId = ++curRunwayEndId;
  int secRwEndId = ++curRunwayEndId;

  airportIndex->addRunwayEnd(airportIcao, primaryName, primRwEndId);
  airportIndex->addRunwayEnd(airportIcao, secondaryName, secRwEndId);

  float lengthMeter = primaryPos.distanceMeterTo(secondaryPos);
  float lengthFeet = meterToFeet(lengthMeter);
  float widthFeet = meterToFeet(line.at(rw::WIDTH).toFloat());
  float heading = primaryPos.angleDegTo(secondaryPos);
  Pos center = primaryPos.interpolate(secondaryPos, lengthMeter, 0.5f);
  airportRect.extend(primaryPos);
  airportRect.extend(secondaryPos);

  if(isSurfaceHard(surface))
    numHardRunway++;

  numSoftRunway++;
  if(isSurfaceSoft(surface))
    if(isSurfaceWater(surface))
      numWaterRunway++;

  QString surfaceStr = surfaceToDb(surface);
  if(lengthFeet > longestRunwayLength)
  {
    longestRunwayLength = lengthFeet;
    longestRunwayWidth = widthFeet;
    longestRunwayHeading = heading;
    longestRunwaySurface = surfaceStr;
  }

  insertRunwayQuery->bindValue(":runway_id", primRwEndId);
  insertRunwayQuery->bindValue(":airport_id", curAirportId);
  insertRunwayQuery->bindValue(":primary_end_id", primRwEndId);
  insertRunwayQuery->bindValue(":secondary_end_id", secRwEndId);
  insertRunwayQuery->bindValue(":surface", surfaceStr);
  insertRunwayQuery->bindValue(":length", lengthFeet);
  insertRunwayQuery->bindValue(":width", widthFeet);
  insertRunwayQuery->bindValue(":heading", heading);

  if(rowCode == LAND_RUNWAY)
  {
    insertRunwayQuery->bindValue(":marking_flags",
                                 markingToDb(static_cast<Marking>(line.at(rw::PRIMARY_MARKINGS).toInt())) |
                                 markingToDb(static_cast<Marking>(line.at(rw::SECONDARY_MARKINGS).toInt())));

    int edgeLights = line.at(rw::EDGE_LIGHTS).toInt();

    if(edgeLights == 0)
      insertRunwayQuery->bindValue(":edge_light", QVariant(QVariant::String));
    else if(edgeLights == 1)
      insertRunwayQuery->bindValue(":edge_light", "L");
    else if(edgeLights == 2)
      insertRunwayQuery->bindValue(":edge_light", "M");
    else if(edgeLights == 3)
      insertRunwayQuery->bindValue(":edge_light", "H");

    int centerLights = line.at(rw::CENTER_LIGHTS).toInt();
    if(centerLights == 1)
      insertRunwayQuery->bindValue(":center_light", "M");
    else
      insertRunwayQuery->bindValue(":center_light", QVariant(QVariant::String));

    if(edgeLights > 0 || centerLights > 0)
      numLightRunway++;
  }
  else
    insertRunwayQuery->bindValue(":marking_flags", 0);

  insertRunwayQuery->bindValue(":pattern_altitude", 0);
  insertRunwayQuery->bindValue(":has_center_red", 0);
  insertRunwayQuery->bindValue(":primary_lonx", primaryPos.getLonX());
  insertRunwayQuery->bindValue(":primary_laty", primaryPos.getLatY());
  insertRunwayQuery->bindValue(":secondary_lonx", secondaryPos.getLonX());
  insertRunwayQuery->bindValue(":secondary_laty", secondaryPos.getLatY());
  insertRunwayQuery->bindValue(":altitude", airportAltitude);
  insertRunwayQuery->bindValue(":lonx", center.getLonX());
  insertRunwayQuery->bindValue(":laty", center.getLatY());

  // ===========================================================================================
  // Primary end ==============================
  SqlRecord rec = runwayEndRecord;
  rec.setValue(":runway_end_id", primRwEndId);
  rec.setValue(":name", primaryName);
  rec.setValue(":end_type", "P");

  if(rowCode == LAND_RUNWAY)
  {
    rec.setValue(":offset_threshold",
                 meterToFeet(line.at(rw::PRIMARY_DISPLACED_THRESHOLD).toFloat()));
    rec.setValue(":blast_pad", meterToFeet(line.at(rw::PRIMARY_OVERRUN_BLASTPAD).toFloat()));

    QString als = alsToDb(static_cast<ApproachLight>(line.at(rw::PRIMARY_ALS).toInt()));
    if(!als.isEmpty())
      rec.setValue(":app_light_system_type", als);
    else
    {
      numRunwayEndAls++;
      rec.setValue(":app_light_system_type", QVariant(QVariant::String));
    }

    rec.setValue(":has_reils", line.at(rw::PRIMARY_REIL).toInt() > 0);
    rec.setValue(":has_touchdown_lights", line.at(rw::PRIMARY_TDZ_LIGHT).toInt());
  }
  else
  {
    rec.setValue(":offset_threshold", 0);
    rec.setValue(":blast_pad", 0);
    rec.setValue(":app_light_system_type", QVariant(QVariant::String));
    rec.setValue(":has_reils", 0);
    rec.setValue(":has_touchdown_lights", 0);
  }

  rec.setValue(":has_end_lights", 0);
  rec.setValue(":num_strobes", 0);
  rec.setValue(":overrun", 0);
  rec.setValue(":has_closed_markings", 0);
  rec.setValue(":has_stol_markings", 0);
  rec.setValue(":is_takeoff", 1);
  rec.setValue(":is_landing", 1);
  rec.setValue(":is_pattern", "N");

  rec.setValue(":heading", heading);
  rec.setValue(":lonx", center.getLonX());
  rec.setValue(":laty", center.getLatY());

  runwayEndRecords.append(rec);

  // ===========================================================================================
  // Secondary end ==============================
  rec = runwayEndRecord;
  rec.setValue(":runway_end_id", secRwEndId);
  rec.setValue(":name", secondaryName);
  rec.setValue(":end_type", "S");

  if(rowCode == LAND_RUNWAY)
  {
    rec.setValue(":offset_threshold",
                 meterToFeet(line.at(rw::SECONDARY_DISPLACED_THRESHOLD).toFloat()));
    rec.setValue(":blast_pad",
                 meterToFeet(line.at(rw::SECONDARY_OVERRUN_BLASTPAD).toFloat()));

    QString als = alsToDb(static_cast<ApproachLight>(line.at(rw::SECONDARY_ALS).toInt()));
    if(!als.isEmpty())
      rec.setValue(":app_light_system_type", als);
    else
      rec.setValue(":app_light_system_type", QVariant(QVariant::String));

    rec.setValue(":has_reils", line.at(rw::SECONDARY_REIL).toInt() > 0);
    rec.setValue(":has_touchdown_lights", line.at(rw::SECONDARY_TDZ_LIGHT).toInt());
  }
  else
  {
    rec.setValue(":offset_threshold", 0);
    rec.setValue(":blast_pad", 0);
    rec.setValue(":app_light_system_type", QVariant(QVariant::String));
    rec.setValue(":has_reils", 0);
    rec.setValue(":has_touchdown_lights", 0);
  }

  rec.setValue(":has_end_lights", 0);
  rec.setValue(":num_strobes", 0);
  rec.setValue(":overrun", 0);
  rec.setValue(":has_closed_markings", 0);
  rec.setValue(":has_stol_markings", 0);
  rec.setValue(":is_takeoff", 1);
  rec.setValue(":is_landing", 1);
  rec.setValue(":is_pattern", "N"); // NONE

  rec.setValue(":heading", atools::geo::normalizeCourse(atools::geo::opposedCourseDeg(heading)));
  rec.setValue(":lonx", center.getLonX());
  rec.setValue(":laty", center.getLatY());

  runwayEndRecords.append(rec);

  insertRunwayQuery->exec();
  insertRunwayQuery->clearBoundValues();
}

void XpAirportWriter::bindAirport(const QStringList& line, AirportRowCode rowCode, const XpWriterContext& context)
{
  static QRegularExpression nameRegexp("^\\[.\\] .+");

  int airportId = ++curAirportId;
  airportIcao = line.value(ap::ICAO);

  writeAirportFile(airportIcao, context.curFileId);

  if(!airportIndex->addAirport(airportIcao, airportId))
    ignoringAirport = true;
  else
  {
    writingAirport = true;

    airportRowCode = rowCode;

    insertAirportQuery->bindValue(":airport_id", airportId);
    insertAirportQuery->bindValue(":file_id", context.curFileId);

    airportAltitude = line.value(ap::ELEVATION).toFloat();

    QString name = line.mid(ap::NAME).join(" ");

    if(nameRegexp.match(name).hasMatch())
      name = name.mid(4);

    bool isMil = atools::fs::util::isNameMilitary(name);
    name = atools::fs::util::capNavString(name);

    insertAirportQuery->bindValue(":ident", airportIcao);
    insertAirportQuery->bindValue(":name", name);
    insertAirportQuery->bindValue(":fuel_flags", 0);
    insertAirportQuery->bindValue(":has_tower_object", 0);
    insertAirportQuery->bindValue(":is_closed", 0);
    insertAirportQuery->bindValue(":is_military", isMil);
    insertAirportQuery->bindValue(":is_addon", context.addOn);
    insertAirportQuery->bindValue(":num_boundary_fence", 0);
    insertAirportQuery->bindValue(":num_parking_gate", 0);
    insertAirportQuery->bindValue(":num_parking_ga_ramp", 0);
    insertAirportQuery->bindValue(":num_parking_cargo", 0);
    insertAirportQuery->bindValue(":num_parking_mil_cargo", 0);
    insertAirportQuery->bindValue(":num_parking_mil_combat", 0);
    insertAirportQuery->bindValue(":num_approach", 0);
    insertAirportQuery->bindValue(":num_runway_end_closed", 0);
    // insertAirportQuery->bindValue(":num_runway_end_ils", 0); filled later
    insertAirportQuery->bindValue(":num_apron", 0);
    insertAirportQuery->bindValue(":num_taxi_path", 0);
    insertAirportQuery->bindValue(":num_jetway", 0);
    // TODO null largest_parking_ramp
    // TODO null largest_parking_gate
    insertAirportQuery->bindValue(":rating", 0);
    insertAirportQuery->bindValue(":scenery_local_path", context.localPath);
    insertAirportQuery->bindValue(":bgl_filename", context.fileName);
    insertAirportQuery->bindValue(":mag_var", 0);
    insertAirportQuery->bindValue(":altitude", airportAltitude);

    insertAirportQuery->bindValue(":has_jetfuel", 0);
    insertAirportQuery->bindValue(":has_avgas", 0);
  }
}

void XpAirportWriter::finishAirport()
{
  if(writingAirport && !ignoringAirport)
  {
    insertAirportQuery->bindValue(":longest_runway_length", longestRunwayLength);
    insertAirportQuery->bindValue(":longest_runway_width", longestRunwayWidth);
    insertAirportQuery->bindValue(":longest_runway_heading", longestRunwayHeading);
    insertAirportQuery->bindValue(":longest_runway_surface", longestRunwaySurface);
    insertAirportQuery->bindValue(":num_runways", numSoftRunway + numWaterRunway + numHardRunway);
    insertAirportQuery->bindValue(":num_runway_hard", numHardRunway);
    insertAirportQuery->bindValue(":num_runway_soft", numSoftRunway);
    insertAirportQuery->bindValue(":num_runway_water", numWaterRunway);
    insertAirportQuery->bindValue(":num_runway_light", numLightRunway);
    insertAirportQuery->bindValue(":num_helipad", numHelipad);
    insertAirportQuery->bindValue(":num_com", numCom);
    insertAirportQuery->bindValue(":num_runway_end_als", numRunwayEndAls);
    insertAirportQuery->bindValue(":num_starts", numStart);
    insertAirportQuery->bindValue(":num_runway_end_vasi", numVasi);

    if(!airportRect.isValid())
      airportRect = Rect(airportPos);

    if(airportRect.isPoint())
      airportRect.inflate(1.f / 60.f, 1.f / 60.f);

    insertAirportQuery->bindValue(":left_lonx", airportRect.getTopLeft().getLonX());
    insertAirportQuery->bindValue(":top_laty", airportRect.getTopLeft().getLatY());
    insertAirportQuery->bindValue(":right_lonx", airportRect.getBottomRight().getLonX());
    insertAirportQuery->bindValue(":bottom_laty", airportRect.getBottomRight().getLatY());

    Pos center = airportPos.isValid() ? airportPos : airportRect.getCenter();
    insertAirportQuery->bindValue(":lonx", center.getLonX());
    insertAirportQuery->bindValue(":laty", center.getLatY());

    insertAirportQuery->exec();
    insertAirportQuery->clearBoundValues();

    progress->incNumAirports();

    for(const SqlRecord& rec : runwayEndRecords)
    {
      insertRunwayEndQuery->bindRecord(rec);
      insertRunwayEndQuery->exec();
    }
    insertRunwayEndQuery->clearBoundValues();

  }
  airportRect = Rect();
  airportPos = Pos();

  longestRunwayLength = 0;
  longestRunwayWidth = 0;
  longestRunwayHeading = 0;
  longestRunwaySurface = "UNKNOWN";
  numSoftRunway = 0;
  numWaterRunway = 0;
  numHardRunway = 0;
  numHelipad = 0;
  numLightRunway = 0;
  numCom = 0;
  numStart = 0;
  numVasi = 0;
  numRunwayEndAls = 0;
  airportAltitude = 0.f;
  airportRowCode = NO_ROWCODE;
  airportIcao.clear();
  runwayEndRecords.clear();

  writingAirport = false;
  ignoringAirport = false;
}

void XpAirportWriter::writeAirportFile(const QString& icao, int curFileId)
{
  insertAirportFileQuery->bindValue(":airport_file_id", --curAirportFileId);
  insertAirportFileQuery->bindValue(":file_id", curFileId);
  insertAirportFileQuery->bindValue(":ident", icao);
  insertAirportFileQuery->exec();
}

void XpAirportWriter::initRunwayEndRecord()
{
  runwayEndRecord.clear();
  runwayEndRecord.appendField(":runway_end_id", QVariant::Int);
  runwayEndRecord.appendField(":name", QVariant::String);
  runwayEndRecord.appendField(":end_type", QVariant::String);
  runwayEndRecord.appendField(":offset_threshold", QVariant::Int);
  runwayEndRecord.appendField(":blast_pad", QVariant::Int);
  runwayEndRecord.appendField(":overrun", QVariant::Int);
  runwayEndRecord.appendField(":left_vasi_type", QVariant::String);
  runwayEndRecord.appendField(":left_vasi_pitch", QVariant::Double);
  runwayEndRecord.appendField(":right_vasi_type", QVariant::String);
  runwayEndRecord.appendField(":right_vasi_pitch", QVariant::Double);
  runwayEndRecord.appendField(":has_closed_markings", QVariant::Int);
  runwayEndRecord.appendField(":has_stol_markings", QVariant::Int);
  runwayEndRecord.appendField(":is_takeoff", QVariant::Int);
  runwayEndRecord.appendField(":is_landing", QVariant::Int);
  runwayEndRecord.appendField(":is_pattern", QVariant::Int);
  runwayEndRecord.appendField(":app_light_system_type", QVariant::String);
  runwayEndRecord.appendField(":has_end_lights", QVariant::Int);
  runwayEndRecord.appendField(":has_reils", QVariant::Int);
  runwayEndRecord.appendField(":has_touchdown_lights", QVariant::Int);
  runwayEndRecord.appendField(":num_strobes", QVariant::Int);
  runwayEndRecord.appendField(":ils_ident", QVariant::String);
  runwayEndRecord.appendField(":heading", QVariant::Double);
  runwayEndRecord.appendField(":lonx", QVariant::Double);
  runwayEndRecord.appendField(":laty", QVariant::Double);
}

void XpAirportWriter::initQueries()
{
  deInitQueries();

  SqlUtil util(&db);

  insertAirportQuery = new SqlQuery(db);
  insertAirportQuery->prepare(util.buildInsertStatement("airport"));

  insertRunwayQuery = new SqlQuery(db);
  insertRunwayQuery->prepare(util.buildInsertStatement("runway"));

  insertRunwayEndQuery = new SqlQuery(db);
  insertRunwayEndQuery->prepare(util.buildInsertStatement("runway_end"));

  insertHelipadQuery = new SqlQuery(db);
  insertHelipadQuery->prepare(util.buildInsertStatement("helipad"));

  insertComQuery = new SqlQuery(db);
  insertComQuery->prepare(util.buildInsertStatement("com"));

  insertStartQuery = new SqlQuery(db);
  insertStartQuery->prepare(util.buildInsertStatement("start"));

  insertAirportFileQuery = new SqlQuery(db);
  insertAirportFileQuery->prepare(util.buildInsertStatement("airport_file"));
}

void XpAirportWriter::deInitQueries()
{
  delete insertAirportQuery;
  insertAirportQuery = nullptr;

  delete insertRunwayQuery;
  insertRunwayQuery = nullptr;

  delete insertRunwayEndQuery;
  insertRunwayEndQuery = nullptr;

  delete insertHelipadQuery;
  insertHelipadQuery = nullptr;

  delete insertComQuery;
  insertComQuery = nullptr;

  delete insertStartQuery;
  insertStartQuery = nullptr;

  delete insertAirportFileQuery;
  insertAirportFileQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
