/*
 * SceneryException.h
 *
 *  Created on: 30.03.2015
 *      Author: alex
 */

#include "bglexception.h"

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
