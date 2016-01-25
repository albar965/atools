/*
 * SceneryAreaWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_SCENERYAREAWRITER_H_
#define WRITER_SCENERYAREAWRITER_H_

#include "fs/writer/writerbase.h"
#include "fs/scenery/sceneryarea.h"

namespace atools {
namespace fs {
namespace writer {

class SceneryAreaWriter :
  public atools::fs::writer::WriterBase<scenery::SceneryArea>
{
public:
  SceneryAreaWriter(atools::sql::SqlDatabase& db, atools::fs::writer::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "scenery_area")
  {
  }

  virtual ~SceneryAreaWriter()
  {
  }

protected:
  virtual void writeObject(const scenery::SceneryArea *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_SCENERYAREAWRITER_H_ */
