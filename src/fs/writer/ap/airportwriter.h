/*
 * AirportWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_AIRPORTWRITER_H_
#define WRITER_AIRPORTWRITER_H_

#include "../writerbase.h"
#include "../../bgl/ap/airport.h"
#include "deleteprocessor.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/bgl/nl/namelist.h"

#include <QHash>

namespace atools {
namespace fs {
namespace bgl {
class Namelist;
class NamelistEntry;
}
namespace writer {

class AirportWriter :
  public WriterBase<atools::fs::bgl::Airport>
{
public:
  AirportWriter(atools::sql::SqlDatabase& db, DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "airport"), deleteProcessor(db, dataWriter)
  {
  }

  virtual ~AirportWriter()
  {
  }

  void setNameLists(const QList<const atools::fs::bgl::Namelist *>& namelists);

  const QString& getCurrentAirportIdent() const
  {
    return currentIdent;
  }

private:
  virtual void writeObject(const atools::fs::bgl::Airport *type);

  typedef QHash<QString, const atools::fs::bgl::NamelistEntry *> NameListMapType;
  typedef NameListMapType::const_iterator NameListMapConstIterType;
  NameListMapType nameListIndex;

  QString currentIdent;

  atools::fs::writer::DeleteProcessor deleteProcessor;
  static const int MIN_RUNWAY_LENGTH = 10;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AIRPORTWRITER_H_ */
