/*
 * IlsWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_ILSWRITER_H_
#define WRITER_ILSWRITER_H_

#include "../writerbase.h"
#include "../../bgl/nav/ils.h"

namespace atools {
namespace fs {
namespace writer {

class IlsWriter :
  public WriterBase<atools::fs::bgl::Ils>
{
public:
  IlsWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "ils")
  {
  }

  virtual ~IlsWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Ils *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_ILSWRITER_H_ */
