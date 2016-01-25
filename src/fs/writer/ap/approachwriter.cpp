/*
 * ApproachWriter.cpp
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/runwayindex.h"
#include "fs/writer/ap/transitionwriter.h"

#include <QString>

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Approach;
using atools::sql::SqlQuery;

void ApproachWriter::writeObject(const Approach *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Approach for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":approach_id", getNextId());
  bind(":type", bgl::util::enumToStr(Approach::approachTypeToStr, type->getType()));
  bind(":has_gps_overlay", type->isGpsOverlay());
  bind(":num_legs", type->getNumLegs());
  bind(":num_missed_legs", type->getNumMissedLegs());
  bind(":fix_type", bgl::util::enumToStr(Approach::approachFixTypeToStr, type->getFixType()));
  bind(":fix_ident", type->getFixIdent());
  bind(":fix_region", type->getFixRegion());
  bind(":fix_airport_ident", type->getFixAirportIdent());
  bind(":altitude", bgl::util::meterToFeet(type->getAltitude(), 1));
  bind(":heading", type->getHeading());
  bind(":missed_altitude", bgl::util::meterToFeet(type->getMissedAltitude(), 1));

  bool isComplete = false;
  const QString& apIdent = getDataWriter().getAirportWriter()->getCurrentAirportIdent();
  if(type->hasRunwayReference() && !apIdent.isEmpty())
  {
    if(getOptions().doesAirportIcaoMatch(apIdent))
    {
      QString msg(" approach ID " + QString::number(getCurrentId()));
      int id = getRunwayIndex()->getRunwayEndId(apIdent, type->getRunwayName(), msg);
      if(id != -1)
      {
        isComplete = true;
        bind(":runway_end_id", id);
      }
      else
        bind(":runway_end_id", QVariant(QVariant::Int));
    }
  }
  else
    isComplete = true;

  if(!getOptions().noIncompleteObjects() || isComplete)
  {
    executeStatement();

    TransitionWriter *appWriter = getDataWriter().getApproachTransWriter();
    appWriter->write(type->getTransitions());
  }
  else
    clearStatement();
}

} // namespace writer
} // namespace fs
} // namespace atools
