/*
 * RunwayApproachLights.h
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#ifndef BGL_RUNWAYAPPLIGHTS_H_
#define BGL_RUNWAYAPPLIGHTS_H_

#include "fs/bgl/record.h"

#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum ApproachLightSystem
{
  NO_ALS = 0x00,
  ODALS = 0x01,
  MALSF = 0x02,
  MALSR = 0x03,
  SSALF = 0x04,
  SSALR = 0x05,
  ALSF1 = 0x06,
  ALSF2 = 0x07,
  RAIL = 0x08,
  CALVERT = 0x09,
  CALVERT2 = 0x0a,
  MALS = 0x0b,
  SALS = 0x0c,
  SSALS = 0x0d,
  SALSF = 0x0e // TODO check
};

} // namespace rw

class RunwayAppLights :
  public Record
{
public:
  RunwayAppLights()
    : system(atools::fs::bgl::rw::NO_ALS), endlights(false), reils(false), touchdown(false), numStrobes(0)
  {
  }

  RunwayAppLights(atools::io::BinaryStream *bs);

  virtual ~RunwayAppLights();

  bool hasEndlights() const
  {
    return endlights;
  }

  int getNumStrobes() const
  {
    return numStrobes;
  }

  bool hasReils() const
  {
    return reils;
  }

  atools::fs::bgl::rw::ApproachLightSystem getSystem() const
  {
    return system;
  }

  QString getSystemAsString() const
  {
    return appLightSystemToStr(system);
  }

  bool hasTouchdown() const
  {
    return touchdown;
  }

  static QString appLightSystemToStr(atools::fs::bgl::rw::ApproachLightSystem type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::RunwayAppLights& record);

  atools::fs::bgl::rw::ApproachLightSystem system;
  bool endlights;
  bool reils;
  bool touchdown;
  int numStrobes;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RUNWAYAPPLIGHTS_H_ */
