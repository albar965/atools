/*
 * ParkingWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_PARKINGWRITER_H_
#define WRITER_PARKINGWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/ap/parking.h"

namespace atools {
namespace fs {
namespace writer {

class ParkingWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Parking>
{
public:
  ParkingWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
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
