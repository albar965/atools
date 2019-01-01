/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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
  routeType = other.routeType;
  cruisingAlt = other.cruisingAlt;
  appVersionMajor = other.appVersionMajor;
  appVersionBuild = other.appVersionBuild;
  title = other.title;
  departureIdent = other.departureIdent;
  destinationIdent = other.destinationIdent;
  description = other.description;
  departureParkingName = other.departureParkingName;
  departureAiportName = other.departureAiportName;
  destinationAiportName = other.destinationAiportName;
  departurePos = other.departurePos;
  destinationPos = other.destinationPos;
  entries = other.entries;
  properties = other.properties;
  fileFormat = other.fileFormat;
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

  title.clear();
  departureIdent.clear();
  destinationIdent.clear();
  description.clear();
  departureParkingName.clear();
  departureAiportName.clear();
  destinationAiportName.clear();

  departurePos = Pos();
  destinationPos = Pos();

  flightplanType = VFR;
  fileFormat = PLN_FSX;
  routeType = DIRECT;
  cruisingAlt = 10000;

  appVersionBuild = APPVERSION_BUILD;
  appVersionMajor = APPVERSION_MAJOR;
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

void Flightplan::reverse()
{
  std::reverse(entries.begin(), entries.end());

  departureAiportName.swap(destinationAiportName);
  departureIdent.swap(destinationIdent);

  // Overwrite parking position with airport position
  departurePos = entries.first().getPosition();
  setDepartureParkingName(QString());
}

FileFormat Flightplan::getFileFormatBySuffix(const QString& file) const
{
  if(file.endsWith(".fms", Qt::CaseInsensitive))
    return FMS11;
  else if(file.endsWith(".flp", Qt::CaseInsensitive))
    return FLP;
  else
    return PLN_FSX;
}

void Flightplan::setFileFormatBySuffix(const QString& file)
{
  if(file.endsWith(".fms", Qt::CaseInsensitive))
    fileFormat = FMS11;
  else if(file.endsWith(".flp", Qt::CaseInsensitive))
    fileFormat = FLP;
  // else leave as is
}

bool Flightplan::canSaveAltitude() const
{
  // FS9 and FSC formats can be used here since it is always overwritten with FSX
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC || fileFormat == FMS11 ||
         fileFormat == FMS3;
}

bool Flightplan::canSaveFlightplanType() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC;
}

bool Flightplan::canSaveRouteType() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC;
}

bool Flightplan::canSaveSpeed() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC;
}

bool Flightplan::canSaveDepartureParking() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC;
}

bool Flightplan::canSaveUserWaypointName() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC;
}

bool Flightplan::canSaveAirways() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC || fileFormat == FLP ||
         fileFormat == FMS11;
}

bool Flightplan::canSaveProcedures() const
{
  return fileFormat == PLN_FSX || fileFormat == PLN_FS9 || fileFormat == PLN_FSC || fileFormat == FLP ||
         fileFormat == FMS11;
}

QDebug operator<<(QDebug out, const Flightplan& record)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Flightplan[fmt" << record.getFileFormat()
                          << ", from/to " << record.getDepartureIdent()
                          << " -> " << record.getDestinationIdent() << endl;

  int i = 1;
  for(const FlightplanEntry& entry : record.getEntries())
    out << i++ << entry;
  out << "]";
  return out;
}

} // namespace pln
} // namespace fs
} // namespace atools
