/*
 * VorWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_VORWRITER_H_
#define WRITER_VORWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/nav/vor.h"

namespace atools {
namespace fs {
namespace writer {

class VorWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Vor>
{
public:
  VorWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "vor")
  {
  }

  virtual ~VorWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Vor *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_VORWRITER_H_ */
