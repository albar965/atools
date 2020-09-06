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

#include "fs/pln/flightplan.h"

#include "geo/calculations.h"

namespace atools {
namespace fs {
namespace pln {

using atools::geo::Pos;

// =============================================================================================
void Flightplan::removeNoSaveEntries()
{
  auto it = std::remove_if(entries.begin(), entries.end(),
                           [](const FlightplanEntry& type) -> bool
        {
          return type.isNoSave();
        });

  if(it != entries.end())
    entries.erase(it, entries.end());
}

float Flightplan::getDistanceNm() const
{
  float distanceMeter = 0.f;
  if(entries.size() > 1)
  {
    for(int i = 0; i < entries.size() - 1; i++)
      distanceMeter += entries.at(i).getPosition().distanceMeterTo(entries.at(i + 1).getPosition());
  }
  return atools::geo::meterToNm(distanceMeter);
}

void Flightplan::clear()
{
  entries.clear();

  departureIdent.clear();
  destinationIdent.clear();
  departureParkingName.clear();
  departureName.clear();
  destinationName.clear();
  comment.clear();

  departurePos = Pos();
  destinationPos = Pos();

  lnmFormat = true;
  flightplanType = VFR;
  cruisingAlt = 10000;
}

void Flightplan::setDeparturePosition(const geo::Pos& value)
{
  departurePos = value;
}

void Flightplan::setDeparturePosition(const geo::Pos& value, float altitude)
{
  departurePos = value;
  departurePos.setAltitude(altitude);
}

QString Flightplan::getFilenamePattern(const QString& pattern, const QString& suffix, bool clean) const
{
  if(isEmpty())
    return tr("Empty Flightplan") + suffix;

  QString type;
  if(getFlightplanType() == atools::fs::pln::IFR)
    type = tr("IFR");
  else if(getFlightplanType() == atools::fs::pln::VFR)
    type = tr("VFR");

  QString departName = getDepartureName(), departIdent = getDepartureIdent(),
          destName = getDestinationName(), destIdent = getDestinationIdent();

  if(departName.isEmpty())
    departName = entries.first().getName();
  if(departIdent.isEmpty())
    departIdent = entries.first().getIdent();
  if(destName.isEmpty())
    destName = destinationAirport().getName();
  if(destIdent.isEmpty())
    destIdent = destinationAirport().getIdent();

  return getFilenamePattern(pattern, type, departName, departIdent, destName, destIdent, suffix, cruisingAlt, clean);
}

QString Flightplan::getFilenamePattern(QString pattern, const QString& type, const QString& departureName,
                                       const QString& departureIdent, const QString& destName, const QString& destIdent,
                                       const QString& suffix, int altitude, bool clean)
{
  if(pattern.isEmpty())
    pattern = pattern::SHORT;

  QString name = pattern.
                 replace(pattern::PLANTYPE, type.trimmed()).
                 replace(pattern::DEPARTNAME, departureName.simplified()).
                 replace(pattern::DEPARTIDENT, departureIdent.simplified()).
                 replace(pattern::DESTNAME, destName.simplified()).
                 replace(pattern::DESTIDENT, destIdent.simplified()).
                 replace(pattern::CRUISEALT, QString::number(altitude)) + suffix;

  return clean ? atools::cleanFilename(name) : name;
}

const atools::fs::pln::FlightplanEntry& Flightplan::destinationAirport() const
{
  static const FlightplanEntry EMPTY;

  for(int i = entries.size() - 1; i >= 0; i--)
  {
    if(!(entries.at(i).isNoSave()))
      return entries.at(i);
  }
  return EMPTY;
}

QDebug operator<<(QDebug out, const Flightplan& record)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Flightplan[ "
                          << ", " << record.getDepartureIdent()
                          << " -> " << record.getDestinationIdent()
                          << ", lnm format " << record.lnmFormat;

  int i = 1;
  for(const FlightplanEntry& entry : record.getEntries())
    out << endl << i++ << " " << entry;
  out << "]";
  return out;
}

} // namespace pln
} // namespace fs
} // namespace atools
