/*
 * ParkingWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_PARKINGWRITER_H_
#define WRITER_PARKINGWRITER_H_

#include "../writerbase.h"
#include "../../bgl/ap/parking.h"

namespace atools {
namespace fs {
namespace writer {

class ParkingWriter :
  public WriterBase<atools::fs::bgl::Parking>
{
public:
  ParkingWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "parking")
  {
  }

  virtual ~ParkingWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Parking *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_PARKINGWRITER_H_ */
