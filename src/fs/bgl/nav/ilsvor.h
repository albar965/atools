/*
 * IlsVor.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

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
