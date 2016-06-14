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

#ifndef ATOOLS_IO_BINARYSTREAM_H
#define ATOOLS_IO_BINARYSTREAM_H

#include "exception.h"

#include <QDataStream>

class QFile;

namespace atools {
namespace io {

/*
 * Simple wrapper for binary file reading around QDataStream
 * that will throw an Exception in case of
 * errors.
 */
class BinaryStream
{
public:
  BinaryStream(QFile *binaryFile, QDataStream::ByteOrder order = QDataStream::LittleEndian);
  virtual ~BinaryStream();

  qint8 readByte();
  qint16 readShort();
  qint32 readInt();

  quint8 readUByte();
  quint16 readUShort();
  quint32 readUInt();

  float readFloat();

  /* reads a null terminated latin-1 string and also stops reading at any
   * control characters */
  QString readString();

  /* Reads a latin-1 string of the given length terminating before at null or
   * control characters */
  QString readString(int length);

  /* Reads a single byte as a latin-1 character */
  QChar readChar();

  int readBytes(char bytes[], int size);

  qint64 tellg() const;
  void skip(qint64 bytes);
  void seekg(qint64 pos);

  qint64 getFileSize() const;
  QString getFilename() const;

private:
  void checkStream(const QString& what) const;

  QDataStream *is;
  QFile *file;
};

} /* namespace io */
} // namespace atools

#endif // ATOOLS_IO_BINARYSTREAM_H
