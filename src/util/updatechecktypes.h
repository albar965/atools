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

#ifndef ATOOLS_UPDATECHECKTYPES_H
#define ATOOLS_UPDATECHECKTYPES_H

#include "util/flags.h"

#include <QString>

namespace atools {
namespace util {

/* Updates can be fetched for each channel in one call */
enum UpdateChannel : quint32
{
  NONE = 0,
  STABLE = 1 << 0,
  BETA = 1 << 1,
  DEVELOP = 1 << 2
};

ATOOLS_DECLARE_FLAGS_32(UpdateChannels, atools::util::UpdateChannel)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::util::UpdateChannels)

class Update
{
public:
  Update()
  {
  }

  Update(atools::util::UpdateChannels channelParam, const QString& versionParam, const QString& changelogParam)
    : channel(channelParam), version(versionParam), changelog(changelogParam)
  {
  }

  atools::util::UpdateChannel getChannel() const
  {
    return channel;
  }

  const QString& getVersion() const
  {
    return version;
  }

  const QString& getChangelog() const
  {
    return changelog;
  }

  void simplyfyChangelog()
  {
    changelog = changelog.simplified();
  }

  void setChannel(atools::util::UpdateChannel value)
  {
    channel = value;
  }

  void setVersion(const QString& value)
  {
    version = value;
  }

  void setChangelog(const QString& value)
  {
    changelog = value;
  }

  void appendChangelog(const QString& value)
  {
    changelog += value;
  }

private:
  atools::util::UpdateChannel channel; /* The used update channel */
  QString version, /* the offered version */
          changelog; /* HTML changelog */
};

typedef QList<atools::util::Update> Updates;

} // namespace util

} // namespace atools

#endif // ATOOLS_UPDATECHECKTYPES_H
