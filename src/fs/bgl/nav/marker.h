/*
 * Marker.h
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#ifndef BGL_NAV_MARKER_H_
#define BGL_NAV_MARKER_H_

#include "../record.h"
#include "../bglposition.h"

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
  public Record
{
public:
  Marker(atools::io::BinaryStream *bs);
  virtual ~Marker();

  const QString& getIdent() const
  {
    return ident;
  }

  const BglPosition& getPosition() const
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
  friend QDebug operator<<(QDebug out, const Marker& record);

  atools::fs::bgl::nav::MarkerType type;
  BglPosition position;
  QString ident, region;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAV_MARKER_H_ */
