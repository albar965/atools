/*
 * NdbWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_NDBWRITER_H_
#define WRITER_NDBWRITER_H_

#include "../writerbase.h"
#include "../../bgl/nav/ndb.h"

namespace atools {
namespace fs {
namespace writer {

class NdbWriter :
  public WriterBase<atools::fs::bgl::Ndb>
{
public:
  NdbWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
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
