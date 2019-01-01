/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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

#include "fs/common/metadatawriter.h"

#include "sql/sqlquery.h"
#include "sql/sqlutil.h"

#include <QFileInfo>
#include <QDateTime>

namespace atools {
namespace fs {
namespace common {

MetadataWriter::MetadataWriter(sql::SqlDatabase& sqlDb)
  : db(sqlDb)
{
  initQueries();
}

MetadataWriter::~MetadataWriter()
{
  deInitQueries();
}

void MetadataWriter::writeFile(const QString& filepath, const QString& comment, int curSceneryId, int curFileId)
{
  QFileInfo fileinfo(filepath);

  insertFileQuery->bindValue(":bgl_file_id", curFileId);
  insertFileQuery->bindValue(":file_modification_time", fileinfo.lastModified().toTime_t());
  insertFileQuery->bindValue(":scenery_area_id", curSceneryId);
  insertFileQuery->bindValue(":bgl_create_time", fileinfo.lastModified().toTime_t());
  insertFileQuery->bindValue(":filepath", fileinfo.filePath());
  insertFileQuery->bindValue(":filename", fileinfo.fileName());
  insertFileQuery->bindValue(":size", fileinfo.size());
  insertFileQuery->bindValue(":comment", comment);
  insertFileQuery->exec();
}

void MetadataWriter::writeSceneryArea(const QString& filepath, const QString& sceneryName, int curSceneryId)
{
  insertSceneryQuery->bindValue(":scenery_area_id", curSceneryId);
  insertSceneryQuery->bindValue(":number", curSceneryId);
  insertSceneryQuery->bindValue(":layer", curSceneryId);
  insertSceneryQuery->bindValue(":title", sceneryName);
  insertSceneryQuery->bindValue(":local_path", filepath.isEmpty() ?
                                QVariant(QVariant::String) : QFileInfo(filepath).filePath());
  insertSceneryQuery->bindValue(":active", true);
  insertSceneryQuery->bindValue(":required", true);
  insertSceneryQuery->exec();
}

void MetadataWriter::initQueries()
{
  deInitQueries();

  atools::sql::SqlUtil util(&db);

  insertSceneryQuery = new atools::sql::SqlQuery(db);
  insertSceneryQuery->prepare(util.buildInsertStatement("scenery_area", QString(), {"remote_path", "exclude"}));

  insertFileQuery = new atools::sql::SqlQuery(db);
  insertFileQuery->prepare(util.buildInsertStatement("bgl_file"));
}

void MetadataWriter::deInitQueries()
{
  delete insertSceneryQuery;
  insertSceneryQuery = nullptr;

  delete insertFileQuery;
  insertFileQuery = nullptr;
}

} // namespace common
} // namespace fs
} // namespace atools
