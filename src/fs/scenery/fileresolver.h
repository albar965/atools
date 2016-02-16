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

#ifndef SCENERY_FILERESOLVER_H_
#define SCENERY_FILERESOLVER_H_

#include <QList>

namespace atools {
namespace fs {
class BglReaderOptions;
namespace scenery {

class SceneryArea;

class FileResolver
{
public:
  FileResolver(const atools::fs::BglReaderOptions& opts, bool noWarnings = false);
  virtual ~FileResolver();

  atools::fs::scenery::FileResolver& addExcludedFilePrefixes(const QStringList& prefixes);
  void clearExcludedFilePrefixes();

  void getFiles(const atools::fs::scenery::SceneryArea& area, QStringList& files) const;

private:
  bool matchesExcludedPrefix(const QString& fname) const;

  const atools::fs::BglReaderOptions& options;
  bool quiet = false;
  QStringList excludedPrefixes;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* SCENERY_FILERESOLVER_H_ */
