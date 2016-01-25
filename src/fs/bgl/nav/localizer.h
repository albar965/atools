/*
 * Localizer.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_LOCALIZER_H_
#define BGL_NAV_LOCALIZER_H_

#include "fs/bgl/record.h"

namespace atools {
namespace fs {
namespace bgl {

class Localizer :
  public atools::fs::bgl::Record
{
public:
  Localizer(atools::io::BinaryStream *bs);
  virtual ~Localizer();

  QString getRunwayName() const;

  float getHeading() const
  {
    return heading;
  }

  float getWidth() const
  {
    return width;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Localizer& record);

  int runwayNumber, runwayDesignator;
  float heading, width;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_LOCALIZER_H_ */
