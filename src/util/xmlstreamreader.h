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

#ifndef ATOOLS_XMLSTREAMREADER_H
#define ATOOLS_XMLSTREAMREADER_H

#include <QCoreApplication>
#include <QXmlStreamReader>
#if defined(QT_WIDGETS_LIB)
#include <QColor>
#endif

class QIODevice;

namespace atools {

namespace geo {

class LineString;
class Pos;
}
namespace util {

/*
 * Provides a simple extension to XML stream reader. Methods do error checking and throw exception on error.
 * Initializes a QXmlStreamReader depending on constructor sand cannot be copied.
 */
class XmlStreamReader
{
  Q_DECLARE_TR_FUNCTIONS(XmlStreamReader)

public:
  /* Detects encoding automatically independend on XML instruction and opens QXmlStreamReader.
   * Data must be UTF-8 or contain a BOM. */
  explicit XmlStreamReader(QIODevice *device, const QString& filenameParam = QString());

  /* Opens QXmlStreamReader based on given source. Data has to be UTF-8.
   Constructs and deletes QXmlStreamReader. */
  explicit XmlStreamReader(const QByteArray& data, const QString& filenameParam = QString());
  explicit XmlStreamReader(const QString& data, const QString& filenameParam = QString());
  explicit XmlStreamReader(const char *data, const QString& filenameParam = QString());

  /* Uses given reader. Does not delete the stream reader. */
  explicit XmlStreamReader(QXmlStreamReader *streamReader);

  ~XmlStreamReader();

  XmlStreamReader(const XmlStreamReader& other) = delete;
  XmlStreamReader& operator=(const XmlStreamReader& other) = delete;

  /* Read until element with given name. Throws exception in case of error */
  void readUntilElement(const QString& name);

  /* Read until next element and checks error. Throws exception in case of error */
  bool readNextStartElement();

  /* Skip element and optionally print a warning about unexpected elements with the name of the current. */
  void skipCurrentElement(bool warning = false);

  /* Reads values from element text and prints a warning if the values are not correct. */
  QString readElementTextStr()
  {
    return reader->readElementText();
  }

  /* Reads element text of current and returns type */
  bool readElementTextBool();
  int readElementTextInt();
  float readElementTextFloat();
  double readElementTextDouble();

  /* Read #RRGGBB color description */
#if defined(QT_WIDGETS_LIB)
  QColor readElementTextColor()
  {
    return QColor::fromString(readElementTextStr());
  }

#endif

  /* Reads Pos attributes from current element and skips it */
  atools::geo::Pos readElementPos();

  /* Reads a chain of Pos child elements and attributes from current element */
  atools::geo::LineString readElementLineString(const QString& entryName);

  /* Reads a chain of element texts with the give name from current element */
  QList<int> readElementListInt(const QString& entryName);
  QList<float> readElementListFloat(const QString& entryName);
  QList<double> readElementListDouble(const QString& entryName);
  QStringList readElementListStr(const QString& entryName);

  /* As above for attributes */
  QString readAttributeStr(const QString& name, const QString& defaultValue = QString());
  bool readAttributeBool(const QString& name, bool defaultValue = false);
  int readAttributeInt(const QString& name, int defaultValue = 0);
  float readAttributeFloat(const QString& name, float defaultValue = 0.f);
  double readAttributeDouble(const QString& name, double defaultValue = 0.);

  /* true if current element has attibute */
  bool hasAttribute(const QString& name) const
  {
    return reader->attributes().hasAttribute(name);
  }

  /* Returns the local name of a StartElement, EndElemen */
  QStringView name() const
  {
    return reader->name();
  }

  /* Get underlying stream reader */
  QXmlStreamReader *getReader()
  {
    return reader;
  }

  const QString& getFilename() const
  {
    return filename;
  }

private:
  /* Checks stream for error. Throws exception in case of error */
  void checkError();

  QXmlStreamReader *reader = nullptr;
  bool deleteReader = true;
  QString errorMsg = tr("Cannot open file %1. Reason: %2"), filename;
};

} // namespace util
} // namespace atools

#endif // ATOOLS_XMLSTREAMREADER_H
