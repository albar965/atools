/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FILEINFO_H
#define ATOOLS_FILEINFO_H

#include <QFileInfo>

namespace atools {
namespace io {

/*
 * Combines relevant information about files without the performance issues of file info.
 * Comparable and hashable.
 */
class FileInfo
{
public:
  explicit FileInfo(const QFileInfo& fileinfo)
    : filename(fileinfo.absoluteFilePath()), lastModified(fileinfo.lastModified().toSecsSinceEpoch()), size(fileinfo.size())
  {
  }

  explicit FileInfo(const QString& filename)
    : FileInfo(QFileInfo(filename))
  {
  }

  FileInfo()
    : lastModified(0LL), size(0LL)
  {
  }

  const QString& getFilename() const
  {
    return filename;
  }

  qint64 getLastModified() const
  {
    return lastModified;
  }

  qint64 getSize() const
  {
    return size;
  }

  bool operator==(const FileInfo& other) const
  {
    return filename == other.filename && lastModified == other.lastModified && size == other.size;
  }

  bool operator!=(const FileInfo& other) const
  {
    return !operator==(other);
  }

  void clear()
  {
    filename.clear();
    lastModified = size = 0LL;
  }

  bool isEmpty() const
  {
    return filename.isEmpty();
  }

private:
  QString filename;
  qint64 lastModified, size;
};

inline size_t qHash(const FileInfo& fileinfo, size_t seed)
{
  return qHashMulti(seed, fileinfo.getFilename(), fileinfo.getSize(), fileinfo.getLastModified());
}

inline QDebug operator<<(QDebug out, const FileInfo& info)
{
  QDebugStateSaver saver(out);
  out.noquote().nospace() << "FileInfo[" << info.getFilename()
                          << ", size " << info.getSize()
                          << ", last mod " << QDateTime::fromSecsSinceEpoch(info.getLastModified()) << "]";
  return out;
}

} // namespace io
} // namespace atools

#endif // ATOOLS_FILEINFO_H
