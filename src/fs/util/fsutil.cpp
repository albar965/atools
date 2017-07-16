/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include <QRegularExpression>
#include <QSet>

namespace atools {
namespace fs {
namespace util {

static const QStringList MIL_ENDS_WITH({" Mil", " Aaf", " Ab", " Af", " Afb", " Afs", " Ahp", " Angb", " Arb",
                                        " Lrrs", " Mcaf", " Mcalf", " Mcas", " Naf", " Nalf", " Nas",
                                        " Naval", " Naws", " Nolf", " Ns"});
static const QStringList MIL_CONTAINS({" AAF", " AB", " AF", " AFB", " AFS", " AHP", " ANGB", " ARB", " LRRS",
                                       " MCAF", " MCALF", " MCAS", " NAF", " NALF", " NAS", " Naval", " Navy",
                                       " NAWS", " NOLF", " NS", " Army", " Mil ", "Military", "Air Force",
                                       " Aaf ", " Ab ", " Af ", " Afb ", " Afs ", " Ahp ", " Angb ", " Arb ",
                                       " Lrrs ", " Mcaf ", " Mcalf ", " Mcas ", " Naf ", " Nalf ", " Nas ",
                                       " Naval ", " Naws ", " Nolf ", " Ns "});

int calculateAirportRating(bool isAddon, bool hasTower, int numTaxiPaths, int numParkings, int numAprons)
{
  // Maximum rating is 5
  int rating = (numTaxiPaths > 0) + (numParkings > 0) + (numAprons > 0) + isAddon;

  if(rating > 0 && hasTower)
    // Add tower only if there is already a rating - otherwise we'll get too many airports with a too good rating
    rating++;

  return rating;
}

bool isNameMilitary(const QString& airportName)
{
  // Check if airport is military
  for(const QString& s : MIL_ENDS_WITH)
  {
    if(airportName.endsWith(s))
      return true;
  }

  for(const QString& s : MIL_CONTAINS)
  {
    if(airportName.contains(s))
      return true;
  }
  return false;
}

QString capNavString(const QString& str)
{
  if(str.contains(QRegularExpression("\\d")) && !str.contains(QRegularExpression("\\s")))
    // Do not capitalize words that contains numbers but not spaces (airspace names)
    return str;

  // Ignore aviation acronyms in capitalization
  static const QSet<QString> ignore({ // Navaids
          "VOR", "VORDME", "TACAN", "VOT", "VORTAC", "DME", "NDB", "GA", "RNAV", "GPS",
          "ILS", "NDBDME",
          // Frequencies
          "ATIS", "AWOS", "ASOS", "CTAF", "FSS", "CAT", "LOC", "I", "II", "III",
          // Navaid and precision approach types
          "H", "HH", "MH", "VASI", "PAPI",
          // Airspace abbreviations
          "ALS", "CTA", "CAE", "TMA", "TRA", "MOA", "ATZ", "MATZ", "CTR", "RMZ", "TRSA",
          // Military designators
          "AAF", "AFB"
        });
  return atools::capString(str, {}, {}, ignore);
}

} // namespace util
} // namespace fs
} // namespace atools
