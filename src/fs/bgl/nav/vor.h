/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef ATOOLS_BGL_NAV_VOR_H
#define ATOOLS_BGL_NAV_VOR_H

#include "fs/bgl/nav/navbase.h"
#include "fs/bgl/nav/ilsvor.h"

namespace atools {
namespace fs {
namespace bgl {

class Dme;

/*
 * VOR/DME/VORDME record
 * Also reads MSFS 2024 TACAN
 */
class Vor :
  public atools::fs::bgl::NavBase
{
public:
  Vor(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *stream);
  virtual ~Vor() override;

  Vor(const Vor& other) = delete;
  Vor& operator=(const Vor& other) = delete;

  /*
   * @return get the DME record for this VOR if available - otherwise null
   */
  const atools::fs::bgl::Dme *getDme() const
  {
    return dme;
  }

  /*
   * @return true if only DME
   */
  bool isDmeOnly() const
  {
    return dmeOnly;
  }

  /*
   * @return VOR type which also indicates the range
   */
  atools::fs::bgl::nav::IlsVorType getType() const
  {
    return type;
  }

  /* true if MSFS TACAN */
  bool isTacan() const
  {
    return tacan;
  }

  /* true if MSFS VORTAC */
  bool isVortac() const
  {
    return vortac;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Vor& record);

  atools::fs::bgl::nav::IlsVorType type;
  bool dmeOnly, tacan, vortac;
  atools::fs::bgl::Dme *dme = nullptr;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_NAV_VOR_H
