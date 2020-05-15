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

#ifndef ATOOLS_XMLSTRAM_H
#define ATOOLS_XMLSTRAM_H

#include <QXmlStreamReader>
#include <QApplication>

namespace atools {
namespace util {

/*
 * Provides a simple extension to XML stream reader. Methods do error checking and throw exception on error.
 * Initializes a QXmlStreamReader on construction and cannot be copied.
 */
class XmlStream
{
  /* Forbid copying */
  XmlStream(const XmlStream&) = delete;
  XmlStream& operator=(const XmlStream&) = delete;

  Q_DECLARE_TR_FUNCTIONS(XmlTools)

public:
  XmlStream()
  {

  }

  explicit XmlStream(QIODevice *device)
    : reader(device)
  {

  }

  explicit XmlStream(const QByteArray& data)
    : reader(data)
  {

  }

  explicit XmlStream(const QString& data)
    : reader(data)
  {

  }

  explicit XmlStream(const char *data)
    : reader(data)
  {

  }

  /* Read until element with given name. Throws exception in case of error */
  void readUntilElement(const QString& name);

  /* Read until next element and checks error. Throws exception in case of error */
  bool readNextStartElement();

  /* Skip element and optionally print a warning about unexpected elements */
  void skipCurrentElement(bool warning = false);

  /* Checks stream for error. Throws exception in case of error */
  void checkError(QXmlStreamReader& reader);

  /* Get underlying constructed stream reader */
  QXmlStreamReader& getReader()
  {
    return reader;
  }

private:
  QXmlStreamReader reader;
  QString errorMsg = tr("Cannot open file %1. Reason: %2");

};

} // namespace util
} // namespace atools

#endif // ATOOLS_XMLSTRAM_H
