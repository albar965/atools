/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_LOGGINGTYPES_H
#define ATOOLS_LOGGINGTYPES_H

#include <QHash>

class QTextStream;
class QFile;

namespace atools {
namespace logging {
namespace internal {

/* Common internal types for logging */

/* Combines the file and use text stream which allows to exchange the file during logging. */
struct Channel
{
  QTextStream *stream;
  QFile *file; /* Null if this channel is stdou or stderr */
};

typedef  QVector<Channel *> ChannelVector;
typedef  QHash<QString, ChannelVector> ChannelMap;

} // namespace internal
} // namespace logging
} // namespace atools

#endif // ATOOLS_LOGGINGTYPES_H
