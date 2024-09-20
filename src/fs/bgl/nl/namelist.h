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

#ifndef ATOOLS_BGL_NAMELIST_H
#define ATOOLS_BGL_NAMELIST_H

#include "fs/bgl/nl/namelistentry.h"
#include "fs/bgl/record.h"
#include "io/binarystream.h"

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

/*
 * Namelist contains all airport names, city, state/province and country names for the
 * airports in one BGL file
 */
class Namelist :
  public atools::fs::bgl::Record
{
public:
  /* read nameslist from BGL */
  Namelist(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);
  virtual ~Namelist() override;

  const QList<atools::fs::bgl::NamelistEntry>& getNameList() const
  {
    return entries;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Namelist& record);

  QList<atools::fs::bgl::NamelistEntry> entries;

  void readList(QStringList& names, atools::io::BinaryStream *bs, int numRegionNames, int regionListOffset,
                atools::io::Encoding encoding);

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAMELIST_H
