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

#ifndef ATOOLS_FS_XP_WRITER_H
#define ATOOLS_FS_XP_WRITER_H

#include <QString>

class QStringList;

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {

class NavDatabaseOptions;
class ProgressHandler;

namespace xp {

struct XpWriterContext
{
  int curFileId = -1;
  QString localPath, fileName;
  bool addOn = false;
};

class XpWriter
{
public:
  XpWriter(atools::sql::SqlDatabase& sqlDb,
           const atools::fs::NavDatabaseOptions& opts, atools::fs::ProgressHandler *progressHandler);
  virtual ~XpWriter();

  virtual void write(const QStringList& line, const XpWriterContext& context) = 0;
  virtual void finish() = 0;

protected:
  atools::sql::SqlDatabase& db;
  const atools::fs::NavDatabaseOptions& options;
  atools::fs::ProgressHandler *progress;
};

} // namespace xp
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_XP_WRITER_H
