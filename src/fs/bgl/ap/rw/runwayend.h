/*
 * RunwayEnd.h
 *
 *  Created on: 23.04.2015
 *      Author: alex
 */

#ifndef BGL_AP_RW_RUNWAYEND_H_
#define BGL_AP_RW_RUNWAYEND_H_

#include "runwayapplights.h"
#include "runwayvasi.h"
#include "../../converter.h"

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum Pattern
{
  LEFT = 0, RIGHT = 1
};

} // namespace rw

class RunwayEnd
{
public:
  RunwayEnd()
    : number(0), designator(0), offsetThreshold(0.0), blastPad(0.0), overrun(0.0), closedMarkings(false),
      stolMarkings(false), takeoff(false), landing(false), pattern(atools::fs::bgl::rw::LEFT)
  {
  }

  virtual ~RunwayEnd();

  QString getName() const
  {
    return converter::runwayToStr(number, designator);
  }

  const RunwayAppLights& getApproachLights() const
  {
    return approachLights;
  }

  int getBlastPad() const
  {
    return blastPad;
  }

  const RunwayVasi& getLeftVasi() const
  {
    return leftVasi;
  }

  int getOffsetThreshold() const
  {
    return offsetThreshold;
  }

  int getOverrun() const
  {
    return overrun;
  }

  const RunwayVasi& getRightVasi() const
  {
    return rightVasi;
  }

  bool hasClosedMarkings() const
  {
    return closedMarkings;
  }

  bool isLanding() const
  {
    return landing;
  }

  atools::fs::bgl::rw::Pattern getPattern() const
  {
    return pattern;
  }

  bool hasStolMarkings() const
  {
    return stolMarkings;
  }

  bool isTakeoff() const
  {
    return takeoff;
  }

  static QString patternToStr(atools::fs::bgl::rw::Pattern pattern);

private:
  friend class Runway;
  friend QDebug operator<<(QDebug out, const RunwayEnd& record);

  QString designatorStr(int designator) const;

  int number, designator;
  int offsetThreshold, blastPad, overrun;

  RunwayVasi leftVasi, rightVasi;

  RunwayAppLights approachLights;

  bool closedMarkings, stolMarkings, takeoff, landing;
  atools::fs::bgl::rw::Pattern pattern;

};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_RW_RUNWAYEND_H_ */
