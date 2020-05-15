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

#include <QFileDevice>
#include <QDebug>

namespace atools {
namespace util {

void XmlStream::readUntilElement(const QString& name)
{
  while(reader.name() != name)
    readNextStartElement();
}

bool XmlStream::readNextStartElement()
{
  bool retval = reader.readNextStartElement();
  checkError(reader);
  return retval;
}

void XmlStream::checkError(QXmlStreamReader& reader)
{
  if(reader.hasError())
  {
    // Try to get filename for report
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader.device());
    QString filename = df != nullptr ? df->fileName() : QString();

    QString msg = tr("Error reading \"%1\" on line %2 column %3: %4").
                  arg(filename).arg(reader.lineNumber()).arg(reader.columnNumber()).arg(reader.errorString());
    qWarning() << Q_FUNC_INFO << msg;
    throw atools::Exception(msg);
  }
}

void XmlStream::skipCurrentElement(bool warning)
{
  if(warning)
  {
    // Try to get filename for warning
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader.device());
    QString filename = df != nullptr ? df->fileName() : QString();
    qWarning() << Q_FUNC_INFO << "Unexpected element" << reader.name()
               << "in file" << filename << "in line" << reader.lineNumber();
  }
  reader.skipCurrentElement();
}

} // namespace util
} // namespace atools
