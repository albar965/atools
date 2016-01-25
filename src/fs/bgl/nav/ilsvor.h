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

#ifndef BGL_NAV_ILSVOR_H_
#define BGL_NAV_ILSVOR_H_

#include "fs/bgl/record.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {
enum IlsVorType
{
  TERMINAL = 0x0001,
  LOW = 0x0002,
  HIGH = 0x0003,
  ILS = 0x0004,
  VOT = 0x0005
};

} // namespace nav

class IlsVor :
  public atools::fs::bgl::Record
{
public:
  IlsVor(atools::io::BinaryStream *bs);
  virtual ~IlsVor();

  atools::fs::bgl::nav::IlsVorType getType() const
  {
    return type;
  }

  static QString ilsVorTypeToStr(atools::fs::bgl::nav::IlsVorType type);

private:
  atools::fs::bgl::nav::IlsVorType type;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_ILSVOR_H_ */
