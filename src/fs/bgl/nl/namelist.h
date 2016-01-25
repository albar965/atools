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

#ifndef BGL_NAMELIST_H_
#define BGL_NAMELIST_H_

#include "fs/bgl/nl/namelistentry.h"
#include "fs/bgl/record.h"

#include <QString>
#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class Namelist :
  public atools::fs::bgl::Record
{
public:
  Namelist(atools::io::BinaryStream *bs);
  virtual ~Namelist();

  const QList<atools::fs::bgl::NamelistEntry>& getNameList() const
  {
    return entries;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Namelist& record);

  QList<atools::fs::bgl::NamelistEntry> entries;

  void readList(QStringList& names, atools::io::BinaryStream *bs, int numRegionNames, int regionListOffset);

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAMELIST_H_ */
