/*
 * AirportCom.h
 *
 *  Created on: 23.04.2015
 *      Author: alex
 */

#ifndef BGL_AIRPORTCOM_H_
#define BGL_AIRPORTCOM_H_

#include "../record.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace com {
enum ComType
{
  NONE = 0x0000,
  ATIS = 0x0001,
  MULTICOM = 0x0002,
  UNICOM = 0x0003,
  CTAF = 0x0004,
  GROUND = 0x0005,
  TOWER = 0x0006,
  CLEARANCE = 0x0007,
  APPROACH = 0x0008,
  DEPARTURE = 0x0009,
  CENTER = 0x000A,
  FSS = 0x000B,
  AWOS = 0x000C,
  ASOS = 0x000D,
  CLEARANCE_PRE_TAXI = 0x000E,
  REMOTE_CLEARANCE_DELIVERY = 0x000F
};

} // namespace com

class Com :
  public Record
{
public:
  Com()
    : type(atools::fs::bgl::com::NONE), frequency(0.0f)
  {
  }

  Com(atools::io::BinaryStream *bs);

  virtual ~Com();

  int getFrequency() const
  {
    return frequency;
  }

  const QString& getName() const
  {
    return name;
  }

  com::ComType getType() const
  {
    return type;
  }

  QString getTypeAsString() const
  {
    return comTypeToStr(type);
  }

  static QString comTypeToStr(atools::fs::bgl::com::ComType type);

private:
  friend QDebug operator<<(QDebug out, const Com& record);

  atools::fs::bgl::com::ComType type;
  int frequency;
  QString name;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AIRPORTCOM_H_ */
