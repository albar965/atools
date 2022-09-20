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

#include "zip/gzip.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>

#define GZIP_WINDOWS_BIT 15 + 16
#define GZIP_CHUNK_SIZE 32 * 1024

namespace atools {
namespace zip {

bool gzipCompress(const QByteArray& input, QByteArray& output, int level)
{
  // Prepare output
  output.clear();

  // Is there something to do?
  if(!input.isEmpty())
  {
    // Declare vars
    int flush = 0;

    // Prepare deflater status
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    // Initialize deflater
    int ret = deflateInit2(&strm, qMax(-1, qMin(9, level)), Z_DEFLATED, GZIP_WINDOWS_BIT, 8, Z_DEFAULT_STRATEGY);

    if(ret != Z_OK)
      return false;

    // Prepare output
    output.clear();

    // Extract pointer to input data
    const char *input_data = input.constData();
    int input_data_left = input.length();

    // Compress data until available
    do
    {
      // Determine current chunk size
      int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);

      // Set deflater references
      strm.next_in = (unsigned char *)input_data;
      strm.avail_in = chunk_size;

      // Update interval variables
      input_data += chunk_size;
      input_data_left -= chunk_size;

      // Determine if it is the last chunk
      flush = (input_data_left <= 0 ? Z_FINISH : Z_NO_FLUSH);

      // Deflate chunk and cumulate output
      do
      {

        // Declare vars
        char out[GZIP_CHUNK_SIZE];

        // Set deflater references
        strm.next_out = (unsigned char *)out;
        strm.avail_out = GZIP_CHUNK_SIZE;

        // Try to deflate chunk
        ret = deflate(&strm, flush);

        // Check errors
        if(ret == Z_STREAM_ERROR)
        {
          // Clean-up
          deflateEnd(&strm);

          // Return
          return false;
        }

        // Determine compressed size
        int have = (GZIP_CHUNK_SIZE - strm.avail_out);

        // Cumulate result
        if(have > 0)
          output.append((char *)out, have);

      } while(strm.avail_out == 0);

    } while(flush != Z_FINISH);

    // Clean-up
    (void)deflateEnd(&strm);

    // Return
    return ret == Z_STREAM_END;
  }
  else
    return true;
}

bool gzipDecompress(const QByteArray& input, QByteArray& output)
{
  // Prepare output
  output.clear();

  // Is there something to do?
  if(input.length() > 0)
  {
    // Prepare inflater status
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    // Initialize inflater
    int ret = inflateInit2(&strm, GZIP_WINDOWS_BIT);

    if(ret != Z_OK)
      return false;

    // Extract pointer to input data
    const char *input_data = input.constData();
    int input_data_left = input.length();

    // Decompress data until available
    do
    {
      // Determine current chunk size
      int chunk_size = qMin(GZIP_CHUNK_SIZE, input_data_left);

      // Check for termination
      if(chunk_size <= 0)
        break;

      // Set inflater references
      strm.next_in = (unsigned char *)input_data;
      strm.avail_in = chunk_size;

      // Update interval variables
      input_data += chunk_size;
      input_data_left -= chunk_size;

      // Inflate chunk and cumulate output
      do
      {

        // Declare vars
        char out[GZIP_CHUNK_SIZE];

        // Set inflater references
        strm.next_out = (unsigned char *)out;
        strm.avail_out = GZIP_CHUNK_SIZE;

        // Try to inflate chunk
        ret = inflate(&strm, Z_NO_FLUSH);

        switch(ret)
        {
          case Z_NEED_DICT:
            ret = Z_DATA_ERROR;
            inflateEnd(&strm);
            return false;

          case Z_DATA_ERROR:
          case Z_MEM_ERROR:
          case Z_STREAM_ERROR:
            inflateEnd(&strm);
            return false;
        }

        // Determine decompressed size
        int have = (GZIP_CHUNK_SIZE - strm.avail_out);

        if(have < 0 || have > GZIP_CHUNK_SIZE)
        {
          qWarning() << Q_FUNC_INFO << "Invalid value \"have\" while decompressing" << have;
          output.clear();
          return false;
        }

        // Cumulate result
        if(have > 0)
          output.append((char *)out, have);

      } while(strm.avail_out == 0);

    } while(ret != Z_STREAM_END);

    // Clean-up
    inflateEnd(&strm);

    // Return
    return ret == Z_STREAM_END;
  }
  else
    return true;
}

bool isGzipCompressed(const QString& filename)
{
  if(QFile::exists(filename))
  {
    QFile file(filename);
    if(file.open(QIODevice::ReadOnly))
      return isGzipCompressed(file.read(10));
  }

  return false;
}

bool isGzipCompressed(const QByteArray& bytes)
{
  return bytes.size() >= 2 && bytes.at(0) == '\x1F' && bytes.at(1) == '\x8B';
}

QByteArray gzipCompress(const QByteArray& input, int level)
{
  QByteArray retval;
  if(!input.isEmpty() && gzipCompress(input, retval, level))
    return retval;
  else
    return QByteArray();
}

QByteArray gzipDecompress(const QByteArray& input)
{
  QByteArray retval;
  if(!input.isEmpty() && gzipDecompress(input, retval))
    return retval;
  else
    return QByteArray();
}

bool gzipDecompressIf(const QByteArray& input, QByteArray& output)
{
  if(isGzipCompressed(input))
    return gzipDecompress(input, output);
  else
  {
    output = input;
    return true;
  }
}

QByteArray gzipDecompressIf(const QByteArray& input, const QString& funcInfo)
{
  if(isGzipCompressed(input))
  {
    QByteArray retval;
    if(!atools::zip::gzipDecompress(input, retval))
    {
      qWarning() << funcInfo << "Error unzipping data";
      return QByteArray();
    }
    else
      // Now uncompressed
      return retval;
  }
  else
    // Not compressed
    return input;
}

} // namespace zip
} // namespace atools
