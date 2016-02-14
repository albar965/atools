/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef WRITER_AIRPORTWRITER_H_
#define WRITER_AIRPORTWRITER_H_

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/airport.h"
#include "fs/db/ap/deleteprocessor.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/bgl/nl/namelist.h"

#include <QHash>

namespace atools {
namespace fs {
namespace bgl {
class Namelist;
class NamelistEntry;
}
namespace db {

class AirportWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::Airport>
{
public:
  AirportWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
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

  void setSceneryLocalPath(const QString& value)
  {
    sceneryLocalPath = value;
  }

  void setBglFilename(const QString& value)
  {
    bglFilename = value;
  }

private:
  virtual void writeObject(const atools::fs::bgl::Airport *type) override;

  typedef QHash<QString, const atools::fs::bgl::NamelistEntry *> NameListMapType;
  typedef NameListMapType::const_iterator NameListMapConstIterType;
  NameListMapType nameListIndex;

  QString currentIdent, sceneryLocalPath, bglFilename;

  atools::fs::db::DeleteProcessor deleteProcessor;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_AIRPORTWRITER_H_ */
