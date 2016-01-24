/*
 * ApproachTrans.h
 *
 *  Created on: 04.05.2015
 *      Author: alex
 */

#ifndef BGL_AP_TRANSITION_H_
#define BGL_AP_TRANSITION_H_

#include "../record.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace ap {

enum TransitionType
{
  APPR_TRANS_FULL = 1,
  APPR_TRANS_DME = 2
};

enum TransitionFixType
{
  APPR_VOR = 2,
  APPR_NDB = 3,
  APPR_TERMINAL_NDB = 4,
  APPR_WAYPOINT = 5,
  APPR_TERMINAL_WAYPOINT = 6
};

} // namespace ap

class Transition :
  public Record
{
public:
  Transition(atools::io::BinaryStream *bs);
  virtual ~Transition();

  const QString& getFixAirportIdent() const
  {
    return fixAirportIdent;
  }

  float getAltitude() const
  {
    return altitude;
  }

  const QString& getDmeAirportIdent() const
  {
    return dmeAirportIdent;
  }

  float getDmeDist() const
  {
    return dmeDist;
  }

  const QString& getDmeIdent() const
  {
    return dmeIdent;
  }

  int getDmeRadial() const
  {
    return dmeRadial;
  }

  const QString& getDmeRegion() const
  {
    return dmeRegion;
  }

  const QString& getFixRegion() const
  {
    return fixRegion;
  }

  int getNumLegs() const
  {
    return numLegs;
  }

  const QString& getTransFixIdent() const
  {
    return transFixIdent;
  }

  atools::fs::bgl::ap::TransitionFixType getTransFixType() const
  {
    return transFixType;
  }

  atools::fs::bgl::ap::TransitionType getType() const
  {
    return type;
  }

  static QString transitionTypeToStr(atools::fs::bgl::ap::TransitionType type);
  static QString transitionFixTypeToStr(atools::fs::bgl::ap::TransitionFixType type);

private:
  friend QDebug operator<<(QDebug out, const Transition& record);

  atools::fs::bgl::ap::TransitionType type;
  int numLegs;
  atools::fs::bgl::ap::TransitionFixType transFixType;
  QString transFixIdent, fixRegion, fixAirportIdent;
  float altitude;
  QString dmeIdent, dmeRegion, dmeAirportIdent;
  int dmeRadial;
  float dmeDist;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_TRANSITION_H_ */
