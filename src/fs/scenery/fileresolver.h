/*
 * FileResolver.h
 *
 *  Created on: 28.04.2015
 *      Author: alex
 */

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
  FileResolver(const BglReaderOptions& opts);
  virtual ~FileResolver();

  FileResolver& operator()(const QString& prefix)
  {
    return addExcludedFilePrefix(prefix);
  }

  FileResolver& addExcludedFilePrefix(const QString& prefix)
  {
    excludedPrefixes.push_back(prefix);
    return *this;
  }

  void clearExcludedFilePrefixes()
  {
    excludedPrefixes.clear();
  }

  void getFiles(const SceneryArea& area, QStringList& files) const;

private:
  bool matchesExcludedPrefix(const QString& fname) const;

  const BglReaderOptions& options;
  QStringList excludedPrefixes;
};

} // namespace scenery
} // namespace fs
} // namespace atools

#endif /* SCENERY_FILERESOLVER_H_ */
