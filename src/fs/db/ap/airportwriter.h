/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_DB_AIRPORTWRITER_H
#define ATOOLS_FS_DB_AIRPORTWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/airport.h"
#include "fs/db/ap/deleteprocessor.h"
#include "fs/bgl/nl/namelistentry.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/db/datawriter.h"

#include <QHash>


namespace atools {
namespace fs {
namespace bgl {
class Namelist;
class NamelistEntry;
}
namespace db {

/*
 * Writes to airport and all subrecord tables like runway, runway_end or approach tables.
 * Also takes care about deletion of default/stock airports when saving an add-on airport.
 */
class AirportWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::Airport>
{
public:
  AirportWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "airport"), deleteProcessor(db, dataWriter.getOptions())
  {
  }

  virtual ~AirportWriter()
  {
  }

  /* Name list has to be set before so city, county and state can be saved. Called once per BGL file. */
  void setNameLists(const QList<const atools::fs::bgl::Namelist *>& namelists);

  const QString& getCurrentAirportIdent() const
  {
    return currentIdent;
  }

private:
  virtual void writeObject(const atools::fs::bgl::Airport *type) override;

  typedef QHash<QString, const atools::fs::bgl::NamelistEntry *> NameListMapType;
  typedef NameListMapType::const_iterator NameListMapConstIterType;
  /* Maps airport ICAO idents to NamelistEntrys */
  NameListMapType nameListIndex;

  QString currentIdent;

  atools::fs::db::DeleteProcessor deleteProcessor;
};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_AIRPORTWRITER_H
