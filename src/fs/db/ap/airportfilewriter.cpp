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

#include "fs/db/ap/airportfilewriter.h"
#include "fs/db/meta/bglfilewriter.h"

namespace atools {
namespace fs {
namespace db {

void atools::fs::db::AirportFileWriter::writeObject(const atools::fs::bgl::Airport *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing airport file " << type->getIdent();

  bind(":airport_file_id", getNextId());
  bind(":file_id", getDataWriter().getBglFileWriter()->getCurrentId());
  bind(":ident", type->getIdent());

  executeStatement();
}

} // namespace db
} // namespace fs
} // namespace atools
