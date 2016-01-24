/*
 * RunwayWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_RUNWAYWRITER_H_
#define WRITER_RUNWAYWRITER_H_

#include "../../writerbase.h"
#include "../../../bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace writer {

class RunwayWriter :
  public WriterBase<atools::fs::bgl::Runway>
{
public:
  RunwayWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "runway")
  {
  }

  virtual ~RunwayWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Runway *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_RUNWAYWRITER_H_ */
