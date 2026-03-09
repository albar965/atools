/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "util/xmlstreamwriter.h"

#include "exception.h"
#include "geo/linestring.h"
#include "geo/pos.h"

#include <QFileDevice>
#include <QDebug>
#include <QXmlStreamWriter>
#include <QColor>

namespace atools {
namespace util {

XmlStreamWriter::XmlStreamWriter(QIODevice *device, const QString& filenameParam)
  : filename(filenameParam)
{
  // Let the stream reader detect the encoding in the PI
  writer = new QXmlStreamWriter(device);
  writer->setAutoFormatting(true);
  writer->setAutoFormattingIndent(2);
  checkError();
}

XmlStreamWriter::XmlStreamWriter(QByteArray *data, const QString& filenameParam)
  : filename(filenameParam)
{
  writer = new QXmlStreamWriter(data);
  writer->setAutoFormatting(true);
  writer->setAutoFormattingIndent(2);
  checkError();
}

XmlStreamWriter::XmlStreamWriter(QString *data, const QString& filenameParam)
  : filename(filenameParam)
{
  writer = new QXmlStreamWriter(data);
  writer->setAutoFormatting(true);
  writer->setAutoFormattingIndent(2);
  checkError();
}

XmlStreamWriter::XmlStreamWriter(QXmlStreamWriter *streamwriter)
{
  writer = streamwriter;
  deleteWriter = false;
}

XmlStreamWriter::~XmlStreamWriter()
{
  if(deleteWriter)
    delete writer;
}

void XmlStreamWriter::writeStartDocument(const QString& topElementName, const QString& schema)
{
  writer->setAutoFormatting(true);
  writer->setAutoFormattingIndent(2);

  writer->writeStartDocument(QStringLiteral("1.0"));
  writer->writeStartElement(topElementName);

  // Schema namespace and reference to XSD ======================
  if(!schema.isEmpty())
  {
    writer->writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
    writer->writeAttribute(QStringLiteral("xsi:noNamespaceSchemaLocation"), schema);
  }

  checkError();
}

void XmlStreamWriter::writeEndDocument()
{
  writer->writeEndElement();
  writer->writeEndDocument();
  checkError();
}

void XmlStreamWriter::writeStartElement(const QString& name)
{
  writer->writeStartElement(name);
  checkError();
}

void XmlStreamWriter::writeEndElement()
{
  writer->writeEndElement();
  checkError();
}

void XmlStreamWriter::writeCharacters(const QString& text)
{
  writer->writeCharacters(text);
  checkError();
}

void XmlStreamWriter::writeTextElement(const QString& name, const QString& value)
{
  if(!value.isEmpty())
    writer->writeTextElement(name, value);
  checkError();
}

void XmlStreamWriter::writeTextElement(const QString& name, bool value)
{
  writer->writeTextElement(name, value ? QStringLiteral("true") : QStringLiteral("false"));
  checkError();
}

void XmlStreamWriter::writeTextElement(const QString& name, const QColor& value)
{
  writer->writeTextElement(name, value.name());
  checkError();
}

void XmlStreamWriter::writeTextElement(const QString& posName, const atools::geo::Pos& pos, bool writeAltitude)
{
  if(pos.isValid())
  {
    writer->writeStartElement(posName);
    writer->writeAttribute(QStringLiteral("Lon"), QString::number(pos.getLonX(), 'f', 6));
    writer->writeAttribute(QStringLiteral("Lat"), QString::number(pos.getLatY(), 'f', 6));
    if(writeAltitude)
      writer->writeAttribute(QStringLiteral("Alt"), QString::number(pos.getAltitude(), 'f', 2));
    writer->writeEndElement(); // posName
    checkError();
  }
}

void XmlStreamWriter::writeTextElement(const QString& lineName, const QString& posName,
                                       const geo::LineString& line,
                                       bool writeAltitude)
{
  if(!line.isEmpty())
  {
    writer->writeStartElement(lineName);
    for(const atools::geo::Pos& pos : std::as_const(line))
      writeTextElement(posName, pos, writeAltitude);
    writer->writeEndElement(); // lineName
    checkError();
  }
}

void XmlStreamWriter::writeAttribute(const QString& name, const QString& value)
{
  if(!value.isEmpty())
  {
    writer->writeAttribute(name, value);
    checkError();
  }
}

void XmlStreamWriter::writeAttribute(const QString& name, bool value)
{
  writeAttribute(name, value ? QStringLiteral("true") : QStringLiteral("false"));
  checkError();
}

void XmlStreamWriter::checkError()
{
  if(writer->hasError())
  {
    // Try to get filename for report
    QFileDevice *df = dynamic_cast<QFileDevice *>(writer->device());
    QString name = df != nullptr ? df->fileName() : QStringLiteral();

    QString msg = tr("Error writing \"%1\"").arg(name);
    qWarning() << Q_FUNC_INFO << msg;
    throw atools::Exception(msg);
  }
}

} // namespace util
} // namespace atools
