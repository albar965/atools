/*
 * NamelistRecord.h
 *
 *  Created on: 21.04.2015
 *      Author: alex
 */

#ifndef BGL_NAMELIST_H_
#define BGL_NAMELIST_H_

#include "namelistentry.h"
#include "../record.h"

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
  public Record
{
public:
  Namelist(atools::io::BinaryStream *bs);
  virtual ~Namelist();

  const QList<NamelistEntry>& getNameList() const
  {
    return entries;
  }

private:
  friend QDebug operator<<(QDebug out, const Namelist& record);

  QList<NamelistEntry> entries;

  void readList(QStringList& names, atools::io::BinaryStream *bs, int numRegionNames, int regionListOffset);

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAMELIST_H_ */
