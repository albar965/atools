/*
 * DeleteAirport.h
 *
 *  Created on: 27.04.2015
 *      Author: alex
 */

#ifndef BGL_AP_DELETEAIRPORT_H_
#define BGL_AP_DELETEAIRPORT_H_

#include "fs/bgl/record.h"
#include "fs/bgl/ap/del/deleterunway.h"
#include "fs/bgl/ap/del/deletecom.h"

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
enum DeleteAllFlags
{
  APPROACHES = 1 << 0,
  APRONLIGHTS = 1 << 1,
  APRONS = 1 << 2,
  COMS = 1 << 3,
  HELIPADS = 1 << 4,
  RUNWAYS = 1 << 5,
  STARTS = 1 << 6,
  TAXIWAYS = 1 << 7
};

} // namespace del

class DeleteAirport :
  public atools::fs::bgl::Record
{
public:
  DeleteAirport(atools::io::BinaryStream *bs);
  virtual ~DeleteAirport();

  const QList<atools::fs::bgl::DeleteCom>& getDeleteComs() const
  {
    return deleteComs;
  }

  const QList<atools::fs::bgl::DeleteRunway>& getDeleteRunways() const
  {
    return deleteRunways;
  }

  del::DeleteAllFlags getFlags() const
  {
    return flags;
  }

  static QString deleteAllFlagsToStr(atools::fs::bgl::del::DeleteAllFlags flags);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::DeleteAirport& record);

  atools::fs::bgl::del::DeleteAllFlags flags;
  int numRunways, numStarts, numFrequencies;

  QList<atools::fs::bgl::DeleteRunway> deleteRunways;
  QList<atools::fs::bgl::DeleteCom> deleteComs;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_DELETEAIRPORT_H_ */
