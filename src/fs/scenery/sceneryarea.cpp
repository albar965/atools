/*
 * SceneryEntry.cpp
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#include "fs/scenery/sceneryarea.h"

#include <QDir>
#include "logging/loggingdefs.h"


namespace atools {
namespace fs {
namespace scenery {

const QString SceneryArea::getLocalPath() const
{
  return QDir::toNativeSeparators(localPath);
}

QDebug operator<<(QDebug out, const SceneryArea& area)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << "SceneryCfg["
  << "areaNumber " << area.areaNumber
  << ", title " << area.title
  << ", layer " << area.layer
  << ", active " << area.active
  << ", required " << area.required
  << ", localPath " << area.localPath
  << ", textureId " << area.textureId
  << ", remotePath" << area.remotePath
  << ", exclude " << area.exclude
  << "]";
  return out;
}

SceneryArea::~SceneryArea()
{
}

} // namespace scenery
} // namespace fs
} // namespace atools
