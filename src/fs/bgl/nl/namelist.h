/*
 * NamelistRecord.h
 *
 *  Created on: 21.04.2015
 *      Author: alex
 */

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
