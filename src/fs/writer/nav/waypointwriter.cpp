/*
 * WaypointWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "waypointwriter.h"
#include "temproutewriter.h"
#include "../meta/bglfilewriter.h"
#include "../datawriter.h"
#include "fs/writer/airportindex.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Waypoint;
using atools::sql::SqlQuery;

void WaypointWriter::writeObject(const Waypoint *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Waypoint " << type->getIdent();

  bind(":waypoint_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());
  bind(":region", type->getRegion());
  bind(":type", bgl::Waypoint::waypointTypeToStr(type->getType()));
  bind(":mag_var", type->getMagVar());
  bind(":lonx", type->getPosition().getLonX());
  bind(":laty", type->getPosition().getLatY());

  QString apIdent = type->getAirportIdent();
  if(!apIdent.isEmpty() && getOptions().doesAirportIcaoMatch(apIdent))
  {
    QString msg("Waypoint ID " + QString::number(getCurrentId()) + " ident " + type->getIdent());
    int id = getAirportIndex()->getAirportId(apIdent, msg);
    if(id != -1)
      bind(":airport_id", id);
    else
      bind(":airport_id", QVariant(QVariant::Int));
  }

  executeStatement();

  TempRouteWriter *tempRouteWriter = getDataWriter().getTempRouteWriter();
  tempRouteWriter->write(type->getRoutes());
}

} // namespace writer
} // namespace fs
} // namespace atools
