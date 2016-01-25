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

#include "fs/bgl/bglexception.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

BglException::BglException(const QString& msg, const atools::io::BinaryStream *bs)
  : Exception(QString("file \"%1\" at offset 0x%2. %3").
              arg(bs != nullptr ? bs->getFilename() : QString()).
              arg(bs != nullptr ? bs->tellg() : 0, 0, 16).
              arg(msg))
{
}

BglException::~BglException() noexcept
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
