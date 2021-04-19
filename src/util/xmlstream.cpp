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

#include "util/xmlstream.h"
#include "exception.h"
#include "atools.h"

#include <QFileDevice>
#include <QDebug>
#include <QTextCodec>
#include <QXmlStreamReader>

namespace atools {
namespace util {

XmlStream::XmlStream(QIODevice *device, const QString& filenameParam)
  : filename(filenameParam)
{
  QTextCodec *codec = atools::codecForFile(*device);

  if(codec != nullptr)
  {
    // Load the file into a text file to avoid BOM / xml encoding mismatches
    QTextStream stream(device);
    stream.setCodec(codec);
    QString str = stream.readAll();

    // The reader ignores the XML encoding header when reading from a string
    reader = new QXmlStreamReader(str);
  }
  else
  {
    // Let the stream reader detect the encoding in the PI
    reader = new QXmlStreamReader(device);
  }
}

XmlStream::XmlStream(const QByteArray& data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStream::XmlStream(const QString& data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStream::XmlStream(const char *data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStream::~XmlStream()
{
  delete reader;
}

void XmlStream::readUntilElement(const QString& name)
{
  while(reader->name() != name)
    readNextStartElement();
}

bool XmlStream::readNextStartElement()
{
  bool retval = reader->readNextStartElement();
  checkError();
  return retval;
}

void XmlStream::checkError()
{
  if(reader->hasError())
  {
    // Try to get filename for report
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader->device());
    QString name = df != nullptr ? df->fileName() : QString();

    QString msg = tr("Error reading \"%1\" on line %2 column %3: %4").
                  arg(name).arg(reader->lineNumber()).arg(reader->columnNumber()).arg(reader->errorString());
    qWarning() << Q_FUNC_INFO << msg;
    throw atools::Exception(msg);
  }
}

void XmlStream::skipCurrentElement(bool warning)
{
  if(warning)
  {
    // Try to get filename for warning
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader->device());
    QString name = df != nullptr ? df->fileName() : QString();
    qWarning() << Q_FUNC_INFO << "Unexpected element" << reader->name()
               << "in file" << name << "in line" << reader->lineNumber();
  }
  reader->skipCurrentElement();
}

} // namespace util
} // namespace atools
