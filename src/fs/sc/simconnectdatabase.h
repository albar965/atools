/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_SC_SIMCONNECTDATABASE_H
#define ATOOLS_FS_SC_SIMCONNECTDATABASE_H

#include "fs/sc/simconnecttypes.h"

#include <QtGlobal>

class QIODevice;

namespace atools {
namespace fs {
namespace sc {

class SimConnectDataBase
{
public:
  SimConnectDataBase();
  virtual ~SimConnectDataBase();

  /*
   * @return Error status for last reading or writing call
   */
  atools::fs::sc::SimConnectStatus getStatus() const
  {
    return status;
  }

  /*
   * @return Error status text for last reading or writing call
   */
  QString getStatusText() const;

  static int writeBlock(QIODevice *ioDevice, const QByteArray& block, atools::fs::sc::SimConnectStatus& status);

  static void writeString(QDataStream& out, const QString& str);

  static bool readString(QDataStream & in, QString & str);
  static void writeLongString(QDataStream& out, const QString& str);

  static bool readLongString(QDataStream & in, QString & str);

protected:
  atools::fs::sc::SimConnectStatus status = OK;

};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectDataBase)

#endif // ATOOLS_FS_SC_SIMCONNECTDATABASE_H
