/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_DB_LEGBASEWRITER_H
#define ATOOLS_FS_DB_LEGBASEWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/approachleg.h"

namespace atools {
namespace fs {
namespace db {

/*
 * Base for writing approach and transition legs (table schema the same - only name differs).
 */
class LegBaseWriter :
  public atools::fs::db::WriterBase<atools::fs::bgl::ApproachLeg>
{
public:
  LegBaseWriter(atools::sql::SqlDatabase& db,
                atools::fs::db::DataWriter& dataWriter,
                const QLatin1String& table);

protected:
  virtual void writeObject(const atools::fs::bgl::ApproachLeg *type) override;

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_LEGBASEWRITER_H
