/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_ONLINETYPES_H
#define ATOOLS_ONLINETYPES_H

#include <functional>
#include <QVector>

class QString;
class QStringList;

namespace atools {
namespace geo {
class LineString;
}
namespace fs {
namespace online {

enum Format
{
  UNKNOWN,
  VATSIM,
  IVAO
};

// enum ClientType
// Value  Description
// ATC  ATC or Observer connections
// PILOT  Pilot connections
// FOLME  Follow Me Car connections
QString clientType(const QString& type);

namespace fac {
enum FacilityType
{
  UNKNOWN = -1,
  OBSERVER = 0,
  FLIGHT_INFORMATION = 1,
  DELIVERY = 2,
  GROUND = 3,
  TOWER = 4,
  APPROACH = 5,
  ACC = 6,
  DEPARTURE = 7
};

}

namespace adm {
enum AdministrativeRating
{
  SUSPENDED = 0,
  OBSERVER = 1,
  USER = 2,
  SUPERVISOR = 11,
  ADMINISTRATOR = 12
};

}

namespace pilot {
enum PilotRating
{
  OBSERVER = 1,
  BASIC_FLIGHT_STUDENT_FS1 = 2,
  FLIGHT_STUDENT_FS2 = 3,
  ADVANCED_FLIGHT_STUDENT_FS3 = 4,
  PRIVATE_PILOT_PP = 5,
  SENIOR_PRIVATE_PILOT_SPP = 6,
  COMMERCIAL_PILOT_CP = 7,
  AIRLINE_TRANSPORT_PILOT_ATP = 8,
  SENIOR_FLIGHT_INSTRUCTOR_SFI = 9,
  CHIEF_FLIGHT_INSTRUCTOR_CFI = 10
};

}

namespace atc {
enum AtcRating
{
  OBSERVER = 1,
  ATC_APPLICANT_AS1 = 2,
  ATC_TRAINEE_AS2 = 3,
  ADVANCED_ATC_TRAINEE_AS3 = 4,
  AERODROME_CONTROLLER_ADC = 5,
  APPROACH_CONTROLLER_APC = 6,
  CENTER_CONTROLLER_ACC = 7,
  SENIOR_CONTROLLER_SEC = 8,
  SENIOR_ATC_INSTRUCTOR_SAI = 9,
  CHIEF_ATC_INSTRUCTOR_CAI = 10
};

}

namespace sim {
enum Simulator
{
  UNKNOWN = 0,
  MICROSOFT_FLIGHT_SIMULATOR_95 = 1,
  MICROSOFT_FLIGHT_SIMULATOR_98 = 2,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR = 3,
  MICROSOFT_FLIGHT_SIMULATOR_2000 = 4,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR_2 = 5,
  MICROSOFT_FLIGHT_SIMULATOR_2002 = 6,
  MICROSOFT_COMBAT_FLIGHT_SIMULATOR_3 = 7,
  MICROSOFT_FLIGHT_SIMULATOR_2004 = 8,
  MICROSOFT_FLIGHT_SIMULATOR_X = 9,
  XPLANE = 11,
  XPLANE_8X = 12,
  XPLANE_9X = 13,
  XPLANE_10X = 14,
  PS1 = 15,
  XPLANE_11X = 16,
  XPLANE_12X = 17,
  FLY = 20,
  FLY_2 = 21,
  FLIGHTGEAR = 25,
  PREPAR3D_1X = 30
};

}

/* Callback which tries to fetch geometry from the user airspace database.
 * Default circle will be used if this returns an empty byte array. */
typedef std::function<atools::geo::LineString *(const QString& callsign,
                                                atools::fs::online::fac::FacilityType type)> GeoCallbackType;

QString facilityTypeText(int type);
QString facilityTypeText(atools::fs::online::fac::FacilityType type);

/* Convert untranslated values to enum */
atools::fs::online::fac::FacilityType textToFacilityType(QString value);

/* Untranslated names*/
QString facilityTypeTextSettings(fac::FacilityType type);

const QVector<atools::fs::online::fac::FacilityType>& allFacilityTypes();

QString admRatingText(int rating);
QString admRatingText(atools::fs::online::adm::AdministrativeRating rating);

QString simulatorText(int simulator);
QString simulatorText(atools::fs::online::sim::Simulator simulator);

QString pilotRatingText(int rating);
QString pilotRatingText(atools::fs::online::pilot::PilotRating rating);
QString pilotRatingTextShort(atools::fs::online::pilot::PilotRating rating);

QString atcRatingText(int rating);
QString atcRatingText(atools::fs::online::atc::AtcRating rating);
QString atcRatingTextShort(atools::fs::online::atc::AtcRating rating);

} // namespace online
} // namespace fs
} // namespace atools

#endif // ATOOLS_ONLINETYPES_H
