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

#ifndef ATOOLS_BGL_AP_DELETEAIRPORT_H
#define ATOOLS_BGL_AP_DELETEAIRPORT_H

#include "fs/bgl/record.h"
#include "fs/bgl/ap/del/deleterunway.h"
#include "fs/bgl/ap/del/deletecom.h"
#include "fs/bgl/ap/del/deletestart.h"

#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace del {
enum DeleteAllFlag
{
  // TODO missing not documented flags
  // deleteAllBlastFences = "TRUE"
  // deleteAllBoundaryFences = "TRUE"
  // deleteAllControlTowers = "TRUE"
  // deleteAllJetways = "TRUE"
  NONE = 0,
  APPROACHES = 1 << 0,
  APRONLIGHTS = 1 << 1,
  APRONS = 1 << 2,
  COMS = 1 << 3,
  HELIPADS = 1 << 4,
  RUNWAYS = 1 << 5,
  STARTS = 1 << 6,
  TAXIWAYS = 1 << 7
};

Q_DECLARE_FLAGS(DeleteAllFlags, DeleteAllFlag);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::bgl::del::DeleteAllFlags);

} // namespace del

/*
 * Delete airport record. Used by add-on airports to remove parts or all of default airports.
 * This class does not do any deletion it just reads the record. Deletion is done by the class
 * DeleteProcessor which does not support all cases.
 */
class DeleteAirport :
  public atools::fs::bgl::Record
{
public:
  DeleteAirport(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);
  virtual ~DeleteAirport();

  /*
   * Not supported since it is apparently not used
   * @return List of COM frequenies to delete
   */
  const QList<atools::fs::bgl::DeleteCom>& getDeleteComs() const
  {
    return deleteComs;
  }

  /*
   * Not supported since it is apparently not used
   * @return List of ruwnays to delete
   */
  const QList<atools::fs::bgl::DeleteRunway>& getDeleteRunways() const
  {
    return deleteRunways;
  }

  /*
   * @return Flags determine what airport facilities to delete
   */
  del::DeleteAllFlags getFlags() const
  {
    return flags;
  }

  /*
   * All delete flags are supported.
   * @return List of start positions to delete
   */
  const QList<atools::fs::bgl::DeleteStart>& getDeleteStarts() const
  {
    return deleteStarts;
  }

  static QString deleteAllFlagsToStr(atools::fs::bgl::del::DeleteAllFlags flags);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::DeleteAirport& record);

  atools::fs::bgl::del::DeleteAllFlags flags;
  int numRunways, numStarts, numFrequencies;

  QList<atools::fs::bgl::DeleteRunway> deleteRunways;
  QList<atools::fs::bgl::DeleteCom> deleteComs;
  QList<atools::fs::bgl::DeleteStart> deleteStarts;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_AP_DELETEAIRPORT_H
