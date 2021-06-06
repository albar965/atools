/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

/* Code from Morix Dev on stackoverflow */

#ifndef ATOOLS_ZIP_GZIP_H
#define ATOOLS_ZIP_GZIP_H

#include "QtZlib/zlib.h"

class QByteArray;
class QString;

/* Gzip compression support functions
 * https://www.ietf.org/rfc/rfc1952.txt
 */
namespace atools {
namespace zip {

/* true if input stream is prefixed with Gzip magic number */
bool isGzipCompressed(const QString& filename);
bool isGzipCompressed(const QByteArray& bytes);

/**
 * @brief Compresses the given buffer using the standard GZIP algorithm
 * @param input The buffer to be compressed
 * @param output The result of the compression
 * @param level The compression level to be used (@c 0 = no compression, @c 9 = max, @c -1 = default)
 * @return @c true if the compression was successful, @c false otherwise
 */
bool gzipCompress(const QByteArray& input, QByteArray& output, int level = -1);
QByteArray gzipCompress(const QByteArray& input, int level = -1);

/**
 * @brief Decompresses the given buffer using the standard GZIP algorithm
 * @param input The buffer to be decompressed
 * @param output The result of the decompression
 * @return @c true if the decompression was successfull, @c false otherwise
 */
bool gzipDecompress(const QByteArray& input, QByteArray& output);
QByteArray gzipDecompress(const QByteArray& input);

} // namespace zip
} // namespace atools

#endif // ATOOLS_ZIP_GZIP_H
