/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include <QCoreApplication>
#include <QFileInfo>

namespace atools {
namespace fs {
namespace pln {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

using atools::geo::Pos;

// =============================================================================================

float Flightplan::getDistanceNm() const
{
  float distanceMeter = 0.f;
  if(size() > 1)
  {
    for(int i = 0; i < size() - 1; i++)
      distanceMeter += at(i).getPosition().distanceMeterTo(at(i + 1).getPosition());
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

QString Flightplan::getDescr() const
{
  return QString("%1, %2 created by %3 %4").
         arg(departureIdent).arg(destinationIdent).arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
}

QString Flightplan::getFilenamePatternExample(const QString& pattern, const QString& suffix, bool html, QString *errorMessage)
{
  if(!pattern.isEmpty())
  {
    if(errorMessage != nullptr)
    {
      // Check pattern for errors
      QString patternSuffix = QFileInfo(pattern).suffix();
      QString defaultPatternSuffix = suffix.startsWith('.') ? suffix.mid(1) : suffix;

      if(patternSuffix.isEmpty())
        errorMessage->append(tr("File pattern has no extension. It should end with \".%1\".").arg(defaultPatternSuffix));
      else if(patternSuffix.compare(defaultPatternSuffix, Qt::CaseInsensitive) != 0)
        // Pattern differs to given extension
        errorMessage->append(tr("File pattern uses a wrong extension \".%1\". It should end with \".%2\".").
                             arg(patternSuffix).arg(defaultPatternSuffix));
    }

    // Build an example filename
    QString example = atools::fs::pln::Flightplan::getFilenamePattern(pattern, "IFR", "Frankfurt am Main", "EDDF",
                                                                      "Fiumicino", "LIRF", QString(), 30000);

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
    // Nothing given - return empty example
    if(errorMessage != nullptr)
      *errorMessage = tr("Pattern is empty.");
    return QString();
  }
}

void Flightplan::clearAll()
{
  clearEntries();
  clearProperties();

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
  cruiseAltitudeFt = 10000.f;
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

QString Flightplan::getFilenamePattern(const QString& pattern, const QString& suffix, bool metric) const
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
    departName = constFirst().getName();
  if(departIdent.isEmpty())
    departIdent = constFirst().getIdent();
  if(destName.isEmpty())
    destName = destinationAirport().getName();
  if(destIdent.isEmpty())
    destIdent = destinationAirport().getIdent();

  // Convert feet to metric altitude if needed
  int cruiseLocal = atools::roundToInt(metric ? atools::geo::feetToMeter(cruiseAltitudeFt) : cruiseAltitudeFt);
  return getFilenamePattern(pattern, type, departName, departIdent, destName, destIdent, suffix, cruiseLocal);
}

QString Flightplan::getFilenamePattern(QString pattern, const QString& type, const QString& departureName,
                                       const QString& departureIdent, const QString& destName, const QString& destIdent,
                                       const QString& suffix, int altitudeLocal)
{
  QString name = pattern.
                 replace(pattern::PLANTYPE, type.trimmed()).
                 replace(pattern::DEPARTNAME, departureName.simplified().mid(0, 30)).
                 replace(pattern::DEPARTIDENT, departureIdent.simplified()).
                 replace(pattern::DESTNAME, destName.simplified().mid(0, 30)).
                 replace(pattern::DESTIDENT, destIdent.simplified()).
                 replace(pattern::CRUISEALT, altitudeLocal > 0 ? QString::number(altitudeLocal) : QString()) + suffix;

  return atools::cleanFilename(name, 1000);
}

void Flightplan::adjustDepartureAndDestination(bool force)
{
  if(!isEmpty())
  {
    if(force || departureIdent.isEmpty())
      departureIdent = constFirst().getIdent();
    if(force || departureName.isEmpty())
      departureName = constFirst().getName();
    if(force || !departurePos.isValid())
      departurePos = constFirst().getPosition();

    if(force || destinationIdent.isEmpty())
      destinationIdent = constLast().getIdent();
    if(force || destinationName.isEmpty())
      destinationName = constLast().getName();

    if(force || !destinationPos.isValid())
      destinationPos = constLast().getPosition();
    // These remain empty
    // departureParkingName, departureAiportName, destinationAiportName, appVersionMajor, appVersionBuild;
  }
}

void Flightplan::assignAltitudeToAllEntries()
{
  for(FlightplanEntry& entry : *this)
    entry.setPosition(Pos(entry.getPosition().getLonX(), entry.getPosition().getLatY(), cruiseAltitudeFt));
}

atools::fs::pln::Flightplan Flightplan::compressedAirways() const
{
  atools::fs::pln::Flightplan plan(*this);

  if(!isEmpty())
  {
    plan.clear();

    for(int i = 0; i < size() - 1; i++)
    {
      const FlightplanEntry& entry = at(i);

      if(!entry.getAirway().isEmpty() && entry.getAirway() == at(i + 1).getAirway())
        // Skip if the next airway is the same
        continue;

      plan.append(entry);
    }

    // Add destination
    plan.append(constLast());
  }
  return plan;
}

QString Flightplan::toShortString() const
{
  QStringList str;
  for(const FlightplanEntry& entry : *this)
  {
    if(!entry.getAirway().isEmpty())
      str.append(QString("[%1]").arg(entry.getAirway()));
    str.append(entry.getIdent());
  }
  return str.join(' ');
}

QVector<const FlightplanEntry *> Flightplan::getAlternates() const
{
  QVector<const FlightplanEntry *> alternates;

  for(const FlightplanEntry& entry : *this)
  {
    if(entry.getFlags() & entry::ALTERNATE)
      alternates.append(&entry);
  }
  return alternates;
}

const atools::fs::pln::FlightplanEntry& Flightplan::destinationAirport() const
{
  static const FlightplanEntry EMPTY;

  for(int i = size() - 1; i >= 0; i--)
  {
    if(!(at(i).isAlternate()))
      return at(i);
  }
  return EMPTY;
}

QDebug operator<<(QDebug out, const Flightplan& record)
{
  QDebugStateSaver saver(out);

  out.noquote().nospace() << "Flightplan[ "
                          << ", " << record.getDepartureIdent()
                          << " -> " << record.getDestinationIdent()
                          << ", lnm format " << record.lnmFormat
                          << ", properties " << record.properties;

  int i = 1;
  for(const FlightplanEntry& entry : record)
    out << endl << i++ << " " << entry;
  out << "]";
  return out;
}

} // namespace pln
} // namespace fs
} // namespace atools
