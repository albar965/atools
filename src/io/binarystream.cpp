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

#include "io/binarystream.h"

#include <QFile>
#include <QDebug>
#include <QUuid>
#include <QFileInfo>
#include "exception.h"

namespace atools {
namespace io {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::hex;
using Qt::dec;
#endif

/* Default intel / little
 * Big endian 1A2B3C4D = 1A 2B 3C 4D in mem
 * Little endian 1A2B3C4D =  4D 3C 2B 1A in mem
 */
BinaryStream::BinaryStream(QFile *binaryFile, QDataStream::ByteOrder order)
  : file(binaryFile)
{
  this->is = new QDataStream(file);
  this->is->setByteOrder(order);
  checkStream("constructor");
}

BinaryStream::~BinaryStream()
{
  delete is;
}

quint32 BinaryStream::readUInt()
{
  quint32 retval;
  (*is) >> retval;

  checkStream("readInt");

  return retval;
}

int BinaryStream::readBytes(char bytes[], int size)
{
  int numRead = is->readRawData(bytes, size);
  checkStream("readBytes");
  return numRead;
}

int BinaryStream::readUBytes(unsigned char bytes[], int size)
{
  int numRead = is->readRawData(reinterpret_cast<char *>(bytes), size);
  checkStream("readBytes");
  return numRead;
}

QUuid BinaryStream::readUuid()
{
  uint l = readUInt();
  ushort w1, w2;
  w1 = readUShort();
  w2 = readUShort();

  unsigned char bytes[8];
  readUBytes(bytes, 8);
  return QUuid(l, w1, w2, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
}

qint64 BinaryStream::tellg() const
{
  checkStream("tellg");
  return is->device()->pos();
}

void BinaryStream::skip(qint64 bytes)
{
  checkStream("skip");
  if(bytes != 0)
    is->device()->seek(tellg() + bytes);
}

void BinaryStream::seekg(qint64 pos)
{
  checkStream("seekg");
  is->device()->seek(pos);
}

qint64 BinaryStream::getFileSize() const
{
  return file->size();
}

QString BinaryStream::getFilename() const
{
  return file->fileName();
}

QString BinaryStream::getFilenameOnly() const
{
  return QFileInfo(file->fileName()).fileName();
}

float BinaryStream::readFloat()
{
  // Needs special convertion from BGL float
  union
  {
    quint32 intValue;
    float floatValue;
  } u;

  u.intValue = readUInt();

  checkStream("readFloat");

  return u.floatValue;
}

quint16 BinaryStream::readUShort()
{
  quint16 retval;
  (*is) >> retval;

  checkStream("readShort");

  return retval;
}

quint8 BinaryStream::readUByte()
{
  quint8 retval;
  (*is) >> retval;

  checkStream("readByte");

  return retval;
}

qint32 BinaryStream::readInt()
{
  qint32 retval;
  (*is) >> retval;

  checkStream("readInt");

  return retval;
}

qint16 BinaryStream::readShort()
{
  qint16 retval;
  (*is) >> retval;

  checkStream("readShort");

  return retval;
}

qint8 BinaryStream::readByte()
{
  qint8 retval;
  (*is) >> retval;

  checkStream("readByte");

  return retval;
}

QChar BinaryStream::readChar()
{
  return QChar::fromLatin1(readByte());
}

QString BinaryStream::readString(Encoding encoding)
{
  QByteArray retval;
  char c = 0;
  do
  {
    c = readByte();
    retval.append(c);
  } while(c != '\0');

  checkStream("readString");

  if(encoding == UTF8)
    return QString::fromUtf8(retval);
  else if(encoding == LATIN1)
    return QString::fromLatin1(retval);
  else
    return QString::fromLocal8Bit(retval);
}

QString BinaryStream::readString(int length, Encoding encoding)
{
  char *buf = new char[static_cast<size_t>(length)];
  readBytes(buf, length);

  QByteArray retval;
  for(int i = 0; i < length; i++)
  {
    retval.append(buf[i]);
    if(buf[i] == '\0')
      break;
  }
  delete[] buf;

  if(encoding == UTF8)
    return QString::fromUtf8(retval);
  else if(encoding == LATIN1)
    return QString::fromLatin1(retval);
  else
    return QString::fromLocal8Bit(retval);
}

void BinaryStream::checkStream(const QString& what) const
{
  if(is->status() != QDataStream::Ok)
  {
    QString statusText(tr("Unknown"));
    switch(is->status())
    {
      case QDataStream::Ok:
        statusText = tr("No error");
        break;
      case QDataStream::ReadPastEnd:
        statusText = tr("Read past file end");
        break;
      case QDataStream::ReadCorruptData:
        statusText = tr("Read corrupted data");
        break;
      case QDataStream::WriteFailed:
        statusText = tr("Write failed");
        break;
    }

    QString msg = QString("%1 for file \"%2\" failed. Reason: %3 (%4).").arg(what).arg(getFilename()).arg(statusText).arg(is->status());

    qWarning() << msg << "Position" << hex << "0x" << is->device()->pos() << dec << is->device()->pos();
    throw Exception(msg);
  }
}

} /* namespace io */
} // namespace atools
