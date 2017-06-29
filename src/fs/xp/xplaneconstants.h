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

#ifndef ATOOLS_FS_XP_XPCONSTANTS_H
#define ATOOLS_FS_XP_XPCONSTANTS_H

namespace atools {
namespace fs {
namespace xp {

enum RowCode
{
  NDB = 2, /*  NDB (Non-Directional Beacon) Includes NDB component of Locator Outer Markers (LOM) */
  VOR = 3, /*  VOR (including VOR-DME and VORTACs) Includes VORs, VOR-DMEs, TACANs and VORTACs */
  LOC = 4, /*  Localizer component of an ILS (Instrument Landing System) */
  LOC_ONLY = 5, /*  Localizer component of a localizer-only approach Includes for LDAs and SDFs */
  GS = 6, /*  Glideslope component of an ILS Frequency shown is paired frequency, notthe DME channel */
  OM = 7, /*  Outer markers (OM) for an ILS Includes outer maker component of LOMs */
  MM = 8, /*  Middle markers (MM) for an ILS */
  IM = 9, /*  Inner markers (IM) for an ILS */
  DME = 12, /*  DME, including the DME component of an ILS, VORTAC or VOR-DME Paired frequency display suppressed on X-Plane’s charts */
  DME_ONLY = 13, /*  Stand-alone DME, orthe DME component of an NDB-DME Paired frequency will be displayed on X-Plane’s charts */

  /*  Unused below */
  SBAS_GBAS_FINAL = 14, /*  14 Final approach path alignment point of an SBAS or GBAS approach path Will not appear in X-Plane’s charts */
  GBAS = 15, /*  15 GBAS differential ground station of a GLS Will not appear in X-Plane’s charts */
  SBAS_GBAS_TRESHOLD = 16 /*  16 Landing threshold point or fictitious threshold point of an SBAS/GBAS approach Will */
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_XPCONSTANTS_H
