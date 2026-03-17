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

#ifndef ATOOLS_XMLSTREAMWRITER_H
#define ATOOLS_XMLSTREAMWRITER_H

#include <QCoreApplication>

#include <QXmlStreamWriter>

class QIODevice;

namespace atools {

namespace geo {

class LineString;
class Pos;
}
namespace util {

/*
 * Provides a simple extension to XML stream writer. Methods do error checking and throw exception on error.
 * Initializes a QXmlStreamWriter depending on constructor and cannot be copied.
 */
class XmlStreamWriter
{
  Q_DECLARE_TR_FUNCTIONS(XmlStreamReader)

public:
  /* Detects encoding automatically independend on XML instruction and opens QXmlStreamReader.
   * Data must be UTF-8 or contain a BOM. */
  explicit XmlStreamWriter(QIODevice *device, const QString& filenameParam = QString());

  /* Opens QXmlStreamReader based on given source. Data has to be UTF-8.
   Constructs and deletes QXmlStreamReader. */
  explicit XmlStreamWriter(QByteArray *data, const QString& filenameParam = QString());
  explicit XmlStreamWriter(QString *data, const QString& filenameParam = QString());

  /* Uses given reader. Does not delete the stream reader. */
  explicit XmlStreamWriter(QXmlStreamWriter *streamWriter);

  ~XmlStreamWriter();

  XmlStreamWriter(const XmlStreamWriter& other) = delete;
  XmlStreamWriter& operator=(const XmlStreamWriter& other) = delete;

  /* Get underlying stream writer */
  QXmlStreamWriter& getWriter()
  {
    return *writer;
  }

  const QString& getFilename() const
  {
    return filename;
  }

  /* Write start of document with schema reference if given. Also writes the initial top level element. */
  void writeStartDocument(const QString& topElementName, const QString& schema = QString());
  void writeEndDocument();

  void writeStartElement(const QString& name);
  void writeEndElement();

  /* Write element text */
  void writeCharacters(const QString& text);

  /* Write element and text */
  void writeTextElement(const QString& name, const QString& value);

  /* Write element and text true/false */
  void writeTextElement(const QString& name, bool value);

  /* Write any numeric type that can be converted to a string */
  void writeTextElement(const QString& name, int value)
  {
    writeTextElement(name, QString::number(value));
  }

  void writeTextElement(const QString& name, long long value)
  {
    writeTextElement(name, QString::number(value));
  }

  void writeTextElement(const QString& name, float value)
  {
    writeTextElement(name, QString::number(value, 'f', 10));
  }

  void writeTextElement(const QString& name, double value)
  {
    writeTextElement(name, QString::number(value, 'f', 10));
  }

  void writeAttribute(const QString& name, const QString& value);

  /* Write attribute  and text true/false */
  void writeAttribute(const QString& name, bool value);

  void writeAttribute(const QString& name, int value)
  {
    writeAttribute(name, QString::number(value));
  }

  void writeAttribute(const QString& name, long long value)
  {
    writeAttribute(name, QString::number(value));
  }

  void writeAttribute(const QString& name, float value)
  {
    writeAttribute(name, QString::number(value, 'f', 10));
  }

  void writeAttribute(const QString& name, double value)
  {
    writeAttribute(name, QString::number(value, 'f', 10));
  }

#if defined(QT_WIDGETS_LIB)
  void writeTextElement(const QString& name, const QColor& value);

#endif

  /* Write pos element and attributes */
  void writeTextElement(const QString& posName, const atools::geo::Pos& pos, bool writeAltitude = false);

  /* Write element and pos child elements with attributes */
  void writeTextElement(const QString& lineName, const QString& posName, const atools::geo::LineString& line, bool writeAltitude = false);

  /* Write element and list of child elements with values */
  template<typename TYPE>
  void writeTextElement(const QString& listName, const QString& name, const QList<TYPE>& values)
  {
    if(!values.isEmpty())
    {
      writer->writeStartElement(listName);
      for(const TYPE& value : std::as_const(values))
        writeTextElement(name, value);
      writer->writeEndElement(); // listName
    }
  }

  /* Write element and list of child elements with values from hash. Does not write keys. */
  template<typename KEY, typename TYPE>
  void writeTextElement(const QString& listName, const QString& name, const QHash<KEY, TYPE>& values)
  {
    if(!values.isEmpty())
    {
      writer->writeStartElement(listName);
      for(const TYPE& value : std::as_const(values))
        writeTextElement(name, value);
      writer->writeEndElement(); // listName
    }
  }

private:
  /* Checks stream for error. Throws exception in case of error */
  void checkError();

  QXmlStreamWriter *writer = nullptr;
  bool deleteWriter = true;
  QString errorMsg = tr("Cannot open file %1. Reason: %2"), filename;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_XMLSTREAMWRITER_H
