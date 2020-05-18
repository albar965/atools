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
Flightplan::Flightplan()
{

}

Flightplan::Flightplan(const Flightplan& other)
{
  this->operator=(other);

}

Flightplan::~Flightplan()
{

}

Flightplan& Flightplan::operator=(const Flightplan& other)
{
  flightplanType = other.flightplanType;
  cruisingAlt = other.cruisingAlt;
  departureIdent = other.departureIdent;
  destinationIdent = other.destinationIdent;
  departureParkingName = other.departureParkingName;
  departureName = other.departureName;
  destinationName = other.destinationName;
  departurePos = other.departurePos;
  destinationPos = other.destinationPos;
  entries = other.entries;
  properties = other.properties;
  comment = other.comment;
  lnmFormat = other.lnmFormat;
  return *this;
}

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

QString Flightplan::getFilenameLong(const QString& extension, const QString& suffix) const
{
  QString filename;

  if(isEmpty())
    return tr("Empty Flightplan") + suffix;

  if(getFlightplanType() == atools::fs::pln::IFR)
    filename = "IFR ";
  else if(getFlightplanType() == atools::fs::pln::VFR)
    filename = "VFR ";

  if(getDepartureName().isEmpty())
    filename += getEntries().first().getIdent();
  else
    filename += getDepartureName() + " (" + getDepartureIdent() + ")";

  filename += " to ";

  if(getDestinationName().isEmpty())
    filename += destinationAirportIdent();
  else
    filename += getDestinationName() + " (" + getDestinationIdent() + ")";

  filename += extension;
  filename += suffix;

  // Remove characters that are note allowed in most filesystems
  filename = atools::cleanFilename(filename);
  return filename;
}

QString Flightplan::getFilenameShort(const QString& sep, const QString& suffix) const
{
  QString filename;

  if(isEmpty())
    return tr("Empty") + sep + tr("Plan") + suffix;

  filename += getEntries().first().getIdent();
  filename += sep;

  filename += destinationAirportIdent();
  filename += suffix;

  // Remove characters that are note allowed in most filesystems
  filename = atools::cleanFilename(filename);
  return filename;
}

QString Flightplan::destinationAirportIdent() const
{
  for(int i = entries.size() - 1; i >= 0; i--)
  {
    if(!(entries.at(i).isNoSave()))
      return entries.at(i).getIdent();
  }

  return tr("Unknown");
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
