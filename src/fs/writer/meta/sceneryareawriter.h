/*
 * SceneryAreaWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_SCENERYAREAWRITER_H_
#define WRITER_SCENERYAREAWRITER_H_

#include "../writerbase.h"
#include "../../scenery/sceneryarea.h"

namespace atools {
namespace fs {
namespace writer {

class SceneryAreaWriter :
  public WriterBase<scenery::SceneryArea>
{
public:
  SceneryAreaWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
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
