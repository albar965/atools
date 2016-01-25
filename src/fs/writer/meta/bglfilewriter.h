/*
 * BglFileWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_BGLFILEWRITER_H_
#define WRITER_BGLFILEWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/bgl/bglfile.h"

namespace atools {
namespace fs {
namespace writer {

class BglFileWriter :
  public atools::fs::writer::WriterBase<bgl::BglFile>
{
public:
  BglFileWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "bgl_file"), sceneryAreaId(0)
  {
  }

  virtual ~BglFileWriter()
  {
  }

protected:
  virtual void writeObject(const bgl::BglFile *type);

  int sceneryAreaId;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_BGLFILEWRITER_H_ */
