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
namespace x = atools::fs::xp;

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

namespace n {
enum NodeFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  CTRL_LATY = 3,
  CTRL_LONX = 4
};

}

namespace p {
enum PavementFieldIndex
{
  ROWCODE = 0,
  SURFACE = 1,
  SMOOTHNESS = 2,
  ORIENT = 3,
  DESCRIPTION = 4
};

}

namespace tn {
enum TaxiNodeFieldIndex
{
  ROWCODE = 0,
  LATY = 1,
  LONX = 2,
  USAGE = 3,
  ID = 4,
  NAME = 5
};

}

namespace te {
enum TaxiEdgeFieldIndex
{
  ROWCODE = 0,
  START = 1,
  END = 2,
  DIR = 3,
  TYPE = 4,
  NAME = 5
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

namespace sm {
enum StartupLocationMetaFieldIndex
{
  ROWCODE = 0,
  WIDTH = 1,
  OPTYPE = 2,
  AIRLINE = 3
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

  if(!contains(rowCode, {x::PAVEMENT_HEADER, x::NODE, x::NODE_AND_CONTROL_POINT,
                         x::NODE_CLOSE, x::NODE_AND_CONTROL_POINT_CLOSE}))
    finishPavement();

  if(rowCode != x::RAMP_START_METADATA)
    finishStartupLocation();

  switch(rowCode)
  {
    case x::LAND_AIRPORT_HEADER:
    case x::SEAPLANE_BASE_HEADER:
    case x::HELIPORT_HEADER:
      finishAirport(context);
      bindAirport(line, rowCode, context);
      break;

    case x::LAND_RUNWAY:
    case x::WATER_RUNWAY:
      writeRunway(line, rowCode);
      break;

    case x::HELIPAD:
      writeHelipad(line);
      break;

    case x::PAVEMENT_HEADER:
      finishPavement();
      bindPavement(line);
      break;

    case x::NODE:
    case x::NODE_AND_CONTROL_POINT:
    case x::NODE_CLOSE:
    case x::NODE_AND_CONTROL_POINT_CLOSE:
      bindPavementNode(line, rowCode);
      break;

    case x::AIRPORT_VIEWPOINT:
      bindViewpoint(line);
      break;

    case x::AEROPLANE_STARTUP_LOCATION:
      writeStart(line);
      break;

    case x::LIGHTING_OBJECT:
      bindVasi(line);
      break;

    case x::AIRPORT_LOCATION:
      finishStartupLocation();
      writeStartupLocation(line);
      break;

    case x::RAMP_START_METADATA:
      writeStartupLocationMetadata(line);
      break;

    case x::TAXI_ROUTE_NETWORK_NODE:
      bindTaxiNode(line);
      break;
    case x::TAXI_ROUTE_NETWORK_EDGE:
      bindTaxiEdge(line);
      break;

    case x::METADATA_RECORDS:
      bindMetadata(line);
      break;

    case x::TRUCK_PARKING_LOCATION:
    case x::TRUCK_DESTINATION_LOCATION:
      bindFuel(line);
      break;

    case x::COM_WEATHER:
    case x::COM_UNICOM:
    case x::COM_CLEARANCE:
    case x::COM_GROUND:
    case x::COM_TOWER:
    case x::COM_APPROACH:
    case x::COM_DEPARTURE:
      writeCom(line, rowCode);
      break;

    case x::LINEAR_FEATURE_HEADER:
    case x::AIRPORT_BOUNDARY_HEADER:
    case x::NODE_TERMINATING_A_STRING:
    case x::NODE_WITH_BEZIER_CONTROL_POINT_NO_CLOSE:
    case x::AIRPORT_LIGHT_BEACON:
    case x::WINDSOCK:
    case x::TAXIWAY_SIGN:
    case x::AIRPORT_TRAFFIC_FLOW:
    case x::TRAFFIC_FLOW_WIND_RULE:
    case x::TRAFFIC_FLOW_MINIMUM_CEILING_RULE:
    case x::TRAFFIC_FLOW_MINIMUM_VISIBILITY_RULE:
    case x::TRAFFIC_FLOW_TIME_RULE:
    case x::RUNWAY_IN_USE:
    case x::VFR_TRAFFIC_PATTERN:
    case x::HEADER_INDICATING_THAT_TAXI_ROUTE_NETWORK_DATA_FOLLOWS:
    case x::TAXI_ROUTE_EDGE_ACTIVE_ZONE:
    case x::NO_ROWCODE:
      break;
  }
}

void XpAirportWriter::finish(const XpWriterContext& context)
{
  finishPavement();
  finishAirport(context);
}

void XpAirportWriter::bindTaxiNode(const QStringList& line)
{
  taxiNodes.insert(line.at(tn::ID).toInt(), Pos(line.at(tn::LONX).toFloat(), line.at(tn::LATY).toFloat()));
}

void XpAirportWriter::bindTaxiEdge(const QStringList& line)
{
  if(line.at(te::TYPE) == "runway")
    return;

  const Pos& start = taxiNodes.value(line.at(te::START).toInt());
  const Pos& end = taxiNodes.value(line.at(te::END).toInt());
  airportRect.extend(start);
  airportRect.extend(end);

  QString name = line.size() > te::NAME ? line.at(te::NAME) : QString();

  numTaxiPath++;
  insertTaxiQuery->bindValue(":taxi_path_id", ++curTaxiPathId);
  insertTaxiQuery->bindValue(":airport_id", curAirportId);
  insertTaxiQuery->bindValue(":surface", QVariant(QVariant::String));
  insertTaxiQuery->bindValue(":width", 0.f);
  insertTaxiQuery->bindValue(":name", name);
  insertTaxiQuery->bindValue(":type", "T" /* taxi */);
  insertTaxiQuery->bindValue(":is_draw_surface", 1);
  insertTaxiQuery->bindValue(":is_draw_detail", 1);

  insertTaxiQuery->bindValue(":has_centerline", 0);
  insertTaxiQuery->bindValue(":has_centerline_light", 0);
  insertTaxiQuery->bindValue(":has_left_edge_light", 0);
  insertTaxiQuery->bindValue(":has_right_edge_light", 0);

  insertTaxiQuery->bindValue(":start_type", "N" /* Normal */);
  insertTaxiQuery->bindValue(":start_lonx", start.getLonX());
  insertTaxiQuery->bindValue(":start_laty", start.getLatY());

  insertTaxiQuery->bindValue(":end_type", "N" /* Normal */);
  insertTaxiQuery->bindValue(":end_lonx", end.getLonX());
  insertTaxiQuery->bindValue(":end_laty", end.getLatY());

  insertTaxiQuery->exec();
}

void XpAirportWriter::bindPavement(const QStringList& line)
{
  currentPavement.clear();
  writingPavementBoundary = true;
  writingPavementHoles = false;
  writingPavementNewHole = false;

  numApron++;
  insertApronQuery->bindValue(":apron_id", ++curApronId);
  insertApronQuery->bindValue(":airport_id", curAirportId);
  insertApronQuery->bindValue(":is_draw_surface", 1);
  insertApronQuery->bindValue(":is_draw_detail", 1);
  insertApronQuery->bindValue(":surface", surfaceToDb(static_cast<Surface>(line.at(p::SURFACE).toInt())));
}

void XpAirportWriter::bindPavementNode(const QStringList& line, atools::fs::xp::AirportRowCode rowCode)
{
  Pos node(line.at(n::LONX).toFloat(), line.at(n::LATY).toFloat());
  Pos control;
  airportRect.extend(node);

  if(rowCode == x::NODE_AND_CONTROL_POINT || rowCode == x::NODE_AND_CONTROL_POINT_CLOSE)
    control = Pos(line.at(n::CTRL_LONX).toFloat(), line.at(n::CTRL_LATY).toFloat());

  if(writingPavementBoundary)
    currentPavement.addBoundaryNode(node, control);
  else if(writingPavementHoles)
    currentPavement.addHoleNode(node, control, writingPavementNewHole);

  writingPavementNewHole = false;

  if(rowCode == x::NODE_CLOSE || rowCode == x::NODE_AND_CONTROL_POINT_CLOSE)
  {
    if(writingPavementBoundary)
    {
      writingPavementBoundary = false;
      writingPavementHoles = true;
    }

    if(writingPavementHoles)
      writingPavementNewHole = true;
  }
}

void XpAirportWriter::finishPavement()
{
  if(ignoringAirport)
    return;

  if(writingPavementBoundary || writingPavementHoles)
  {
    insertApronQuery->bindValue(":geometry", currentPavement.writeToByteArray());
    insertApronQuery->exec();
    writingPavementBoundary = false;
    writingPavementHoles = false;
    writingPavementNewHole = false;
  }
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
    numRunwayEndVasi++;
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
  hasTower = true;
}

void XpAirportWriter::writeStartupLocationMetadata(const QStringList& line)
{

  // Operation type none, general_aviation, airline, cargo, military
  // Airline permitted to use this ramp 3-letter airline codes (AAL, SWA, etc)

  QString ops = line.at(sm::OPTYPE);
  if(ops == "general_aviation")
    insertParkingQuery->bindValue(":type", "RGA");
  else if(ops == "cargo")
    insertParkingQuery->bindValue(":type", "RC");
  else if(ops == "military")
    insertParkingQuery->bindValue(":type", "RM");
  // else if(ops == "airline")
  // else if(ops == "none")

  if(line.size() > sm::AIRLINE)
    insertParkingQuery->bindValue(":airline_codes", line.at(sm::AIRLINE).toUpper());

  QString sizeType;

  // ICAO width code A 15 m, B 25 m, C 35 m, D 50 m, E 65 m, F 80 m
  QString widthCode = line.at(sm::WIDTH);
  if(widthCode == "A")
  {
    insertParkingQuery->bindValue(":radius", 25.f);
    sizeType = "S";
  }
  else if(widthCode == "B")
  {
    insertParkingQuery->bindValue(":radius", 40.f);
    sizeType = "S";
  }
  else if(widthCode == "C")
  {
    insertParkingQuery->bindValue(":radius", 60.f);
    sizeType = "M";
  }
  else if(widthCode == "D")
  {
    insertParkingQuery->bindValue(":radius", 80.f);
    sizeType = "M";
  }
  else if(widthCode == "E")
  {
    insertParkingQuery->bindValue(":radius", 100.f);
    sizeType = "H";
  }
  else if(widthCode == "F")
  {
    insertParkingQuery->bindValue(":radius", 130.f);
    sizeType = "H";
  }

  QString type = insertParkingQuery->boundValue(":type").toString();
  if(type == "G" || type == "RGA")
    insertParkingQuery->bindValue(":type", type + sizeType);

  // TYPES
  // {"INVALID", QObject::tr("Invalid")},
  // {"UNKNOWN", QObject::tr("Unknown")},
  // {"RGA", QObject::tr("Ramp GA")},
  // {"RGAS", QObject::tr("Ramp GA Small")},
  // {"RGAM", QObject::tr("Ramp GA Medium")},
  // {"RGAL", QObject::tr("Ramp GA Large")},
  // {"RC", QObject::tr("Ramp Cargo")},
  // {"RMC", QObject::tr("Ramp Mil Cargo")},
  // {"RMCB", QObject::tr("Ramp Mil Combat")},
  // {"T", QObject::tr("Tie down")},
  // {"H", QObject::tr("Hangar")},
  // {"G", QObject::tr("Gate")},
  // {"GS", QObject::tr("Gate Small")},
  // {"GM", QObject::tr("Gate Medium")},
  // {"GH", QObject::tr("Gate Heavy")},
  // {"DGA", QObject::tr("Dock GA")},
  // {"FUEL", QObject::tr("Fuel")},
  // {"V", QObject::tr("Vehicles")}
}

void XpAirportWriter::finishStartupLocation()
{
  if(writingStartLocation)
  {
    QString type = insertParkingQuery->boundValue(":type").toString();

    if(type.startsWith("G"))
    {
      numParkingGate++;

      if(largestParkingGate > type || largestParkingGate.isEmpty())
        largestParkingGate = type;
    }

    if(type.startsWith("RGA"))
    {
      numParkingGaRamp++;
      if(largestParkingRamp > type || largestParkingRamp.isEmpty())
        largestParkingRamp = type;
    }

    if(type.startsWith("RC"))
      numParkingCargo++;

    if(type.startsWith("RMC"))
    {
      numParkingMilCombat++;
      numParkingMilCargo++;
    }

    insertAirportQuery->bindValue(":largest_parking_ramp", largestParkingRamp);
    insertAirportQuery->bindValue(":largest_parking_gate", largestParkingGate);

    insertParkingQuery->exec();
    writingStartLocation = false;
  }
}

void XpAirportWriter::writeStartupLocation(const QStringList& line)
{
  if(ignoringAirport)
    return;

  writingStartLocation = true;
  numParking++;
  insertParkingQuery->bindValue(":parking_id", ++curParkingId);
  insertParkingQuery->bindValue(":airport_id", curAirportId);

  Pos pos(line.at(sl::LONX).toFloat(), line.at(sl::LATY).toFloat());
  airportRect.extend(pos);
  insertParkingQuery->bindValue(":laty", pos.getLatY());
  insertParkingQuery->bindValue(":lonx", pos.getLonX());

  insertParkingQuery->bindValue(":heading", line.at(sl::HEADING).toFloat());
  insertParkingQuery->bindValue(":number", -1);
  insertParkingQuery->bindValue(":radius", 50.f);
  insertParkingQuery->bindValue(":airline_codes", QVariant(QVariant::String));
  insertParkingQuery->bindValue(":name", line.mid(sl::NAME).join(" "));
  insertParkingQuery->bindValue(":has_jetway", 0);

  QString type = line.at(sl::TYPE);
  if(type == "gate")
    insertParkingQuery->bindValue(":type", "G");
  else if(type == "hangar")
    insertParkingQuery->bindValue(":type", "H");
  else if(type == "tie-down")
    insertParkingQuery->bindValue(":type", "T");
  else if(type == "misc")
    insertParkingQuery->bindValue(":type", "");

  // has_jetway integer not null,     -- 1 if the parking has a jetway attached

  // Airplane types that can use this location
  // Pipe-separated list (|). Can include heavy, jets,
  // turboprops, props and helos (or just all for all types)
}

// Obsolete type 15
void XpAirportWriter::writeStart(const QStringList& line)
{
  if(ignoringAirport)
    return;

  writingStartLocation = true;
  numParking++;
  insertParkingQuery->bindValue(":parking_id", ++curParkingId);
  insertParkingQuery->bindValue(":airport_id", curAirportId);

  Pos pos(line.at(s::LONX).toFloat(), line.at(s::LATY).toFloat());
  airportRect.extend(pos);
  insertParkingQuery->bindValue(":laty", pos.getLatY());
  insertParkingQuery->bindValue(":lonx", pos.getLonX());

  insertParkingQuery->bindValue(":heading", line.at(s::HEADING).toFloat());
  insertParkingQuery->bindValue(":number", -1);
  insertParkingQuery->bindValue(":radius", 50.f);
  insertParkingQuery->bindValue(":airline_codes", QVariant(QVariant::String));
  insertParkingQuery->bindValue(":name", line.mid(s::NAME).join(" "));
  insertParkingQuery->bindValue(":has_jetway", 0);
  insertParkingQuery->bindValue(":type", "");

  finishStartupLocation();

  // numStart++;
  // insertStartQuery->bindValue(":start_id", ++curStartId);
  // insertStartQuery->bindValue(":airport_id", curAirportId);

  // Pos pos(line.at(s::LONX).toFloat(), line.at(s::LATY).toFloat());
  // airportRect.extend(pos);
  // insertStartQuery->bindValue(":laty", pos.getLatY());
  // insertStartQuery->bindValue(":lonx", pos.getLonX());
  // insertStartQuery->bindValue(":altitude", airportAltitude);
  // insertStartQuery->bindValue(":heading", line.at(s::HEADING).toFloat());
  // insertAirportQuery->bindValue(":name", line.at(s::NAME));

  // runway_end_id
  // runway_name // helipad and runway
  // type // R, W, H
  // number // only helipad
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

  if(rowCode == x::COM_WEATHER)
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
  else if(rowCode == x::COM_UNICOM)
  {
    insertAirportQuery->bindValue(":unicom_frequency", frequency);
    insertComQuery->bindValue(":type", "UC");
  }
  else if(rowCode == x::COM_TOWER)
  {
    insertAirportQuery->bindValue(":tower_frequency", frequency);
    insertComQuery->bindValue(":type", "T");
  }
  else if(rowCode == x::COM_CLEARANCE)
    insertComQuery->bindValue(":type", "C");
  else if(rowCode == x::COM_GROUND)
    insertComQuery->bindValue(":type", "G");
  else if(rowCode == x::COM_APPROACH)
    insertComQuery->bindValue(":type", "A");
  else if(rowCode == x::COM_DEPARTURE)
    insertComQuery->bindValue(":type", "D");

  insertComQuery->exec();
}

void XpAirportWriter::bindFuel(const QStringList& line)
{
  if(ignoringAirport)
    return;

  QString type = line.at(4);
  // Pipe separated list (“|”). Include 1 or more of the following:
  // baggage_loader, baggage_train, crew_car, crew_ferrari, crew_limo, pushback, fuel_liners, fuel_jets, fuel_props, food, gpu

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

  insertHelipadQuery->bindValue(":type", "H"); // not available
  insertHelipadQuery->bindValue(":is_transparent", 0); // not available
  insertHelipadQuery->bindValue(":is_closed", 0); // not available

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

  int shoulder = line.at(rw::SHOULDER_SURFACE).toInt();
  if(shoulder == 1)
    insertRunwayQuery->bindValue(":shoulder", surfaceToDb(ASPHALT));
  else if(shoulder == 2)
    insertRunwayQuery->bindValue(":shoulder", surfaceToDb(CONCRETE));
  else
    insertRunwayQuery->bindValue(":shoulder", QVariant(QVariant::String));

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
      insertRunwayQuery->bindValue(":center_light", "M"); // Either none or medium
    else
      insertRunwayQuery->bindValue(":center_light", QVariant(QVariant::String));

    if(edgeLights > 0 || centerLights > 0)
      numLightRunway++;
  }
  else
    insertRunwayQuery->bindValue(":marking_flags", 0);

  insertRunwayQuery->bindValue(":pattern_altitude", 0); // not available
  insertRunwayQuery->bindValue(":has_center_red", 0); // not available
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
    rec.setValue(":offset_threshold", meterToFeet(line.at(rw::PRIMARY_DISPLACED_THRESHOLD).toFloat()));
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

  rec.setValue(":has_end_lights", 0); // not available
  rec.setValue(":num_strobes", 0); // not available
  rec.setValue(":overrun", 0); // not available
  rec.setValue(":has_closed_markings", 0); // not available
  rec.setValue(":has_stol_markings", 0); // not available
  rec.setValue(":is_takeoff", 1); // not available
  rec.setValue(":is_landing", 1); // not available
  rec.setValue(":is_pattern", "N"); // not available

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
    rec.setValue(":offset_threshold", meterToFeet(line.at(rw::SECONDARY_DISPLACED_THRESHOLD).toFloat()));
    rec.setValue(":blast_pad", meterToFeet(line.at(rw::SECONDARY_OVERRUN_BLASTPAD).toFloat()));

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
    insertAirportQuery->bindValue(":fuel_flags", 0); // not available
    insertAirportQuery->bindValue(":has_tower_object", 0);
    insertAirportQuery->bindValue(":is_closed", 0); // not available
    insertAirportQuery->bindValue(":is_military", isMil);
    insertAirportQuery->bindValue(":is_addon", context.addOn);
    insertAirportQuery->bindValue(":num_boundary_fence", 0);

    insertAirportQuery->bindValue(":num_approach", 0); // TODO num_approach fill later
    insertAirportQuery->bindValue(":num_runway_end_closed", 0); // not available
    // insertAirportQuery->bindValue(":num_runway_end_ils", 0); filled later - nothing to do here
    insertAirportQuery->bindValue(":num_jetway", 0); // not available
    insertAirportQuery->bindValue(":scenery_local_path", context.localPath);
    insertAirportQuery->bindValue(":bgl_filename", context.fileName);
    insertAirportQuery->bindValue(":mag_var", 0); // TODO magvar
    insertAirportQuery->bindValue(":altitude", airportAltitude);

    insertAirportQuery->bindValue(":has_jetfuel", 0); // filled later
    insertAirportQuery->bindValue(":has_avgas", 0); // filled later
  }
}

void XpAirportWriter::finishAirport(const XpWriterContext& context)
{
  // TODO add starts from runway and helipads
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
    insertAirportQuery->bindValue(":num_runway_end_vasi", numRunwayEndVasi);
    insertAirportQuery->bindValue(":num_apron", numApron);
    insertAirportQuery->bindValue(":num_taxi_path", numTaxiPath);

    insertAirportQuery->bindValue(":has_tower_object", hasTower);

    int rating = atools::fs::util::calculateAirportRating(context.addOn, hasTower, numTaxiPath, numParking, numApron);
    insertAirportQuery->bindValue(":rating", rating);

    insertAirportQuery->bindValue(":num_parking_gate", numParkingGate);
    insertAirportQuery->bindValue(":num_parking_ga_ramp", numParkingGaRamp);
    insertAirportQuery->bindValue(":num_parking_cargo", numParkingCargo);
    insertAirportQuery->bindValue(":num_parking_mil_cargo", numParkingMilCargo);
    insertAirportQuery->bindValue(":num_parking_mil_combat", numParkingMilCombat);

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

    insertRunwayEndQuery->bindAndExecRecords(runwayEndRecords);
  }
  airportRect = Rect();
  airportPos = Pos();

  longestRunwayLength = longestRunwayWidth = longestRunwayHeading = 0;
  longestRunwaySurface = "UNKNOWN";
  numSoftRunway = numWaterRunway = numHardRunway = numHelipad = numLightRunway = 0;
  numParkingGate = numParkingGaRamp = numParkingCargo = numParkingMilCargo = numParkingMilCombat = 0;
  numCom = numStart = numRunwayEndVasi = numApron = numTaxiPath = numRunwayEndAls = numParking = 0;
  airportAltitude = 0.f;
  airportRowCode = NO_ROWCODE;
  airportIcao.clear();
  runwayEndRecords.clear();
  taxiNodes.clear();
  hasTower = false;

  writingAirport = ignoringAirport = false;
  writingPavementBoundary = writingPavementHoles = writingPavementNewHole = writingStartLocation = false;
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

  insertParkingQuery = new SqlQuery(db);
  insertParkingQuery->prepare(util.buildInsertStatement("parking", QString(), {"pushback"}));

  insertApronQuery = new SqlQuery(db);
  insertApronQuery->prepare(util.buildInsertStatement("apron", QString(), {"vertices", "vertices2", "triangles"}));

  insertTaxiQuery = new SqlQuery(db);
  insertTaxiQuery->prepare(util.buildInsertStatement("taxi_path", QString(), {"start_dir", "end_dir"}));

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

  delete insertParkingQuery;
  insertParkingQuery = nullptr;

  delete insertApronQuery;
  insertApronQuery = nullptr;

  delete insertTaxiQuery;
  insertTaxiQuery = nullptr;

  delete insertAirportFileQuery;
  insertAirportFileQuery = nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
