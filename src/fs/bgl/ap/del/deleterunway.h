/*
 * DeleteRunwayX.h
 *
 *  Created on: 18.05.2015
 *      Author: alex
 */

#ifndef BGL_AP_DEL_DELETERUNWAY_H_
#define BGL_AP_DEL_DELETERUNWAY_H_

#include "../../bglbase.h"
#include "../rw/runway.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class DeleteRunway :
  public BglBase
{
public:
  DeleteRunway(atools::io::BinaryStream *bs);
  virtual ~DeleteRunway();

private:
  friend QDebug operator<<(QDebug out, const DeleteRunway& record);

  atools::fs::bgl::rw::Surface surface;
  QString primaryName;
  QString secondaryName;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_DEL_DELETERUNWAY_H_ */
