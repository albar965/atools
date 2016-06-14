/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_BGL_BGLEXCEPTION_H
#define ATOOLS_BGL_BGLEXCEPTION_H

#include "exception.h"

#include <QString>

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class BglException :
  public atools::Exception
{
public:
  BglException(const QString& msg)
    : Exception(msg)
  {
  }

  BglException(const QString& msg, const atools::io::BinaryStream *bs);

  virtual ~BglException() Q_DECL_NOEXCEPT;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_BGLEXCEPTION_H
