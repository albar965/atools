/*
 * AirportDeleteAirportWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_DELETEAIRPORTWRITER_H_
#define WRITER_DELETEAIRPORTWRITER_H_

#include "../writerbase.h"
#include "../../bgl/ap/del/deleteairport.h"

namespace atools {
namespace fs {
namespace writer {

class DeleteAirportWriter :
  public WriterBase<atools::fs::bgl::DeleteAirport>
{
public:
  DeleteAirportWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "delete_airport")
  {
  }

  virtual ~DeleteAirportWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::DeleteAirport *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_DELETEAIRPORTWRITER_H_ */
