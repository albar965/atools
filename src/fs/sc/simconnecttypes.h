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

#ifndef ATOOLS_SC_TYPES_H
#define ATOOLS_SC_TYPES_H

#include <QVector>
#include <QObject>

namespace atools {
namespace fs {
namespace sc {

enum SimConnectStatus
{
  OK, /* No error */
  INVALID_MAGIC_NUMBER, /* Packet data does not start with expected magic number */
  VERSION_MISMATCH, /* Client and server data version does not match for either data or reply */
  INSUFFICIENT_WRITE /* No enough data written */
};

const QVector<QString> SIMCONNECT_STATUS_TEXT =
{
  QObject::tr("No Error"), QObject::tr("Invalid magic number"),
  QObject::tr("Version mismatch"), QObject::tr("Insufficient write")
};

} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SC_TYPES_H
