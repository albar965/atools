/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_METADATAWRITER_H
#define ATOOLS_METADATAWRITER_H

#include <QString>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace common {

class MetadataWriter
{
public:
  MetadataWriter(atools::sql::SqlDatabase& sqlDb);
  virtual ~MetadataWriter();

  /* write to metadata file table */
  void writeFile(const QString& filepath, const QString& comment, int curSceneryId, int curFileId);
  void writeSceneryArea(const QString& filepath, const QString& sceneryName, int curSceneryId);

  void deInitQueries();
  void initQueries();

private:
  atools::sql::SqlDatabase& db;
  atools::sql::SqlQuery *insertFileQuery = nullptr, *insertSceneryQuery = nullptr;

};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_METADATAWRITER_H
