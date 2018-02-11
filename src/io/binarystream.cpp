/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

namespace atools {
namespace io {

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

qint64 BinaryStream::tellg() const
{
  checkStream("tellg");
  return is->device()->pos();
}

void BinaryStream::skip(qint64 bytes)
{
  checkStream("skip");
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

float BinaryStream::readFloat()
{
  union
  {
    int intValue;
    float floatValue;
  } u;

  u.intValue = readInt();

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

QString BinaryStream::readString()
{
  QString retval;
  char c = 0;
  while((c = readByte()) != 0)
  {
    if(iscntrl(c))
      break;
    retval.append(QChar::fromLatin1(c));
  }

  checkStream("readString");

  return retval;
}

QString BinaryStream::readString(int length)
{
  char *buf = new char[length];
  readBytes(buf, length);

  QString retval;
  for(int i = 0; i < length; i++)
  {
    if(iscntrl(buf[i]))
      break;
    retval.append(QChar::fromLatin1(buf[i]));
  }
  delete[] buf;
  return retval;
}

void BinaryStream::checkStream(const QString& what) const
{
  if(is->status() != QDataStream::Ok)
    throw Exception(QString("%1 for file \"%2\" failed. Reason %3").
                    arg(what).arg(getFilename()).arg(is->status()));
  // qDebug() << hex << "0x" << is->device()->pos() << dec;
}

} /* namespace io */
} // namespace atools
