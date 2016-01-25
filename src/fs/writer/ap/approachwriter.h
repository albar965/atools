/*
 * ApproachWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_APPROACHWRITER_H_
#define WRITER_APPROACHWRITER_H_

#include "fs/writer//writerbase.h"
#include "fs/bgl/ap/approach.h"

namespace atools {
namespace fs {
namespace writer {

class ApproachWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Approach>
{
public:
  ApproachWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "approach")
  {
  }

  virtual ~ApproachWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Approach *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_APPROACHWRITER_H_ */
