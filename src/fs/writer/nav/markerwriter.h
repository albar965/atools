/*
 * MarkerWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_MARKERWRITER_H_
#define WRITER_MARKERWRITER_H_

#include "../writerbase.h"
#include "../../bgl/nav/marker.h"

namespace atools {
namespace fs {
namespace writer {

class MarkerWriter :
  public WriterBase<atools::fs::bgl::Marker>
{
public:
  MarkerWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "marker")
  {
  }

  virtual ~MarkerWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Marker *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_MARKERWRITER_H_ */
