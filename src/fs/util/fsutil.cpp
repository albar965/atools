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

#include "fs/util/fsutil.h"
#include "atools.h"
#include "geo/calculations.h"

#include <QRegularExpression>
#include <QSet>

namespace atools {
namespace fs {
namespace util {

// Closed airport by name
static const QRegularExpression REGEXP_CLOSED(QLatin1Literal("(\\[X\\]|\\bCLSD\\b|\\bCLOSED\\b)"));
const static QRegularExpression REGEXP_DIGIT("\\d");
const static QRegularExpression REGEXP_WHITESPACE("\\s");

/* ICAO speed and altitude matches */
const static QRegularExpression REGEXP_SPDALT("^([NMK])(\\d{2,4})(([FSAM])(\\d{2,4}))?$");
const static QRegularExpression REGEXP_SPDALT_ALL("^([NMK])(\\d{3,4})([FSAM])(\\d{3,4})$");

// Look for longer military designators - taken from MSFS
static const QStringList CONTAINS_MIL({
        "MILITÄR", // de-DE.locPak:
        "BASE AÉREA", // es-ES.locPak:
        "BASE AÉRIENNE", // fr-FR.locPak:
        "BASE AEREA", // it-IT.locPak:
        "BAZA LOTNICZA", // pl-PL.locPak:
        "BASE AÉREA BRACCIANO" // pt-BR.locPak:
      });

// Look for military designator words
static const QVector<QRegularExpression> REGEXP_MIL({
        // "AAF", "AB", "AF", "AFB", "AFS", "AHP", "ANGB", "ARB", "GTS", "LRRS", "PMRF", "MCAF", "MCALF", "MCAS", "NAF",
        // "NALF", "NAS", "NWS", "NAWS", "NOLF", "NS", "NSB", "NSY", "NSWC", "NSF", "RAF", "RNAS", "AFLD", "AAC",

        QRegularExpression(QLatin1Literal("(\\[M\\]|\\[MIL\\])")), // X-Plane special
        QRegularExpression(QLatin1Literal("\\bAAF\\b")),
        QRegularExpression(QLatin1Literal("\\bAB\\b")),
        QRegularExpression(QLatin1Literal("\\bAF\\b")),
        QRegularExpression(QLatin1Literal("\\bAFB\\b")),
        QRegularExpression(QLatin1Literal("\\bAFS\\b")),
        QRegularExpression(QLatin1Literal("\\bAHP\\b")),
        QRegularExpression(QLatin1Literal("\\bAIR BASE\\b")),
        QRegularExpression(QLatin1Literal("\\bAIRBASE\\b")),
        QRegularExpression(QLatin1Literal("\\bAIR FORCE\\b")),
        QRegularExpression(QLatin1Literal("\\bANGB\\b")),
        QRegularExpression(QLatin1Literal("\\bARB\\b")),
        QRegularExpression(QLatin1Literal("\\bARMY\\b")),
        // QRegularExpression(QLatin1Literal("\\bGTS\\b")), not an airbase
        QRegularExpression(QLatin1Literal("\\bLRRS\\b")),
        QRegularExpression(QLatin1Literal("\\bPMRF\\b")),
        QRegularExpression(QLatin1Literal("\\bMCAF\\b")),
        QRegularExpression(QLatin1Literal("\\bMCALF\\b")),
        QRegularExpression(QLatin1Literal("\\bMCAS\\b")),
        QRegularExpression(QLatin1Literal("\\bMIL\\b")),
        QRegularExpression(QLatin1Literal("\\bMILITARY\\b")),
        QRegularExpression(QLatin1Literal("\\bNAF\\b")),
        QRegularExpression(QLatin1Literal("\\bNALF\\b")),
        QRegularExpression(QLatin1Literal("\\bNAS\\b")),
        QRegularExpression(QLatin1Literal("\\bNWS\\b")),
        QRegularExpression(QLatin1Literal("\\bNAVAL\\b")),
        QRegularExpression(QLatin1Literal("\\bNAVY\\b")),
        QRegularExpression(QLatin1Literal("\\bNAWS\\b")),
        QRegularExpression(QLatin1Literal("\\bNOLF\\b")),
        QRegularExpression(QLatin1Literal("\\bNS\\b")),
        QRegularExpression(QLatin1Literal("\\bNSF\\b")),
        QRegularExpression(QLatin1Literal("\\bNSB\\b")),
        QRegularExpression(QLatin1Literal("\\bNSY\\b")),
        QRegularExpression(QLatin1Literal("\\bRAF\\b")),
        QRegularExpression(QLatin1Literal("\\bNSWC\\b")),
        QRegularExpression(QLatin1Literal("\\bRNAS\\b")),
        QRegularExpression(QLatin1Literal("\\bROYAL MARINES\\b")),
        QRegularExpression(QLatin1Literal("\\bAFLD\\b")),
        QRegularExpression(QLatin1Literal("\\bAAC\\b"))

      });

static const QHash<QString, QString> NAME_CODE_MAP(
      {
        {"A124", "Antonov AN-124 Ruslan"},
        {"A140", "Antonov AN-140"},
        {"A148", "Antonov An-148"},
        {"A158", "Antonov An-158"},
        {"A19N", "Airbus A319neo"},
        {"A20N", "Airbus A320neo"},
        {"A21N", "Airbus A321neo"},
        {"A225", "Antonov An-225 Mriya"},
        {"A306", "Airbus A300-600"},
        {"A30B", "Airbus A300"},
        {"A310", "Airbus A310"},
        {"A318", "Airbus A318"},
        {"A319", "Airbus A319"},
        {"A320", "Airbus A320"},
        {"A321", "Airbus A321"},
        {"A332", "Airbus A330-200"},
        {"A333", "Airbus A330-300"},
        {"A339", "Airbus A330-900"},
        {"A342", "Airbus A340-200"},
        {"A343", "Airbus A340-300"},
        {"A345", "Airbus A340-500"},
        {"A346", "Airbus A340-600"},
        {"A359", "Airbus A350-900"},
        {"A35K", "Airbus A350-1000"},
        {"A388", "Airbus A380-800"},
        {"A3ST", "Airbus A300-600ST Beluga Freighter"},
        {"A5", "ICON A5"},
        {"A748", "Hawker Siddeley HS 748"},
        {"AC68", "Gulfstream/Rockwell (Aero) Commander"},
        {"AC90", "Gulfstream/Rockwell (Aero) Turbo Commander"},
        {"AN12", "Antonov AN-12"},
        {"AN24", "Antonov AN-24"},
        {"AN26", "Antonov AN-26"},
        {"AN28", "Antonov AN-28"},
        {"AN30", "Antonov AN-30"},
        {"AN32", "Antonov AN-32"},
        {"AN72", "Antonov AN-72 / AN-74"},
        {"AP22", "Aeroprakt A-22 Foxbat / A-22 Valor / A-22 Vision"},
        {"AS32", "Eurocopter AS332 Super Puma"},
        {"AS50", "Eurocopter AS350 Écureuil / AS355 Ecureuil 2 / AS550 Fennec"},
        {"AT43", "Aerospatiale/Alenia ATR 42-300 / 320"},
        {"AT45", "Aerospatiale/Alenia ATR 42-500"},
        {"AT46", "Aerospatiale/Alenia ATR 42-600"},
        {"AT72", "Aerospatiale/Alenia ATR 72"},
        {"AT73", "Aerospatiale/Alenia ATR 72-200 series"},
        {"AT75", "Aerospatiale/Alenia ATR 72-500"},
        {"AT76", "Aerospatiale/Alenia ATR 72-600"},
        {"ATL", "Robin ATL"},
        {"ATP", "British Aerospace ATP"},
        {"B105", "Eurocopter (MBB) Bo.105"},
        {"B190", "Beechcraft 1900"},
        {"B212", "Bell 212"},
        {"B412", "Bell 412"},
        {"B429", "Bell 429"},
        {"B37M", "Boeing 737 MAX 7"},
        {"B38M", "Boeing 737 MAX 8"},
        {"B39M", "Boeing 737 MAX 9"},
        {"B461", "BAe 146-100"},
        {"B462", "BAe 146-200"},
        {"B463", "BAe 146-300"},
        {"B703", "Boeing 707"},
        {"B712", "Boeing 717"},
        {"B720", "Boeing 720B"},
        {"B721", "Boeing 727-100"},
        {"B722", "Boeing 727-200"},
        {"B732", "Boeing 737-200"},
        {"B733", "Boeing 737-300"},
        {"B734", "Boeing 737-400"},
        {"B735", "Boeing 737-500"},
        {"B736", "Boeing 737-600"},
        {"B737", "Boeing 737-700"},
        {"B738", "Boeing 737-800"},
        {"B739", "Boeing 737-900"},
        {"B741", "Boeing 747-100"},
        {"B742", "Boeing 747-200"},
        {"B743", "Boeing 747-300"},
        {"B744", "Boeing 747-400"},
        {"B748", "Boeing 747-8"},
        {"B74R", "Boeing 747SR"},
        {"B74S", "Boeing 747SP"},
        {"B752", "Boeing 757-200"},
        {"B753", "Boeing 757-300"},
        {"B762", "Boeing 767-200"},
        {"B763", "Boeing 767-300"},
        {"B764", "Boeing 767-400"},
        {"B772", "Boeing 777-200 / Boeing 777-200ER"},
        {"B77L", "Boeing 777-200LR / Boeing 777F"},
        {"B773", "Boeing 777-300"},
        {"B77W", "Boeing 777-300ER"},
        {"B788", "Boeing 787-8"},
        {"B789", "Boeing 787-9"},
        {"B78X", "Boeing 787-10"},
        {"BA11", "British Aerospace (BAC) One Eleven"},
        {"BCS1", "Bombardier CS100"},
        {"BCS3", "Bombardier CS300"},
        {"BE55", "Beechcraft Baron / 55 Baron"},
        {"BE58", "Beechcraft Baron / 58 Baron"},
        {"BELF", "Shorts SC-5 Belfast"},
        {"BER2", "Beriev Be-200 Altair"},
        {"BLCF", "Boeing 747 LCF Dreamlifter"},
        {"BN2P", "Pilatus Britten-Norman BN-2A/B Islander"},
        {"C130", "Lockheed L-182 / 282 / 382 (L-100) Hercules"},
        {"C152", "Cessna 152"},
        {"C162", "Cessna 162"},
        {"C172", "Cessna 172"},
        {"C72R", "Cessna 172 Cutlass RG"},
        {"C77R", "Cessna 177 Cardinal RG"},
        {"C182", "Cessna 182 Skylane"},
        {"C208", "Cessna 208 Caravan"},
        {"C210", "Cessna 210 Centurion"},
        {"C212", "CASA / IPTN 212 Aviocar"},
        {"C25A", "Cessna Citation CJ2"},
        {"C25B", "Cessna Citation CJ3"},
        {"C25C", "Cessna Citation CJ4"},
        {"C46", "Curtiss C-46 Commando"},
        {"C500", "Cessna Citation I"},
        {"C510", "Cessna Citation Mustang"},
        {"C525", "Cessna CitationJet"},
        {"C550", "Cessna Citation II"},
        {"C560", "Cessna Citation V"},
        {"C56X", "Cessna Citation Excel"},
        {"C650", "Cessna Citation III"},
        {"C680", "Cessna Citation Sovereign"},
        {"C750", "Cessna Citation X"},
        {"CL2T", "Bombardier 415"},
        {"CL30", "Bombardier BD-100 Challenger 300"},
        {"CL44", "Canadair CL-44"},
        {"CL60", "Canadair Challenger"},
        {"CN35", "CASA/IPTN CN-235"},
        {"CONI", "Lockheed L-1049 Super Constellation"},
        {"CRJ1", "Canadair Regional Jet 100"},
        {"CRJ2", "Canadair Regional Jet 200"},
        {"CRJ7", "Canadair Regional Jet 700"},
        {"CRJ9", "Canadair Regional Jet 900"},
        {"CRJX", "Canadair Regional Jet 1000"},
        {"CVLP", "Convair CV-240 & -440"},
        {"CVLT", "Convair CV-580, Convair CV-600, Convair CV-640"},
        {"D228", "Fairchild Dornier Do.228"},
        {"D328", "Fairchild Dornier Do.328"},
        {"DC10", "Douglas DC-10"},
        {"DC3", "Douglas DC-3"},
        {"DC6", "Douglas DC-6"},
        {"DC85", "Douglas DC-8-50"},
        {"DC86", "Douglas DC-8-62"},
        {"DC87", "Douglas DC-8-72"},
        {"DC91", "Douglas DC-9-10"},
        {"DC92", "Douglas DC-9-20"},
        {"DC93", "Douglas DC-9-30"},
        {"DC94", "Douglas DC-9-40"},
        {"DC95", "Douglas DC-9-50"},
        {"DH2T", "De Havilland Canada DHC-2 Turbo-Beaver"},
        {"DH8A", "De Havilland Canada DHC-8-100 Dash 8 / 8Q"},
        {"DH8B", "De Havilland Canada DHC-8-200 Dash 8 / 8Q"},
        {"DH8C", "De Havilland Canada DHC-8-300 Dash 8 / 8Q"},
        {"DH8D", "De Havilland Canada DHC-8-400 Dash 8Q"},
        {"DHC2", "De Havilland Canada DHC-2 Beaver"},
        {"DHC3", "De Havilland Canada DHC-3 Otter"},
        {"DHC4", "De Havilland Canada DHC-4 Caribou"},
        {"DHC5", "De Havilland Canada DHC-5 Buffalo"},
        {"DHC6", "De Havilland Canada DHC-6 Twin Otter"},
        {"DHC7", "De Havilland Canada DHC-7 Dash 7"},
        {"DOVE", "De Havilland DH.104 Dove"},
        {"E110", "Embraer EMB 110 Bandeirante"},
        {"E120", "Embraer EMB 120 Brasilia"},
        {"E135", "Embraer RJ135"},
        {"E135", "Embraer RJ140"},
        {"E145", "Embraer RJ145"},
        {"E170", "Embraer 170"},
        {"E190", "Embraer 190"},
        {"E195", "Embraer 195"},
        {"E35L", "Embraer Legacy 600 / Legacy 650"},
        {"E545", "Embraer Legacy 450"},
        {"E190", "Embraer Lineage 1000"},
        {"E50P", "Embraer Phenom 100"},
        {"E55P", "Embraer Phenom 300"},
        {"E75L", "Embraer 175 (long wing)"},
        {"E75S", "Embraer 175 (short wing)"},
        {"EC20", "Eurocopter EC120 Colibri / Harbin HC120"},
        {"EC25", "Eurocopter EC225 Super Puma"},
        {"EC35", "Eurocopter EC135 / EC635"},
        {"EC45", "Eurocopter EC145"},
        {"ECHO", "Tecnam P92 Echo / P92 Eaglet / P92 SeaSky"},
        {"EV97", "Evektor SportStar / EV-97 Harmony / EV-97 EuroStar"},
        {"EXPL", "MD Helicopters MD900 Explorer"},
        {"F100", "Fokker 100"},
        {"F27", "Fokker F27 Friendship"},
        {"F28", "Fokker F28 Fellowship"},
        {"F2TH", "Dassault Falcon 2000"},
        {"F50", "Fokker 50"},
        {"F70", "Fokker 70"},
        {"F900", "Dassault Falcon 900"},
        {"FA7X", "Dassault Falcon 7X"},
        {"G159", "Gulfstream Aerospace G-159 Gulfstream I"},
        {"G21", "Grumman G-21 Goose"},
        {"G280", "Gulfstream G280"},
        {"G73T", "Grumman G-73 Turbo Mallard"},
        {"GL5T", "Bombardier BD-700 Global 5000"},
        {"GLEX", "Bombardier Global Express / Raytheon Sentinel"},
        {"GLF4", "Gulfstream IV"},
        {"GLF5", "Gulfstream V"},
        {"GLF6", "Gulfstream G650"},
        {"HERN", "De Havilland DH.114 Heron"},
        {"H25B", "British Aerospace 125 series / Hawker/Raytheon 700/800/800XP/850/900"},
        {"H25C", "British Aerospace 125-1000 series / Hawker/Raytheon 1000"},
        {"HDJT", "Honda HA-420"},
        {"I114", "Ilyushin IL114"},
        {"IL18", "Ilyushin IL18"},
        {"IL62", "Ilyushin IL62"},
        {"IL76", "Ilyushin IL76"},
        {"IL86", "Ilyushin IL86"},
        {"IL96", "Ilyushin IL96"},
        {"J328", "Fairchild Dornier 328JET"},
        {"JS31", "British Aerospace Jetstream 31"},
        {"JS32", "British Aerospace Jetstream 32"},
        {"JS41", "British Aerospace Jetstream 41"},
        {"JU52", "Junkers Ju52/3M"},
        {"L101", "Lockheed L-1011 Tristar"},
        {"L188", "Lockheed L-188 Electra"},
        {"L410", "LET 410"},
        {"LJ35", "Learjet 35 / 36 / C-21A"},
        {"LJ60", "Learjet 60"},
        {"MD11", "McDonnell Douglas MD-11"},
        {"MD81", "McDonnell Douglas MD-81"},
        {"MD82", "McDonnell Douglas MD-82"},
        {"MD83", "McDonnell Douglas MD-83"},
        {"MD87", "McDonnell Douglas MD-87"},
        {"MD88", "McDonnell Douglas MD-88"},
        {"MD90", "McDonnell Douglas MD-90"},
        {"MI8", "MIL Mi-8 / Mi-17 / Mi-171 / Mil-172"},
        {"MI24", "Mil Mi-24 / Mi-25 / Mi-35"},
        {"MU2", "Mitsubishi Mu-2"},
        {"N262", "Aerospatiale (Nord) 262"},
        {"NOMA", "Government Aircraft Factories N22B / N24A Nomad"},
        {"P06T", "Tecnam P2006T"},
        {"P28A", "Piper PA-28(up to 180 hp)"},
        {"P28B", "Piper PA-28(above 200 hp)"},
        {"P68", "Partenavia P.68"},
        {"PA31", "Piper PA-31 Navajo"},
        {"PA44", "Piper PA-44 Seminole"},
        {"PA46", "Piper PA-46"},
        {"PC12", "Pilatus PC-12"},
        {"PC6T", "Pilatus PC-6 Turbo Porter"},
        {"RJ1H", "Avro RJ100"},
        {"R200", "Robin HR200/R2000 series, Alpha160A"},
        {"RJ70", "Avro RJ70"},
        {"RJ85", "Avro RJ85"},
        {"S210", "Aerospatiale (Sud Aviation) Se.210 Caravelle"},
        {"S58T", "Sikorsky S-58T"},
        {"S601", "Aerospatiale SN.601 Corvette"},
        {"S61", "Sikorsky S-61"},
        {"S65C", "Eurocopter (Aerospatiale) SA365C / SA365N Dauphin 2"},
        {"S76", "Sikorsky S-76"},
        {"S92", "Sikorsky S-92"},
        {"SB20", "Saab 2000"},
        {"SC7", "Shorts SC-7 Skyvan"},
        {"SF34", "Saab SF340A/B"},
        {"SH33", "Shorts SD.330"},
        {"SH36", "Shorts SD.360"},
        {"SU95", "Sukhoi Superjet 100"},
        {"T134", "Tupolev Tu-134"},
        {"T144", "Tupolev Tu-144"},
        {"T154", "Tupolev Tu-154"},
        {"T204", "Tupolev Tu-204 / Tu-214"},
        {"TB20", "Socata TB-20 Trinidad"},
        {"TL20", "TL Ultralight TL-96 Star / TL-2000 Sting"},
        {"TRIS", "Pilatus Britten-Norman BN-2A Mk III Trislander"},
        {"WW24", "Israel Aircraft Industries 1124 Westwind"},
        {"Y12", "Harbin Yunshuji Y12"},
        {"YK40", "Yakovlev Yak-40"},
        {"YK42", "Yakovlev Yak-42"},
        {"YS11", "NAMC YS-11"}
      });

QString aircraftTypeForCode(const QString& code)
{
  return NAME_CODE_MAP.value(code);
}

int calculateAirportRating(bool isAddon, bool hasTower, int numTaxiPaths, int numParkings, int numAprons)
{
  // Maximum rating is 5
  int rating = (numTaxiPaths > 0) + (numParkings > 0) + (numAprons > 0) + isAddon;

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
          "ATIS", "AWOS", "ASOS", "AWIS", "CTAF", "FSS", "CAT", "LOC", "I", "II", "III",
          // Navaid and precision approach types
          "H", "HH", "MH", "VASI", "PAPI",
          // Airspace abbreviations
          "ALS", "ATZ", "CAE", "CTA", "CTR", "FIR", "UIR", "FIZ", "FTZ",
          "MATZ", "MOA", "RMZ", "TIZ", "TMA", "TMZ", "TRA", "TRSA", "TWEB", "ARSA",
          "AAS", "CARS", "FIS", "AFIS", "ATF", "VDF", "PCL", "RCO", "RCAG",
          "NOTAM", "CERAP", "ARTCC",
        });

  return atools::capString(str, FORCE_UPPER);
}

QString capAirportName(const QString& str)
{
  // Force acronyms to upper case
  const static QSet<QString> FORCE_UPPER({
          "AAC", "AAF", "AB", "ABMS", "AF", "AFB", "AFLD", "AFS", "AHP", "ANGB", "APCM", "ARB", "CGS", "DGAC", "FBO",
          "GTS", "LRRS", "MAF", "MCAF", "MCALF", "MCAS", "NAF", "NALF", "NAS", "NAWS", "NFK", "NOLF", "NRC", "NRC",
          "NS", "NSB", "NSF", "NSWC", "NSY", "NWS", "PMRF", "RAF", "RBMU", "RNAS", "USFS"});

  return atools::capString(str, FORCE_UPPER);
}

QString adjustFsxUserWpName(QString name, int length)
{
  static const QRegularExpression NAME_REGEXP("[^A-Za-z0-9_ ]");
  name.replace(NAME_REGEXP, "");
  name = name.left(length);
  if(name.isEmpty())
    name = "User_WP";
  return name;
}

QString adjustIdent(QString ident, int length, int id)
{
  static const QRegularExpression IDENT_REGEXP("[^A-Z0-9]");
  ident = ident.toUpper().replace(IDENT_REGEXP, "").left(length);
  if(ident.isEmpty() && id != -1)
    ident = QString("N%1").arg(id, 4, 36, QChar('0')).left(length);
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

QString createSpeedAndAltitude(float speedKnots, float altFeet)
{
  if(altFeet < 18000.f)
    return QString("N%1A%2").arg(speedKnots, 4, 'f', 0, QChar('0')).arg(altFeet / 100.f, 3, 'f', 0, QChar('0'));
  else
    return QString("N%1F%2").arg(speedKnots, 4, 'f', 0, QChar('0')).arg(altFeet / 100.f, 3, 'f', 0, QChar('0'));
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

} // namespace util
} // namespace fs
} // namespace atools
