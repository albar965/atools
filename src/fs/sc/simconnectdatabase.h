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
  const QString& getStatusText() const
  {
    return SIMCONNECT_STATUS_TEXT.at(status);
  }

  static int writeBlock(QIODevice *ioDevice, const QByteArray& block,
                        atools::fs::sc::SimConnectStatus& status);

protected:
  void writeString(QDataStream& out, const QString& str) const;
  bool readString(QDataStream& in, QString& str, quint16 *size = nullptr);

  atools::fs::sc::SimConnectStatus status = OK;
};

} // namespace sc
} // namespace fs
} // namespace atools

Q_DECLARE_METATYPE(atools::fs::sc::SimConnectDataBase);

Q_DECLARE_TYPEINFO(atools::fs::sc::SimConnectDataBase, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_FS_SC_SIMCONNECTDATABASE_H
