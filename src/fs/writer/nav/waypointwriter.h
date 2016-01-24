/*
 * WaypointWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_WAYPOINTWRITER_H_
#define WRITER_WAYPOINTWRITER_H_

#include "../writerbase.h"
#include "../../bgl/nav/waypoint.h"

namespace atools {
namespace fs {
namespace writer {

class WaypointWriter :
  public WriterBase<atools::fs::bgl::Waypoint>
{
public:
  WaypointWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "waypoint")
  {
  }

  virtual ~WaypointWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Waypoint *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_WAYPOINTWRITER_H_ */
