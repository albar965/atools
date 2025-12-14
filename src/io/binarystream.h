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

#ifndef ATOOLS_IO_BINARYSTREAM_H
#define ATOOLS_IO_BINARYSTREAM_H

#include <QDataStream>
#include <QCoreApplication>

class QFile;

namespace atools {
namespace io {

enum Encoding
{
  UTF8,
  LATIN1
};

/*
 * Simple wrapper for binary file reading around QDataStream
 * that will throw an Exception in case of
 * errors.
 */
class BinaryStream
{
  Q_DECLARE_TR_FUNCTIONS(BinaryStream)

public:
  BinaryStream(QFile*binaryFile, QDataStream::ByteOrder order = QDataStream::LittleEndian);

  BinaryStream(const BinaryStream& other) = delete;
  BinaryStream& operator=(const BinaryStream& other) = delete;

  qint8 readByte();
  qint16 readShort();
  qint32 readInt();
  qint64 readLong();

  quint8 readUByte();
  quint16 readUShort();
  quint32 readUInt();
  quint64 readULong();

  float readFloat();

  /* reads a null terminated latin-1 or UTF-8 string and also stops reading at NUL */
  QString readString(Encoding encoding);

  /* Reads a latin-1 or UTF-8 string of the given length terminating at NUL */
  QString readString(int length, Encoding encoding);

  /* Reads a single byte as a latin-1 character */
  QChar readChar();

  int readBytes(char bytes[], int size);
  int readUBytes(unsigned char bytes[], int size);

  /* Reads 16 bytes like 38EA37B0-F8ED-E54A-B41B-2CA423ADA3EF into UUID
   *  {B037EA38-EDF8-4AE5-B41B-2CA423ADA3EF} */
  QUuid readUuid();

  qint64 tellg() const;
  void skip(qint64 bytes);
  void seekg(qint64 pos);

  qint64 getFileSize() const
  {
    return filesize;
  }

  /* Returns full file name and path */
  QString getFilepath() const
  {
    return filename;
  }

  /* Returns file name without path */
  QString getFilename() const;

private:
  void checkStream(const QString& what) const;

  QDataStream is;
  QString filename;
  qint64 filesize;
};

} /* namespace io */
} // namespace atools

#endif // ATOOLS_IO_BINARYSTREAM_H
