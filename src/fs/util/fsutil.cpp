/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "fs/util/fsutil.h"

#include "atools.h"
#include "geo/calculations.h"
#include "geo/linestring.h"
#include "geo/pos.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSet>
#include <QStringBuilder>

namespace atools {
namespace fs {
namespace util {

// Closed airport by name
const static QRegularExpression REGEXP_CLOSED(QLatin1String("(\\[X\\]|\\bCLSD\\b|\\bCLOSED\\b)"));
const static QRegularExpression REGEXP_DIGIT("\\d");
const static QRegularExpression REGEXP_WAYPOINT_DME("(\\w+) \\((\\w+) ([\\d\\.]+) DME\\)");
const static QRegularExpression REGEXP_WHITESPACE("\\s");

/* ICAO speed and altitude matches */
const static QRegularExpression REGEXP_SPDALT("^([NMK])(\\d{2,4})(([FSAM])(\\d{2,4}))?$");
const static QRegularExpression REGEXP_SPDALT_ALL("^([NMK])(\\d{3,4})([FSAM])(\\d{3,4})$");

// Look for longer military designators - taken from MSFS
static const QStringList CONTAINS_MIL({
        "HEERESFLUGPLATZ", // DE MSFS 2024
        "FLIEGERHORST", // DE MSFS 2024
        "MILITÄR", // de-DE.locPak:
        "BASE AÉREA", // es-ES.locPak:
        "BASE AÉRIENNE", // fr-FR.locPak:
        "BASE AEREA", // it-IT.locPak:
        // "BASE AEREAS", // it-IT.locPak:
        "BAZA LOTNICZA", // pl-PL.locPak:
        "FORÇA AÉREA"
        "MILITAR"
        "MILITAIRE",
        "MILITÆRE",
        "BASE AÉRIENNE",
        "MILITÄRFLYGBAS",
        "BASE AÉREA" // pt-BR.locPak:
      });

// Look for military designator words - if an airport name matches
// one of these patterns it will be designated as "military"
static const QList<QRegularExpression> REGEXP_MIL({
        QRegularExpression(QLatin1String("(\\[M\\]|\\[MIL\\])")), // X-Plane special
        QRegularExpression(QLatin1String("\\bAAC\\b")),
        QRegularExpression(QLatin1String("\\bAAF\\b")),
        QRegularExpression(QLatin1String("\\bAB\\b")),
        QRegularExpression(QLatin1String("\\bAFB\\b")),
        QRegularExpression(QLatin1String("\\bAFLD\\b")),
        QRegularExpression(QLatin1String("\\bAFS\\b")),
        QRegularExpression(QLatin1String("\\bAF\\b")),
        QRegularExpression(QLatin1String("\\bAHP\\b")),
        QRegularExpression(QLatin1String("\\bAIR BASE\\b")),
        QRegularExpression(QLatin1String("\\bAIR FORCE\\b")),
        QRegularExpression(QLatin1String("\\bAIRBASE\\b")),
        QRegularExpression(QLatin1String("\\bANGB\\b")),
        QRegularExpression(QLatin1String("\\bARB\\b")),
        QRegularExpression(QLatin1String("\\bARMY\\b")),
        QRegularExpression(QLatin1String("\\bCFB\\b")),
        QRegularExpression(QLatin1String("\\bLRRS\\b")),
        QRegularExpression(QLatin1String("\\bMCAF\\b")),
        QRegularExpression(QLatin1String("\\bMCALF\\b")),
        QRegularExpression(QLatin1String("\\bMCAS\\b")),
        QRegularExpression(QLatin1String("\\bMILITARY\\b")),
        QRegularExpression(QLatin1String("\\bMIL\\b")),
        QRegularExpression(QLatin1String("\\bNAF\\b")),
        QRegularExpression(QLatin1String("\\bNALF\\b")),
        QRegularExpression(QLatin1String("\\bNAS\\b")),
        QRegularExpression(QLatin1String("\\bNAVAL\\b")),
        QRegularExpression(QLatin1String("\\bNAVY\\b")),
        QRegularExpression(QLatin1String("\\bNAWS\\b")),
        QRegularExpression(QLatin1String("\\bNOLF\\b")),
        QRegularExpression(QLatin1String("\\bNSB\\b")),
        QRegularExpression(QLatin1String("\\bNSF\\b")),
        QRegularExpression(QLatin1String("\\bNSWC\\b")),
        QRegularExpression(QLatin1String("\\bNSY\\b")),
        QRegularExpression(QLatin1String("\\bNS\\b")),
        QRegularExpression(QLatin1String("\\bNWS\\b")),
        QRegularExpression(QLatin1String("\\bPMRF\\b")),
        QRegularExpression(QLatin1String("\\bRAF\\b")),
        QRegularExpression(QLatin1String("\\bRNAS\\b")),
        QRegularExpression(QLatin1String("\\bROYAL MARINES\\b")),
      });

QString capAirportName(const QString& str)
{
  // Force acronyms in airports to upper case
  const static QSet<QString> FORCE_UPPER({
          "AAC", "AAF", "AB", "AS", "ABMS", "AF", "AFB", "AFLD", "AFS", "AHP", "ANGB", "APCM", "ARB", "CFB", "CGS", "DGAC",
          "FAA", "FBO", "GTS", "HSC", "LRRS", "MAF", "MCAF", "MCALF", "MCAS", "NAF", "NALF", "NAS", "NAWS", "NFK",
          "NOLF", "NRC", "NRC", "NS", "NSB", "NSF", "NSWC", "NSY", "NWS", "PMRF", "RAF", "RBMU", "RLA", "RNAS",
          "USFS", "CGAS", "TV", "NVC", "USAF",
          "I", "II", "III", "IV", "V", "VI"});

  return atools::capString(str, FORCE_UPPER).replace("-O-", "-o-").replace("-N-", "-n-").replace("-A-", "-a-").trimmed();
}

/* from https://en.wikipedia.org/wiki/List_of_aircraft_type_designators */
static const QHash<QString, QString> NAME_CODE_MAP({
        {"A3ST", "Airbus A300-600ST"},
        {"A19N", "Airbus A319neo"},
        {"A20N", "Airbus A320neo"},
        {"A21N", "Airbus A321neo"},
        {"A30B", "Airbus A300B2, A300B4 and A300C4"},
        {"A35K", "Airbus A350-1000"},
        {"A124", "Antonov An-124 Ruslan"},
        {"A140", "Antonov An-140"},
        {"A148", "Antonov An-148"},
        {"A158", "Antonov An-158"},
        {"A225", "Antonov An-225 Mriya"},
        {"A306", "Airbus A300-600"},
        {"A306", "Airbus A300-600 Freighter[4]"},
        {"A310", "Airbus A310-200"},
        {"A310", "Airbus A310-300"},
        {"A318", "Airbus A318"},
        {"A318", "Airbus A318 (sharklets)"},
        {"A319", "Airbus A319"},
        {"A319", "Airbus A319 (sharklets)"},
        {"A320", "Airbus A320"},
        {"A320", "Airbus A320 (sharklets)"},
        {"A321", "Airbus A321"},
        {"A321", "Airbus A321 (sharklets)"},
        {"A332", "Airbus A330-200"},
        {"A332", "Airbus A330-200 Freighter"},
        {"A333", "Airbus A330-300"},
        {"A333", "Airbus A330-300 Freighter"},
        {"A337", "Airbus A330-700 BelugaXL"},
        {"A338", "Airbus A330-800"},
        {"A339", "Airbus A330-900"},
        {"A342", "Airbus A340-200"},
        {"A343", "Airbus A340-300"},
        {"A345", "Airbus A340-500"},
        {"A346", "Airbus A340-600"},
        {"A359", "Airbus A350-900"},
        {"A388", "Airbus A380-800"},
        {"A400", "Airbus A400M Atlas"},
        {"A748", "Hawker Siddeley HS 748"},
        {"AC90", "Gulfstream/Rockwell (Aero) Turbo Commander 690"},
        {"AJ27", "COMAC ARJ21"},
        {"AN12", "Antonov An-12"},
        {"AN24", "Antonov An-24"},
        {"AN26", "Antonov An-26"},
        {"AN28", "Antonov An-28"},
        {"AN30", "Antonov An-30"},
        {"AN32", "Antonov An-32"},
        {"AN72", "Antonov An-72 / An-74"},
        {"AT43", "Aerospatiale/Alenia ATR 42-300 / 320"},
        {"AT45", "Aerospatiale/Alenia ATR 42-500"},
        {"AT46", "Aerospatiale/Alenia ATR 42-600"},
        {"AT72", "Aerospatiale/Alenia ATR 72-201/-202"},
        {"AT73", "Aerospatiale/Alenia ATR 72-211/-212"},
        {"AT75", "Aerospatiale/Alenia ATR 72-212A (500)"},
        {"AT76", "Aerospatiale/Alenia ATR 72-212A (600)"},
        {"ATP", "British Aerospace ATP"},
        {"B3XM", "Boeing 737 MAX 10"},
        {"B37M", "Boeing 737 MAX 7"},
        {"B38M", "Boeing 737 MAX 8"},
        {"B39M", "Boeing 737 MAX 9"},
        {"B52", "Boeing B-52 Stratofortress"},
        {"B74R", "Boeing 747SR"},
        {"B74R", "Boeing 747SR Freighter"},
        {"B74S", "Boeing 747SP"},
        {"B77L", "Boeing 777-200 Freighter"},
        {"B77L", "Boeing 777-200LR"},
        {"B77W", "Boeing 777-300ER"},
        {"B78X", "Boeing 787-10"},
        {"B190", "Beechcraft 1900"},
        {"B461", "BAe 146-100"},
        {"B462", "BAe 146-200"},
        {"B463", "BAe 146-300"},
        {"B703", "Boeing 707"},
        {"B712", "Boeing 717"},
        {"B720", "Boeing 720B"},
        {"B721", "Boeing 727-100"},
        {"B722", "Boeing 727-200"},
        {"B732", "Boeing 737-200"},
        {"B732", "Boeing 737-200 Freighter"},
        {"B733", "Boeing 737-300"},
        {"B733", "Boeing 737-300 Freighter"},
        {"B733", "Boeing 737-300 Winglets"},
        {"B734", "Boeing 737-400"},
        {"B734", "Boeing 737-400 Freighter"},
        {"B735", "Boeing 737-500"},
        {"B735", "Boeing 737-500 Winglets"},
        {"B736", "Boeing 737-600"},
        {"B737", "Boeing 737-700 / Boeing 737-700ER"},
        {"B737", "Boeing 737-700 Winglets"},
        {"B738", "Boeing 737-800"},
        {"B738", "Boeing 737-800 Freighter"},
        {"B738", "Boeing 737-800 Freighter Winglets"},
        {"B738", "Boeing 737-800 Winglets"},
        {"B739", "Boeing 737-900 / Boeing 737-900ER"},
        {"B739", "Boeing 737-900 Winglets"},
        {"B741", "Boeing 747-100"},
        {"B741", "Boeing 747-100 Freighter"},
        {"B742", "Boeing 747-200"},
        {"B742", "Boeing 747-200F"},
        {"B742", "Boeing 747-200M"},
        {"B743", "Boeing 747-300"},
        {"B743", "Boeing 747-300M"},
        {"B744", "Boeing 747-400 / Boeing 747-400ER"},
        {"B744", "Boeing 747-400F / Boeing 747-400ERF"},
        {"B744", "Boeing 747-400M"},
        {"B748", "Boeing 747-8F"},
        {"B748", "Boeing 747-8I"},
        {"B752", "Boeing 757-200"},
        {"B752", "Boeing 757F"},
        {"B753", "Boeing 757-300"},
        {"B762", "Boeing 767-200 / Boeing 767-200ER"},
        {"B762", "Boeing 767-200 Freighter / Boeing 767-200ER"},
        {"B763", "Boeing 767-300"},
        {"B763", "Boeing 767-300 Freighter"},
        {"B763", "Boeing 767-300 Winglets / Boeing 767-300ER"},
        {"B764", "Boeing 767-400ER"},
        {"B772", "Boeing 777-200 / Boeing 777-200ER"},
        {"B773", "Boeing 777-300"},
        {"B778", "Boeing 777-8"},
        {"B779", "Boeing 777-9"},
        {"B788", "Boeing 787-8"},
        {"B789", "Boeing 787-9"},
        {"BA11", "British Aerospace (BAC) One Eleven"},
        {"BCS1", "Airbus A220-100"},
        {"BCS3", "Airbus A220-300"},
        {"BE20", "Beechcraft (Super) King Air 200"},
        {"BE40", "Hawker 400"},
        {"BE99", "Beechcraft Model 99"},
        {"BELF", "Shorts SC-5 Belfast"},
        {"BER2", "Beriev Be-200 Altair"},
        {"BLCF", "Boeing 747-400 LCF Dreamlifter"},
        {"C5M", "Lockheed C-5M Super Galaxy"},
        {"C25A", "Cessna Citation CJ2"},
        {"C25B", "Cessna Citation CJ3"},
        {"C25C", "Cessna Citation CJ4"},
        {"C30J", "Lockheed Martin C-130J Hercules"},
        {"C56X", "Cessna Citation Excel"},
        {"C130", "Lockheed L-182 / 282 / 382 (L-100) Hercules"},
        {"C212", "CASA / IPTN 212 Aviocar"},
        {"C500", "Cessna Citation I"},
        {"C510", "Cessna Citation Mustang"},
        {"C525", "Cessna CitationJet"},
        {"C550", "Cessna Citation II"},
        {"C560", "Cessna Citation V"},
        {"C650", "Cessna Citation III, VI, VII"},
        {"C680", "Cessna Citation Sovereign"},
        {"C700", "Cessna Citation Longitude"},
        {"C750", "Cessna Citation X"},
        {"C919", "COMAC C919"},
        {"CL2T", "Bombardier 415"},
        {"CL30", "Bombardier BD-100 Challenger 300"},
        {"CL60", "Canadair Challenger 600"},
        {"CN35", "CASA/IPTN CN-235"},
        {"CRJ1", "Canadair Regional Jet 100"},
        {"CRJ2", "Canadair Regional Jet 200"},
        {"CRJ7", "Canadair Regional Jet 700 | Regional Jet 550"},
        {"CRJ9", "Canadair Regional Jet 900"},
        {"CRJX", "Canadair Regional Jet 1000"},
        {"CVLT", "Convair CV-580, Convair CV-600, Convair CV-640"},
        {"D228", "Dornier 228"},
        {"D328", "Fairchild Dornier Do.328"},
        {"DC10", "Douglas DC-10-10 / -15 Passenger"},
        {"DC10", "Douglas DC-10-10 Freighter"},
        {"DC10", "Douglas DC-10-30 / -40 Freighter"},
        {"DC10", "Douglas DC-10-30 / -40 Passenger"},
        {"DC10", "Douglas DC-10-30 Combi"},
        {"DC85", "Douglas DC-8-50"},
        {"DC86", "Douglas DC-8-62"},
        {"DC87", "Douglas DC-8-72"},
        {"DC91", "Douglas DC-9-10"},
        {"DC92", "Douglas DC-9-20"},
        {"DC93", "Douglas DC-9-30"},
        {"DC94", "Douglas DC-9-40"},
        {"DC95", "Douglas DC-9-50"},
        {"DH8A", "De Havilland Canada DHC-8-100 Dash 8 / 8Q"},
        {"DH8B", "De Havilland Canada DHC-8-200 Dash 8 / 8Q"},
        {"DH8C", "De Havilland Canada DHC-8-300 Dash 8 / 8Q"},
        {"DH8D", "De Havilland Canada DHC-8-400 Dash 8Q"},
        {"DHC5", "De Havilland Canada DHC-5 Buffalo"},
        {"DHC6", "De Havilland Canada DHC-6 Twin Otter"},
        {"DHC7", "De Havilland Canada DHC-7 Dash 7"},
        {"E35L", "Embraer Legacy 600 / Legacy 650"},
        {"E50P", "Embraer Phenom 100"},
        {"E55P", "Embraer Phenom 300"},
        {"E75L", "Embraer 175 (long wing)"},
        {"E75S", "Embraer 175 (short wing)"},
        {"E110", "Embraer EMB 110 Bandeirante"},
        {"E120", "Embraer EMB 120 Brasilia"},
        {"E135", "Embraer RJ135"},
        {"E135", "Embraer RJ140"},
        {"E145", "Embraer RJ145"},
        {"E170", "Embraer 170"},
        {"E190", "Embraer 190 / Lineage 1000"},
        {"E195", "Embraer 195"},
        {"E290", "Embraer E190-E2"},
        {"E295", "Embraer E195-E2"},
        {"E545", "Embraer Legacy 450 / Praetor 500"},
        {"E550", "Embraer Legacy 500 / Praetor 600"},
        {"EA50", "Eclipse 500"},
        {"F2TH", "Dassault Falcon 2000"},
        {"F27", "Fokker F27 Friendship"},
        {"F28", "Fokker F28 Fellowship"},
        {"F50", "Fokker 50"},
        {"F70", "Fokker 70"},
        {"F100", "Fokker 100"},
        {"F406", "Reims-Cessna F406 Caravan II"},
        {"F900", "Dassault Falcon 900"},
        {"FA7X", "Dassault Falcon 7X"},
        {"FA50", "Dassault Falcon 50"},
        {"G73T", "Grumman G-73 Turbo Mallard"},
        {"G159", "Gulfstream Aerospace G-159 Gulfstream I"},
        {"G280", "Gulfstream G280"},
        {"GL5T", "Bombardier BD-700 Global 5000"},
        {"GLEX", "Bombardier Global Express / Raytheon Sentinel"},
        {"GLF4", "Gulfstream IV"},
        {"GLF5", "Gulfstream V"},
        {"GLF6", "Gulfstream G650"},
        {"H25B", "British Aerospace 125 series / Hawker/Raytheon 700/800/800XP/850/900"},
        {"H25C", "British Aerospace 125-1000 series / Hawker/Raytheon 1000"},
        {"HDJT", "Honda HA-420"},
        {"I114", "Ilyushin Il-114"},
        {"IL18", "Ilyushin Il-18"},
        {"IL62", "Ilyushin Il-62"},
        {"IL76", "Ilyushin Il-76"},
        {"IL86", "Ilyushin Il-86"},
        {"IL96", "Ilyushin Il-96"},
        {"J328", "Fairchild Dornier 328JET"},
        {"JS31", "British Aerospace Jetstream 31"},
        {"JS32", "British Aerospace Jetstream 32"},
        {"JS41", "British Aerospace Jetstream 41"},
        {"K35R", "Boeing KC-135 Stratotanker"},
        {"L101", "Lockheed L-1011 Tristar"},
        {"L188", "Lockheed L-188 Electra"},
        {"L410", "LET 410"},
        {"LJ35", "Learjet 35 / 36 / C-21A"},
        {"LJ60", "Learjet 60"},
        {"MD11", "McDonnell Douglas MD-11"},
        {"MD11", "McDonnell Douglas MD-11C"},
        {"MD11", "McDonnell Douglas MD-11F"},
        {"MD81", "McDonnell Douglas MD-81"},
        {"MD82", "McDonnell Douglas MD-82"},
        {"MD83", "McDonnell Douglas MD-83"},
        {"MD87", "McDonnell Douglas MD-87"},
        {"MD88", "McDonnell Douglas MD-88"},
        {"MD90", "McDonnell Douglas MD-90"},
        {"MU2", "Mitsubishi Mu-2"},
        {"N262", "Aerospatiale (Nord) 262"},
        {"NOMA", "Government Aircraft Factories N22B / N24A Nomad"},
        {"P8", "Boeing P-8 Poseidon"},
        {"P180", "Piaggio P.180 Avanti"},
        {"PAY2", "Piper Cheyenne 2"},
        {"PC24", "Pilatus PC-24"},
        {"RJ1H", "Avro RJ100"},
        {"RJ70", "Avro RJ70"},
        {"RJ85", "Avro RJ85"},
        {"S601", "Aerospatiale SN.601 Corvette"},
        {"SB20", "Saab 2000"},
        {"SC7", "Shorts SC-7 Skyvan"},
        {"SF34", "Saab SF340A/B"},
        {"SH33", "Shorts SD.330"},
        {"SH36", "Shorts SD.360"},
        {"SU95", "Sukhoi Superjet 100-95"},
        {"SW4", "Fairchild Swearingen Metroliner"},
        {"T134", "Tupolev Tu-134"},
        {"T154", "Tupolev Tu-154"},
        {"T204", "Tupolev Tu-204 / Tu-214"},
        {"WW24", "Israel Aircraft Industries 1124 Westwind"},
        {"Y12", "Harbin Y-12"},
        {"YK40", "Yakovlev Yak-40"},
        {"YK42", "Yakovlev Yak-42"},
        {"YS11", "NAMC YS-11"}
      });

// ====================================================================================================

// Get runway name attributes
QString runwayFlags(QString runway, bool& prefixRw, bool& noPrefixNull, bool& suffixTrue)
{
  if(runway.isEmpty())
    return QString();

  prefixRw = runway.startsWith("RW");
  if(prefixRw)
    runway = runway.mid(2);

  suffixTrue = runway.endsWith('T');
  if(suffixTrue)
    runway.chop(1);

  // true for "1", "1C" and "1T"
  noPrefixNull = runway.size() == 1 || (runway.size() == 2 && !runway.at(1).isDigit());

  if(noPrefixNull)
    runway.prepend('0');

  return runway.toUpper();
}

QString normalizeRunway(QString runway)
{
  bool dummy;
  return runwayFlags(runway, dummy, dummy, dummy);
}

QStringList normalizeRunways(QStringList names)
{
  for(QString& name : names)
    name = normalizeRunway(name);
  return names;
}

const QString& aircraftTypeForCode(const QString& code)
{
  return atools::hashValue(NAME_CODE_MAP, code);
}

int calculateAirportRating(bool isAddon, bool hasTower, bool msfs, int numTaxiPaths, int numParkings, int numAprons)
{
  // Maximum rating is 5
  int rating = (numTaxiPaths > 0) + (numParkings > 0) + (numAprons > 0) + isAddon;

  // MSFS has a lot of generated airports with tiny apron snippets. Put rating to zero for these if they are not add-ons
  if(msfs && !isAddon && numTaxiPaths == 0 && numParkings == 0)
    rating = 0;

  if(rating > 0 && hasTower)
    // Add tower only if there is already a rating - otherwise we'll get too many airports with a too good rating
    rating++;

  return rating;
}

int calculateAirportRatingXp(bool isAddon, bool is3D, bool hasTower, int numTaxiPaths, int numParkings, int numAprons)
{
  // Maximum rating is 5
  int rating = (numTaxiPaths > 0) + (numParkings > 0) + (numAprons > 0) + (isAddon | is3D);

  if(rating > 0 && hasTower)
    // Add tower only if there is already a rating - otherwise we'll get too many airports with a too good rating
    rating++;

  return rating;
}

bool isNameClosed(const QString& airportName)
{
  return REGEXP_CLOSED.match(airportName.toUpper()).hasMatch();
}

bool isNameMilitary(QString airportName)
{
  airportName = airportName.toUpper();
  // Check if airport is military

  if(strContains(airportName, CONTAINS_MIL))
    return true;

  for(const QRegularExpression& s : REGEXP_MIL)
  {
    if(s.match(airportName).hasMatch())
      return true;
  }
  return false;
}

QString capWaypointNameString(const QString& ident, const QString& name, bool emptyIfEqual)
{
  if(ident == name)
    return emptyIfEqual ? QString() : name;
  else
  {
    if(name.contains('('))
    {
      QRegularExpressionMatch match = REGEXP_WAYPOINT_DME.match(name);
      if(match.hasMatch())
        // Special case "IKR138012 (KRE 11.2 DME)"
        return match.captured(1) % " (" % match.captured(2).toUpper() % ' ' + match.captured(3) % " DME)";

    }
  }

  return capNavString(name);
}

QString capNavString(const QString& str)
{
  if(str.contains(REGEXP_DIGIT) && !str.contains(REGEXP_WHITESPACE))
    // Do not capitalize words that contains numbers but not spaces (airspace names)
    return str;

  // Force abbreviations to upper case
  const static QSet<QString> FORCE_UPPER({
          // Navaids
          "VOR", "VORDME", "TACAN", "VOT", "VORTAC", "DME", "NDB", "GA", "RNAV", "GPS",
          "ILS", "NDBDME",
          // Frequencies
          "ATIS", "AWOS", "ASOS", "AWIS", "CTAF", "FSS", "CAT", "LOC",
          // Navaid and precision approach types
          "H", "HH", "MH", "VASI", "PAPI",
          // Airspace abbreviations
          "ALS", "ATZ", "CAE", "CTA", "CTR", "FIR", "UIR", "FIZ", "FTZ",
          "MATZ", "MOA", "RMZ", "TIZ", "TMA", "TMZ", "TRA", "TRSA", "TWEB", "ARSA", "FBZ", "PJE", "UAF",
          "AAS", "CARS", "FIS", "AFIS", "ATF", "VDF", "PCL", "RCO", "RCAG",
          "NOTAM", "CERAP", "ARTCC",
          "TCA", "MCTR", "VFR", "IFR", "DFS", "TNA", "CAE", "LANTA",
          "TSRA" "AFB", "OCA", "ARB", "MCAS", "NAS", "NOLF", "NS", "NAWS", "USAF", "TMAD", "CON", "ATS", "MTMA",
          "TRSA", "SFB", "AAF", "DC", "CGAS", "RT", "ASPC", "UAC", "LTA",
          "I", "II", "III", "IV", "V", "VI", "NM"});

  return atools::capString(str, FORCE_UPPER).trimmed();
}

QString adjustFsxUserWpName(QString name, int length)
{
  static const QRegularExpression USER_WP_NAME_REGEXP_FSX("[^A-Za-z0-9_ ]");

  name = atools::normalizeStr(name);
  name.replace(USER_WP_NAME_REGEXP_FSX, "");
  name = name.left(length).trimmed();
  if(name.isEmpty())
    name = "User_WP";
  return name;
}

QString adjustMsfsUserWpName(QString name, int length, int *number)
{
  static const QRegularExpression USER_WP_NAME_REGEXP_MSFS("[^A-Za-z0-9\\ \\/\\(\\)\\=\\?\\;\\,\\:\\.\\_\\-\\*]");

  name = atools::normalizeStr(name);
  name.replace(USER_WP_NAME_REGEXP_MSFS, "");
  name = name.left(length).trimmed();
  if(name.isEmpty())
  {
    if(number != nullptr)
      name = QString("AUTOWP%1").arg((*number)++);
    else
      name = "AUTOWP";
  }
  return name;
}

QString adjustIdent(QString ident, int length, int id)
{
  static const QRegularExpression IDENT_REGEXP("[^A-Z0-9]");
  ident = ident.toUpper().replace(IDENT_REGEXP, "").left(length);
  if(ident.isEmpty())
  {
    if(id != -1)
      ident = QString("N%1").arg(id, 4, 36, QChar('0')).left(length);
    else
      ident = "UNKWN";
  }
  return ident.toUpper();
}

QString adjustRegion(QString region)
{
  static const QRegularExpression IDENT_REGEXP("[^A-Z0-9]");
  region = region.toUpper().replace(IDENT_REGEXP, "").left(2);
  if(region.length() != 2)
    region = "ZZ";
  return region.toUpper();
}

bool isValidIdent(const QString& ident)
{
  static const QRegularExpression IDENT_REGEXP("^[A-Z0-9]{1,5}$");
  return IDENT_REGEXP.match(ident).hasMatch();
}

bool isValidRegion(const QString& region)
{
  static const QRegularExpression REGION_REGEXP("^[A-Z0-9]$");
  return REGION_REGEXP.match(region).hasMatch();
}

bool speedAndAltitudeMatch(const QString& item)
{
  return REGEXP_SPDALT_ALL.match(item).hasMatch();
}

bool extractSpeedAndAltitude(const QString& item, float& speedKnots, float& altFeet, bool *speedOk, bool *altitudeOk)
{
  // N0490F360
  // M084F330
  // Speed
  // K0800 (800 Kilometers)
  // N0490 (490 Knots)
  // M082 (Mach 0.82)
  // Level/altitude
  // F340 (Flight level 340)
  // S1260 (12600 Meters)
  // A100 (10000 Feet)
  // M0890 (8900 Meters)

  bool spdOk = true, altOk = true;
  speedKnots = 0.f;
  altFeet = 0.f;

  // const static QRegularExpression SPDALT(""^([NMK])(\\d{3,4})(([FSAM])(\\d{3,4}))?$"");
  QRegularExpressionMatch match = REGEXP_SPDALT.match(item);
  if(match.hasMatch())
  {
    QString speedUnit = match.captured(1);
    float speed = match.captured(2).toFloat(&spdOk);

    QString altUnit = match.captured(4);
    float alt = match.captured(5).toFloat(&altOk);

    // Altitude ==============================
    if(altUnit == "F") // Flight Level
      altFeet = alt >= 1000.f ? alt : alt * 100.f;
    else if(altUnit == "S") // Standard Metric Level in tens of meters
      altFeet = atools::geo::meterToFeet(alt * 10.f);
    else if(altUnit == "A") // Altitude in hundreds of feet
      altFeet = alt >= 1000.f ? alt : alt * 100.f;
    else if(altUnit == "M") // Altitude in tens of meters
      altFeet = atools::geo::meterToFeet(alt * 10.f);
    else
      altOk = false;

    // Speed ==============================
    if(speedUnit == "K") // km/h
      speedKnots = atools::geo::meterToNm(speed * 1000.f);
    else if(speedUnit == "N") // knots
      speedKnots = speed;
    else if(speedUnit == "M") // mach
      speedKnots = atools::geo::machToTasFromAlt(altFeet, speed / 100.f);
    else
      spdOk = false;
  }
  else
    spdOk = altOk = false;

  if(speedOk != nullptr)
    *speedOk = spdOk;
  if(altitudeOk != nullptr)
    *altitudeOk = altOk;

  return spdOk && altOk;
}

QString createSpeedAndAltitude(float speedKts, float altFeet, bool metricSpeed, bool metricAlt)
{
  QString str;
  if(metricSpeed)
    // K: Kilometers per hour followed by a four digit value.
    str = QString("K%1").arg(atools::geo::knotsToKmh(speedKts), 4, 'f', 0, QChar('0'));
  else
    // N: Knots followed by a four digit value.
    str = QString("N%1").arg(speedKts, 4, 'f', 0, QChar('0'));

  if(metricAlt)
  {
    // Meter ===========================
    if(altFeet < atools::geo::feetToMeter(18000.f))
      // M: Altitude in tens of meter in four digits.
      str.append(QString("M%2").arg(atools::geo::feetToMeter(altFeet) / 10.f, 4, 'f', 0, QChar('0')));
    else
      // S: Metric flight level in three digits of tens of meters.
      str.append(QString("S%2").arg(atools::geo::feetToMeter(altFeet) / 10.f, 3, 'f', 0, QChar('0')));
  }
  else
  {
    // Feet ===========================
    if(altFeet < 18000.f)
      // A: Altitude in hundreds of feet in three digits.
      str.append(QString("A%2").arg(altFeet / 100.f, 3, 'f', 0, QChar('0')));
    else
      // F :Flight level in three digits.
      str.append(QString("F%2").arg(altFeet / 100.f, 3, 'f', 0, QChar('0')));
  }
  return str;
}

float roundComFrequency(int frequency)
{
  if(frequency > 10000000)
    // E.g. 120425000 for X-Plane new 8.33 kHz - can be used without rounding
    return frequency / 1000000.f;
  else
    // 118775 for legacy - round to next 0.025r - This is obsolete now
    // return std::round(frequency / 1000.f / 0.025f) * 0.025f;
    return frequency / 1000.f;
}

qint16 decodeTransponderCode(int code)
{
  // Extract decimal digits
  int d1 = code / 1000;
  int d2 = code / 100 - d1 * 10;
  int d3 = code / 10 - d1 * 100 - d2 * 10;
  int d4 = code - d1 * 1000 - d2 * 100 - d3 * 10;

  if(atools::inRange(0, 7, d1) && atools::inRange(0, 7, d2) && atools::inRange(0, 7, d3) && atools::inRange(0, 7, d4))
    // Convert decimals to octal code
    return static_cast<qint16>((d1 << 9) | (d2 << 6) | (d3 << 3) | d4);
  else
    return -1;
}

bool runwayEqual(QString name1, QString name2, bool fuzzy)
{
  if(fuzzy)
  {
    QString rwdesignator1, rwdesignator2;
    int rwnum1, rwnum2;
    runwayNameSplitNum(name1, &rwnum1, &rwdesignator1);
    runwayNameSplitNum(name2, &rwnum2, &rwdesignator2);

    return (rwnum2 == rwnum1 || rwnum2 == (rwnum1 < 36 ? rwnum1 + 1 : 1) || rwnum2 == (rwnum1 > 1 ? rwnum1 - 1 : 36)) &&
           rwdesignator1 == rwdesignator2;
  }
  else
    return normalizeRunway(name1) == normalizeRunway(name2);
}

bool runwayContains(const QStringList& runways, QString name, bool fuzzy)
{
  if(fuzzy)
  {
    // Try exact match first
    for(const QString& rw : runways)
    {
      if(runwayEqual(rw, name, false /* fuzzy */))
        return true;
    }
  }

  for(const QString& rw : runways)
  {
    if(runwayEqual(rw, name, fuzzy))
      return true;
  }
  return false;
}

inline QString runwayNameJoin(int number, const QString& designator)
{
  return QString("%1%2").arg(number, 2, 10, QChar('0')).arg(designator);
}

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list */
const QStringList runwayNameVariants(QString name)
{
  QString prefix;
  if(name.startsWith("RW"))
  {
    prefix = "RW";
    name = name.mid(2);
  }

  QString suffix;
  if(name.endsWith('T'))
  {
    suffix = "T";
    name.chop(1);
  }

  QStringList retval({name});
  QString designator;
  int number;
  runwayNameSplitNum(name, &number, &designator);

  // Try next higher runway number
  retval.append(prefix % runwayNameJoin(number < 36 ? number + 1 : 1, designator) % suffix);

  // Try next lower runway number
  retval.append(prefix % runwayNameJoin(number > 1 ? number - 1 : 36, designator) % suffix);

  return retval;
}

const QStringList runwayNameZeroPrefixVariants(const QString& name)
{
  QStringList retval({name});

  if(name.startsWith('0'))
    retval.append(name.mid(1));
  else if(name.startsWith("RW0"))
    retval.append("RW" % name.mid(3));

  return retval;
}

QString runwayNamePrefixZero(const QString& name)
{
  QString number, designator;
  if(runwayNameSplitStr(name, &number, &designator))
    return number % designator;
  else
    return name;
}

/* Gives all variants of the runway (+1 and -1) plus the original one as the first in the list for an
 * ARINC name like N32 or I19-Y */
QStringList arincNameNameVariants(const QString& name)
{
  QStringList retval({name});
  QString prefix, suffix, rw;
  if(name.size() >= 3 && name.at(1).isDigit() && name.at(2).isDigit())
  {
    prefix = name.at(0);
    rw = name.mid(1, 2);
    suffix = name.mid(3);

    QString designator;
    int number;
    runwayNameSplitNum(rw, &number, &designator);

    // Try next higher runway number
    retval.append(prefix % runwayNameJoin(number < 36 ? number + 1 : 1, designator) % suffix);

    // Try next lower runway number
    retval.append(prefix % runwayNameJoin(number > 1 ? number - 1 : 36, designator) % suffix);
  }
  return retval;
}

QString runwayBestFitFromList(const QString& runwayName, const QStringList& airportRunwayNames)
{
  // normalize runways (RW1 to 01)
  QStringList apRwsNorm = normalizeRunways(airportRunwayNames);

  // Get normalized variants (04 -> {04, 03, 05})
  for(QString rwNormVariant : runwayNameVariants(normalizeRunway(runwayName)))
  {
    // Does variant exist in airport runways?
    int idx = apRwsNorm.indexOf(rwNormVariant);
    if(idx != -1)
    {
      // Get flags for original not normalized name
      QString apRw = airportRunwayNames.value(idx);
      bool prefixRw, noPrefixNull, suffixTrue;
      runwayFlags(apRw, prefixRw, noPrefixNull, suffixTrue);

      // Now adjust name according to the original runway name
      if(noPrefixNull && rwNormVariant.startsWith('0'))
        rwNormVariant = rwNormVariant.mid(1);

      if(prefixRw)
        rwNormVariant.prepend("RW");

      if(suffixTrue)
        rwNormVariant.append('T');

      return rwNormVariant;
    }
  }

  return QString();
}

QString runwayBestFit(const QString& runwayName, const QStringList& airportRunwayNames)
{
  // normalize runways (RW1 to 01)
  QStringList apRwsNorm = normalizeRunways(airportRunwayNames);

  // Get normalized variants (04 -> {04, 03, 05})
  for(QString procRwNormVariant : runwayNameVariants(normalizeRunway(runwayName)))
  {
    if(apRwsNorm.contains(procRwNormVariant))
    {
      // Get flags for given runway name
      bool prefixRw, noPrefixNull, suffixTrue;
      runwayFlags(runwayName, prefixRw, noPrefixNull, suffixTrue);

      // Adjust name according to the original runway name
      if(noPrefixNull && procRwNormVariant.startsWith('0'))
        procRwNormVariant = procRwNormVariant.mid(1);

      if(prefixRw)
        procRwNormVariant.prepend("RW");

      if(suffixTrue)
        procRwNormVariant.append('T');

      return procRwNormVariant;
    }
  }

  return QString();
}

QString runwayDesignatorLong(const QString& designatorName)
{
  if(designatorName.startsWith('L'))
    return "LEFT";
  else if(designatorName.startsWith('R'))
    return "RIGHT";
  else if(designatorName.startsWith('C'))
    return "CENTER";
  else if(designatorName.startsWith('W'))
    return "WATER";

  return designatorName;
}

bool runwayNameValid(const QString& name)
{
  return runwayNameSplitNum(name);
}

bool runwayNameSplitNum(const QString& name, int *number, QString *designator, bool *trueHeading)
{
  // Extract runway number and designator
  const static QRegularExpression NUM_DESIGNATOR("^([0-9]{1,2})([LRCWAB]?)(T?)$");

  QString rwname(name);
  if(rwname.startsWith("RW"))
    rwname = rwname.mid(2);

  if(number != nullptr)
    *number = 0;

  if(trueHeading != nullptr)
    *trueHeading = false;

  QRegularExpressionMatch match = NUM_DESIGNATOR.match(rwname);
  if(match.hasMatch())
  {
    if(number != nullptr)
      *number = match.captured(1).toInt();

    if(designator != nullptr)
      *designator = match.captured(2);

    if(trueHeading != nullptr)
      *trueHeading = match.captured(3) == "T";

    return true;
  }
  return false;
}

bool runwayNameSplitStr(const QString& name, QString *number, QString *designator, bool *trueHeading)
{
  int num = 0;
  bool retval = runwayNameSplitNum(name, &num, designator, trueHeading);

  if(retval && number != nullptr)
    // If it is a number with designator make sure to add a 0 prefix
    *number = QString("%1").arg(num, 2, 10, QChar('0'));
  return retval;
}

inline int runwayDesignatorNumber(const QString& designator)
{
  if(designator.startsWith('L'))
    return 0;
  else if(designator.startsWith('C'))
    return 1;
  else if(designator.startsWith('R'))
    return 2;

  return 3;
}

int compareRunwayNumber(const QString& rw1, const QString& rw2)
{
  QString designator1, designator2;
  int number1 = 0, number2 = 0;
  atools::fs::util::runwayNameSplitNum(rw1, &number1, &designator1);
  atools::fs::util::runwayNameSplitNum(rw2, &number2, &designator2);

  if(number1 == number2)
    return runwayDesignatorNumber(designator1) < runwayDesignatorNumber(designator2);
  else
    return number1 < number2;
}

bool hasSidStarAllRunways(const QString& approachArincName)
{
  return approachArincName == "ALL" || approachArincName.isEmpty();
}

bool hasSidStarParallelRunways(QString approachArincName)
{
  const static QRegularExpression PARALLEL_REGEXP("^RW[0-9]{2}B$");

  if(hasSidStarAllRunways(approachArincName))
    return false;
  else
  {
    if(!approachArincName.startsWith("RW"))
      approachArincName = "RW" % approachArincName;

    return approachArincName.contains(PARALLEL_REGEXP);
  }
}

void sidStarMultiRunways(const QStringList& runwayNames, const QString& arincName, QStringList *sidStarRunways,
                         const QString& allDisplayName, QStringList *sidStarDispNames)
{
  if(hasSidStarAllRunways(arincName))
  {
    if(sidStarDispNames != nullptr)
      sidStarDispNames->append(allDisplayName);
    if(sidStarRunways != nullptr)
      sidStarRunways->append(runwayNames);
  }
  else if(hasSidStarParallelRunways(arincName))
  {
    // Check which runways are assigned from values like "RW12B"
    QString runwayName = arincName.mid(2, 2);
    if(runwayContains(runwayNames, runwayName % "L", false /* fuzzy */))
    {
      if(sidStarDispNames != nullptr)
        sidStarDispNames->append(runwayName % "L");

      if(sidStarRunways != nullptr)
        sidStarRunways->append(runwayName % "L");
    }

    if(runwayContains(runwayNames, runwayName % "R", false /* fuzzy */))
    {
      if(sidStarDispNames != nullptr)
        sidStarDispNames->append(runwayName % "R");

      if(sidStarRunways != nullptr)
        sidStarRunways->append(runwayName % "R");
    }

    if(runwayContains(runwayNames, runwayName % "C", false /* fuzzy */))
    {
      if(sidStarDispNames != nullptr)
        sidStarDispNames->append(runwayName % "C");

      if(sidStarRunways != nullptr)
        sidStarRunways->append(runwayName % "C");
    }
  }
}

QString waypointFlagsToXplane(QString flags, const QString& defaultValue)
{
  // Allow underscore as space replacement and quotes
  flags.replace('_', ' ').remove('\"');

  if(flags.size() != 3)
    return defaultValue;
  else
  {
    // 32bit representation of the 3-byte field defined by ARINC
    // 424.18 field type definition 5.42, with the 4th byte set to 0 in
    // Little Endian byte order. This field can be empty ONLY for user
    // waypoints in user_fix.dat
    union
    {
      quint32 intValue;
      unsigned char byteValue[4];
    } u;

    u.byteValue[0] = atools::strToUChar(flags, 0); // Col 27: "V" = "VFR Waypoint", "I" = Unnamed Charted Intersection, "R" = Named Intersection
    u.byteValue[1] = atools::strToUChar(flags, 1); // Col 28
    u.byteValue[2] = atools::strToUChar(flags, 2); // Col 29
    u.byteValue[3] = 0; // Fourth is null
    return QString::number(u.intValue);
  }
}

QString waypointFlagsFromXplane(const QString& flags, const QString& defaultValue)
{
  // 32bit representation of the 3-byte field defined by ARINC
  // 424.18 field type definition 5.42, with the 4th byte set to 0 in
  // Little Endian byte order. This field can be empty ONLY for user
  // waypoints in user_fix.dat
  union
  {
    quint32 intValue;
    unsigned char byteValue[4];
  } u;
  bool ok;
  u.intValue = flags.toUInt(&ok); // Always little endian
  QString arincTypeStr(defaultValue);
  if(ok)
  {
    if(u.byteValue[0] > 0)
      arincTypeStr.append(QChar(u.byteValue[0]));
    if(u.byteValue[1] > 1)
      arincTypeStr.append(QChar(u.byteValue[1]));
    if(u.byteValue[2] > 2)
      arincTypeStr.append(QChar(u.byteValue[2]));
    if(u.byteValue[3] > 3)
      arincTypeStr.append(QChar(u.byteValue[3]));
  }
  return arincTypeStr;
}

void calculateIlsGeometry(const atools::geo::Pos& pos, float headingTrue, float widthDeg, float featherLengthNm,
                          atools::geo::Pos& p1, atools::geo::Pos& p2, atools::geo::Pos& pmid)
{
  float hdg = atools::geo::opposedCourseDeg(headingTrue);
  float lengthMeter = atools::geo::nmToMeter(featherLengthNm);
  widthDeg = std::max(2.f, widthDeg);

  if(!(widthDeg < atools::geo::INVALID_FLOAT) || widthDeg < 0.1f)
    widthDeg = 4.f;

  p1 = pos.endpoint(lengthMeter, hdg - widthDeg / 2.f);
  p2 = pos.endpoint(lengthMeter, hdg + widthDeg / 2.f);
  float featherWidth = p1.distanceMeterTo(p2);
  pmid = pos.endpoint(lengthMeter - featherWidth / 2.f, hdg);
}

QDateTime xpGribFilenameToDate(const QString& filename)
{
  // XP 11 and 12
  // GRIB-2022-11-25-00.00-ZULU-wind.grib
  // GRIB-2022-9-6-21.00-ZULU-wind.grib
  // GRIB-2023-02-22-18.00-ZULU-wind-v2.grib
  const static QRegularExpression GRIB_REGEXP("^GRIB-(\\d+)-(\\d+)-(\\d+)-(\\d+).(\\d+)(-ZULU)?-wind(-v\\d+)?\\.grib$",
                                              QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = GRIB_REGEXP.match(filename);

  if(match.hasMatch())
  {
#ifdef DEBUG_INFORMATION
    qDebug() << Q_FUNC_INFO << "Match for" << filename << match.capturedTexts();
#endif

    return QDateTime(QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()),
                     QTime(match.captured(4).toInt(), match.captured(5).toInt()), Qt::UTC);
  }
  else
  {
#ifdef DEBUG_INFORMATION
    qDebug() << Q_FUNC_INFO << "No match for" << filename;
#endif
    return QDateTime();
  }
}

QDateTime xpMetarFilenameToDate(const QString& filename)
{
  // XP 12
  // Metar-2022-9-6-20.00.txt
  // metar-2022-10-2-22.00.txt
  const static QRegularExpression METAR_REGEXP("METAR-(\\d+)-(\\d+)-(\\d+)-(\\d+).(\\d+)(-ZULU)?\\.txt$",
                                               QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = METAR_REGEXP.match(filename);

  if(match.hasMatch())
    return QDateTime(QDate(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt()),
                     QTime(match.captured(4).toInt(), match.captured(5).toInt()), Qt::UTC);
  else
    return QDateTime();
}

bool isAircraftTypeDesignatorValid(const QString& type)
{
  const static QRegularExpression AIRCRAFT_TYPE("^[A-Z0-9]{2,4}$");

  return type.isEmpty() ? false : AIRCRAFT_TYPE.match(type).hasMatch();
}

atools::geo::LineString correctBoundary(const atools::geo::LineString& geometry)
{
  atools::geo::LineString boundary;
  for(int i = 0; i < geometry.size(); i++)
  {
    const atools::geo::Pos& current = atools::atRollConst(geometry, i);
    const atools::geo::Pos& next = atools::atRollConst(geometry, i + 1);

    // Calculate NM between points depending on latitude
    float lat = std::abs(current.getLatY());
    float pointDistIntervalNm;
    if(lat > 70.f)
      pointDistIntervalNm = 20.f;
    else if(lat > 60.f)
      pointDistIntervalNm = 40.f;
    else if(lat > 30.f)
      pointDistIntervalNm = 70.f;
    else if(lat > 10.f)
      pointDistIntervalNm = 90.f;
    else
      pointDistIntervalNm = 250.f;

    float distMeter = current.distanceMeterTo(next);
    if(atools::almostEqual(current.getLatY(), next.getLatY()) && distMeter > atools::geo::nmToMeter(pointDistIntervalNm))
    {
      int numPoints = atools::ceilToInt(distMeter / atools::geo::nmToMeter(pointDistIntervalNm));

      atools::geo::LineString positions;
      current.interpolatePointsRhumb(next, distMeter, numPoints, positions);
      boundary.append(positions);
    }
    else
      boundary.append(current);
  }

  // Move points away from the poles
  for(atools::geo::Pos& pos : boundary)
  {
    if(pos.getLatY() > 89.9f)
      pos.setLatY(89.9f);
    if(pos.getLatY() < -89.9f)
      pos.setLatY(-89.9f);
  }

  return boundary;
}

QString runwayNumber(const QString& runwayName)
{
  int number;
  if(atools::fs::util::runwayNameSplitNum(runwayName, &number))
    return QString::number(number);
  else
    return QString();
}

QString runwayDesignator(const QString& runwayName)
{
  QString designator;
  if(atools::fs::util::runwayNameSplitNum(runwayName, nullptr, &designator))
    return atools::fs::util::runwayDesignatorLong(designator);
  else
    return QString();
}

} // namespace util
} // namespace fs
} // namespace atools
