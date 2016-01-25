/*
 * Marker.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_MARKER_H_
#define BGL_NAV_MARKER_H_

#include "fs/bgl/record.h"
#include "fs/bgl/bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

namespace nav {

enum MarkerType
{
  INNER = 0,
  MIDDLE = 1,
  OUTER = 2,
  BACKCOURSE = 3
};

} // namespace nav

class Marker :
  public atools::fs::bgl::Record
{
public:
  Marker(atools::io::BinaryStream *bs);
  virtual ~Marker();

  const QString& getIdent() const
  {
    return ident;
  }

  const atools::fs::bgl::BglPosition& getPosition() const
  {
    return position;
  }

  const QString& getRegion() const
  {
    return region;
  }

  atools::fs::bgl::nav::MarkerType getType() const
  {
    return type;
  }

  static QString markerTypeToStr(atools::fs::bgl::nav::MarkerType type);

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Marker& record);

  atools::fs::bgl::nav::MarkerType type;
  atools::fs::bgl::BglPosition position;
  QString ident, region;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_MARKER_H_ */
