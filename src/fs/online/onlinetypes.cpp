/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#include "fs/online/onlinetypes.h"

#include <QObject>

namespace atools {
namespace fs {
namespace online {

QString clientType(const QString& type)
{
  if(type == "ATC")
    return QObject::tr("ATC");
  else if(type == "PILOT")
    return QObject::tr("Pilot");
  else if(type == "FOLME")
    return QObject::tr("Follow me car");
  else
    return type;
  // ATC  ATC or Observer connections
  // PILOT  Pilot connections
  // FOLME  Follow Me Car connections
}

QString facilityTypeText(int type)
{
  return facilityTypeText(static_cast<fac::FacilityType>(type));
}

QString facilityTypeText(fac::FacilityType type)
{
  switch(type)
  {
    case atools::fs::online::fac::OBSERVER:
      return QObject::tr("Observer");

    case atools::fs::online::fac::FLIGHT_INFORMATION:
      return QObject::tr("FIR");

    case atools::fs::online::fac::DELIVERY:
      return QObject::tr("Delivery");

    case atools::fs::online::fac::GROUND:
      return QObject::tr("Ground");

    case atools::fs::online::fac::TOWER:
      return QObject::tr("Tower");

    case atools::fs::online::fac::APPROACH:
      return QObject::tr("Approach");

    case atools::fs::online::fac::ACC:
      return QObject::tr("ACC");

    case atools::fs::online::fac::DEPARTURE:
      return QObject::tr("Departure");
  }
  return QString::number(type);
}

QString admRatingText(int rating)
{
  return admRatingText(static_cast<adm::AdministrativeRating>(rating));
}

QString admRatingText(adm::AdministrativeRating rating)
{
  switch(rating)
  {
    case atools::fs::online::adm::SUSPENDED:
      return QObject::tr("Suspended");

    case atools::fs::online::adm::OBSERVER:
      return QObject::tr("Observer");

    case atools::fs::online::adm::USER:
      return QObject::tr("User");

    case atools::fs::online::adm::SUPERVISOR:
      return QObject::tr("Supervisor");

    case atools::fs::online::adm::ADMINISTRATOR:
      return QObject::tr("Administrator");
  }
  return QString::number(rating);
}

QString pilotRatingText(atools::fs::online::pilot::PilotRating rating)
{
  switch(rating)
  {
    case atools::fs::online::pilot::OBSERVER:
      return QObject::tr("Observer");

    case atools::fs::online::pilot::BASIC_FLIGHT_STUDENT_FS1:
      return QObject::tr("Basic flight student (FS1)");

    case atools::fs::online::pilot::FLIGHT_STUDENT_FS2:
      return QObject::tr("Flight student (FS2)");

    case atools::fs::online::pilot::ADVANCED_FLIGHT_STUDENT_FS3:
      return QObject::tr("Advanced flight student (FS3)");

    case atools::fs::online::pilot::PRIVATE_PILOT_PP:
      return QObject::tr("Private pilot (PP)");

    case atools::fs::online::pilot::SENIOR_PRIVATE_PILOT_SPP:
      return QObject::tr("Senior private pilot (SPP)");

    case atools::fs::online::pilot::COMMERCIAL_PILOT_CP:
      return QObject::tr("Commercial pilot (CP)");

    case atools::fs::online::pilot::AIRLINE_TRANSPORT_PILOT_ATP:
      return QObject::tr("Airline transport pilot (ATP)");

    case atools::fs::online::pilot::SENIOR_FLIGHT_INSTRUCTOR_SFI:
      return QObject::tr("Senior flight instructor (SFI)");

    case atools::fs::online::pilot::CHIEF_FLIGHT_INSTRUCTOR_CFI:
      return QObject::tr("Chief flight instructor (CFI)");
  }
  return QString::number(rating);
}

QString pilotRatingTextShort(atools::fs::online::pilot::PilotRating rating)
{
  switch(rating)
  {
    case atools::fs::online::pilot::OBSERVER:
      return QObject::tr("OBS");

    case atools::fs::online::pilot::BASIC_FLIGHT_STUDENT_FS1:
      return QObject::tr("FS1");

    case atools::fs::online::pilot::FLIGHT_STUDENT_FS2:
      return QObject::tr("FS2");

    case atools::fs::online::pilot::ADVANCED_FLIGHT_STUDENT_FS3:
      return QObject::tr("FS3");

    case atools::fs::online::pilot::PRIVATE_PILOT_PP:
      return QObject::tr("PP");

    case atools::fs::online::pilot::SENIOR_PRIVATE_PILOT_SPP:
      return QObject::tr("SPP");

    case atools::fs::online::pilot::COMMERCIAL_PILOT_CP:
      return QObject::tr("CP");

    case atools::fs::online::pilot::AIRLINE_TRANSPORT_PILOT_ATP:
      return QObject::tr("ATP");

    case atools::fs::online::pilot::SENIOR_FLIGHT_INSTRUCTOR_SFI:
      return QObject::tr("SFI");

    case atools::fs::online::pilot::CHIEF_FLIGHT_INSTRUCTOR_CFI:
      return QObject::tr("CFI");
  }
  return QString::number(rating);
}

QString atcRatingText(int rating)
{
  return atcRatingText(static_cast<atc::AtcRating>(rating));
}

QString atcRatingText(atc::AtcRating rating)
{
  switch(rating)
  {
    case atools::fs::online::atc::OBSERVER:
      return QObject::tr("Observer");

    case atools::fs::online::atc::ATC_APPLICANT_AS1:
      return QObject::tr("ATC Applicant (AS1)");

    case atools::fs::online::atc::ATC_TRAINEE_AS2:
      return QObject::tr("ATC Trainee (AS2)");

    case atools::fs::online::atc::ADVANCED_ATC_TRAINEE_AS3:
      return QObject::tr("Advanced ATC Trainee (AS3)");

    case atools::fs::online::atc::AERODROME_CONTROLLER_ADC:
      return QObject::tr("Aerodrome Controller (ADC)");

    case atools::fs::online::atc::APPROACH_CONTROLLER_APC:
      return QObject::tr("Approach Controller (APC)");

    case atools::fs::online::atc::CENTER_CONTROLLER_ACC:
      return QObject::tr("Center Controller (ACC)");

    case atools::fs::online::atc::SENIOR_CONTROLLER_SEC:
      return QObject::tr("Senior Controller (SEC)");

    case atools::fs::online::atc::SENIOR_ATC_INSTRUCTOR_SAI:
      return QObject::tr("Senior ATC Instructor (SAI)");

    case atools::fs::online::atc::CHIEF_ATC_INSTRUCTOR_CAI:
      return QObject::tr("Chief ATC Instructor (CAI)");
  }
  return QString::number(rating);
}

QString atcRatingTextShort(atc::AtcRating rating)
{
  switch(rating)
  {
    case atools::fs::online::atc::OBSERVER:
      return QObject::tr("Observer");

    case atools::fs::online::atc::ATC_APPLICANT_AS1:
      return QObject::tr("AS1");

    case atools::fs::online::atc::ATC_TRAINEE_AS2:
      return QObject::tr("AS2");

    case atools::fs::online::atc::ADVANCED_ATC_TRAINEE_AS3:
      return QObject::tr("AS3");

    case atools::fs::online::atc::AERODROME_CONTROLLER_ADC:
      return QObject::tr("ADC");

    case atools::fs::online::atc::APPROACH_CONTROLLER_APC:
      return QObject::tr("APC");

    case atools::fs::online::atc::CENTER_CONTROLLER_ACC:
      return QObject::tr("ACC");

    case atools::fs::online::atc::SENIOR_CONTROLLER_SEC:
      return QObject::tr("SEC");

    case atools::fs::online::atc::SENIOR_ATC_INSTRUCTOR_SAI:
      return QObject::tr("SAI");

    case atools::fs::online::atc::CHIEF_ATC_INSTRUCTOR_CAI:
      return QObject::tr("CAI");
  }
  return QString::number(rating);
}

QString simulatorText(int simulator)
{
  return simulatorText(static_cast<sim::Simulator>(simulator));
}

QString simulatorText(sim::Simulator simulator)
{
  switch(simulator)
  {
    case atools::fs::online::sim::UNKNOWN:
      return QObject::tr("Unknown");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_95:
      return QObject::tr("FS95");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_98:
      return QObject::tr("FS98");

    case atools::fs::online::sim::MICROSOFT_COMBAT_FLIGHT_SIMULATOR:
      return QObject::tr("CFS");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_2000:
      return QObject::tr("FS2000");

    case atools::fs::online::sim::MICROSOFT_COMBAT_FLIGHT_SIMULATOR_2:
      return QObject::tr("CFS2");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_2002:
      return QObject::tr("FS2002");

    case atools::fs::online::sim::MICROSOFT_COMBAT_FLIGHT_SIMULATOR_3:
      return QObject::tr("CFS3");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_2004:
      return QObject::tr("FS2004");

    case atools::fs::online::sim::MICROSOFT_FLIGHT_SIMULATOR_X:
      return QObject::tr("FSX");

    case atools::fs::online::sim::XPLANE:
      return QObject::tr("X-Plane");

    case atools::fs::online::sim::XPLANE_8X:
      return QObject::tr("X-Plane 8");

    case atools::fs::online::sim::XPLANE_9X:
      return QObject::tr("X-Plane 9");

    case atools::fs::online::sim::XPLANE_10X:
      return QObject::tr("X-Plane 10");

    case atools::fs::online::sim::PS1:
      return QObject::tr("PS1");

    case atools::fs::online::sim::XPLANE_11X:
      return QObject::tr("X-Plane 11");

    case atools::fs::online::sim::XPLANE_12X:
      return QObject::tr("X-Plane 12");

    case atools::fs::online::sim::FLY:
      return QObject::tr("Fly");

    case atools::fs::online::sim::FLY_2:
      return QObject::tr("Fly 2");

    case atools::fs::online::sim::FLIGHTGEAR:
      return QObject::tr("FlightGear");

    case atools::fs::online::sim::PREPAR3D_1X:
      return QObject::tr("P3D");
  }
  return QString::number(simulator);
}

} // namespace online
} // namespace fs
} // namespace atools
