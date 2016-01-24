/*
 * ApproachTransWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_TRANSITIONWRITER_H_
#define WRITER_TRANSITIONWRITER_H_

#include "../../bgl/ap/transition.h"
#include "../writerbase.h"

namespace atools {
namespace fs {
namespace writer {

class TransitionWriter :
  public WriterBase<atools::fs::bgl::Transition>
{
public:
  TransitionWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "transition")
  {
  }

  virtual ~TransitionWriter()
  {
  }

protected:
  virtual void writeObject(const atools::fs::bgl::Transition *type);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_TRANSITIONWRITER_H_ */
