/*
 * NdbWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_NDBWRITER_H_
#define WRITER_NDBWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/nav/ndb.h"

namespace atools {
namespace fs {
namespace writer {

class NdbWriter :
  public atools::fs::writer::WriterBase<atools::fs::bgl::Ndb>
{
public:
  NdbWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "ndb")
  {
  }

  virtual ~NdbWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Ndb *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_NDBWRITER_H_ */
