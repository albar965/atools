/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_DB_APRONWRITER_H
#define ATOOLS_FS_DB_APRONWRITER_H

#include "fs/db/writerbase.h"
#include "fs/bgl/ap/apron.h"
#include "fs/bgl/ap/apron2.h"

namespace atools {
namespace fs {
namespace db {

/*
 * Write an apron dataset into the table. Source are two apron records
 */
class ApronWriter :
  public atools::fs::db::WriterBase<std::pair<const atools::fs::bgl::Apron *, const atools::fs::bgl::Apron2 *> >
{
public:
  ApronWriter(atools::sql::SqlDatabase& db, atools::fs::db::DataWriter& dataWriter)
    : WriterBase(db, dataWriter, "apron")
  {
  }

  virtual ~ApronWriter()
  {
  }

protected:
  virtual void writeObject(const std::pair<const bgl::Apron *, const bgl::Apron2 *> *type) override;

private:
  QByteArray toBytes(const QList<int>& triangles);
  QString toWkt(const QList<atools::fs::bgl::BglPosition>& vertices);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_APRONWRITER_H
