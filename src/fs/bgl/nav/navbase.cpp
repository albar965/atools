/*
 * NavBase.cpp
 *
 *  Created on: 27.04.2015
 *      Author: alex
 */

#include "navbase.h"

namespace atools {
namespace fs {
namespace bgl {

QDebug operator<<(QDebug out, const NavBase& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " NavBase[name " << record.name
  << ", ident " << record.ident
  << ", region " << record.region
  << ", airport ID " << record.airportIdent
  << ", frequency " << record.frequency
  << ", " << record.position
  << ", range " << record.range
  << ", magVar " << record.magVar;
  out << "]";
  return out;
}

NavBase::~NavBase()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
