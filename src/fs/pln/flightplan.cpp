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

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

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

QString Flightplan::getFlightplanTypeStr() const
{
  QString type;
  if(getFlightplanType() == atools::fs::pln::IFR)
    type = tr("IFR");
  else if(getFlightplanType() == atools::fs::pln::VFR)
    type = tr("VFR");
  return type;
}

QString Flightplan::getDepartureParkingTypeStr() const
{
  switch(departureParkingType)
  {
    case atools::fs::pln::NO_POS:
      return "None";

    case atools::fs::pln::AIRPORT:
      return "Airport";

    case atools::fs::pln::RUNWAY:
      return "Runway";

    case atools::fs::pln::PARKING:
      return "Parking";

    case atools::fs::pln::HELIPAD:
      return "Helipad";
  }
  return QString();
}

void Flightplan::setDepartureParkingType(QString type)
{
  type = type.toLower();
  if(type == "airport")
    departureParkingType = atools::fs::pln::AIRPORT;
  else if(type == "runway")
    departureParkingType = atools::fs::pln::RUNWAY;
  else if(type == "parking")
    departureParkingType = atools::fs::pln::PARKING;
  else if(type == "helipad")
    departureParkingType = atools::fs::pln::HELIPAD;
  else
    departureParkingType = atools::fs::pln::NO_POS;
}

QString Flightplan::getFilenamePatternExample(const QString& pattern, const QString& suffix, bool html, QString *errorMessage)
{
  if(!pattern.isEmpty())
  {
    // Build an example filename
    QString example = atools::fs::pln::Flightplan::getFilenamePattern(pattern, "IFR", "Frankfurt am Main", "EDDF",
                                                                      "Fiumicino", "LIRF", suffix, 30000, false /* clean */);

    // Clean name from invalid characters
    QString cleanExample = atools::cleanFilename(example, atools::MAX_FILENAME_CHARS);

    // Check if the cleaned filename differs from user input
    if(example != cleanExample && errorMessage != nullptr)
      *errorMessage = tr("Pattern contains invalid characters, double spaces or is longer than %1 characters.%2"
                         "Not allowed are: %3").
                      arg(atools::MAX_FILENAME_CHARS).
                      arg(html ? "<br/>" : "\n").
                      arg(atools::invalidFilenameCharacters(html));
    return cleanExample;
  }
  else
  {
    if(errorMessage != nullptr)
      *errorMessage = tr("Pattern is empty.");
    return QString();
  }
}

void Flightplan::clear()
{
  entries.clear();

  departureIdent.clear();
  destinationIdent.clear();
  departureParkingName.clear();
  departureParkingType = NO_POS;
  departureName.clear();
  destinationName.clear();
  comment.clear();

  departurePos = departureParkingPos = destinationPos = atools::geo::EMPTY_POS;
  departureParkingHeading = INVALID_HEADING;

  lnmFormat = true;
  flightplanType = VFR;
  routeType = DIRECT;
  cruisingAlt = 10000;
  properties.clear();
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

void Flightplan::setDepartureParkingPosition(const geo::Pos& value, float altitudeFt, float headingTrue)
{
  departureParkingPos = value;

  if(altitudeFt < INVALID_ALTITUDE)
    departureParkingPos.setAltitude(altitudeFt);

  departureParkingHeading = headingTrue;
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
    departName = entries.constFirst().getName();
  if(departIdent.isEmpty())
    departIdent = entries.constFirst().getIdent();
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
  QString name = pattern.
                 replace(pattern::PLANTYPE, type.trimmed()).
                 replace(pattern::DEPARTNAME, departureName.simplified()).
                 replace(pattern::DEPARTIDENT, departureIdent.simplified()).
                 replace(pattern::DESTNAME, destName.simplified()).
                 replace(pattern::DESTIDENT, destIdent.simplified()).
                 replace(pattern::CRUISEALT, QString::number(altitude)) + suffix;

  return clean ? atools::cleanFilename(name) : name;
}

void Flightplan::adjustDepartureAndDestination(bool force)
{
  if(!entries.isEmpty())
  {
    if(force || departureIdent.isEmpty())
      departureIdent = entries.constFirst().getIdent();
    if(force || departureName.isEmpty())
      departureName = entries.constFirst().getName();
    if(force || !departurePos.isValid())
      departurePos = entries.constFirst().getPosition();

    if(force || destinationIdent.isEmpty())
      destinationIdent = entries.constLast().getIdent();
    if(force || destinationName.isEmpty())
      destinationName = entries.constLast().getName();

    if(force || !destinationPos.isValid())
      destinationPos = entries.constLast().getPosition();
    // These remain empty
    // departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  }
}

void Flightplan::assignAltitudeToAllEntries()
{
  for(FlightplanEntry& entry : entries)
    entry.setPosition(Pos(entry.getPosition().getLonX(),
                          entry.getPosition().getLatY(), cruisingAlt));
}

atools::fs::pln::Flightplan Flightplan::compressedAirways() const
{
  atools::fs::pln::Flightplan plan(*this);

  if(!isEmpty())
  {
    plan.getEntries().clear();

    for(int i = 0; i < entries.size() - 1; i++)
    {
      const FlightplanEntry& entry = entries.at(i);

      if(!entry.getAirway().isEmpty() && entry.getAirway() == entries.at(i + 1).getAirway())
        // Skip if the next airway is the same
        continue;

      plan.getEntries().append(entry);
    }

    // Add destination
    plan.getEntries().append(entries.constLast());
  }
  return plan;
}

QString Flightplan::toShortString() const
{
  QStringList str;
  for(const FlightplanEntry& entry : entries)
  {
    if(!entry.getAirway().isEmpty())
      str.append(QString("[%1]").arg(entry.getAirway()));
    str.append(entry.getIdent());
  }
  return str.join(' ');
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
