/*
 * Localizer.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_LOCALIZER_H_
#define BGL_NAV_LOCALIZER_H_

#include "../record.h"
#include "../converter.h"

namespace atools {
namespace fs {
namespace bgl {

class Localizer :
  public Record
{
public:
  Localizer(atools::io::BinaryStream *bs);
  virtual ~Localizer();

  QString getRunwayName() const
  {
    return converter::runwayToStr(runwayNumber, runwayDesignator);
  }

  float getHeading() const
  {
    return heading;
  }

  float getWidth() const
  {
    return width;
  }

private:
  friend QDebug operator<<(QDebug out, const Localizer& record);

  int runwayNumber, runwayDesignator;
  float heading, width;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_LOCALIZER_H_ */
