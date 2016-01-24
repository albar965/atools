/*
 * RunwayEndWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_RUNWAYENDWRITER_H_
#define WRITER_RUNWAYENDWRITER_H_

#include "../../writerbase.h"
#include "../../../bgl/ap/rw/runwayend.h"

namespace atools {
namespace fs {
namespace writer {

class RunwayEndWriter :
  public WriterBase<atools::fs::bgl::RunwayEnd>
{
public:
  RunwayEndWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "runway_end")
  {
  }

  virtual ~RunwayEndWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::RunwayEnd *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_RUNWAYENDWRITER_H_ */
