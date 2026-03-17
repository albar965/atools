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

#include "util/xmlstreamreader.h"

#include "exception.h"
#include "geo/linestring.h"
#include "geo/pos.h"

#include <QFileDevice>
#include <QDebug>
#include <QXmlStreamReader>

namespace atools {
namespace util {

XmlStreamReader::XmlStreamReader(QIODevice *device, const QString& filenameParam)
  : filename(filenameParam)
{
  // Let the stream reader detect the encoding in the PI
  reader = new QXmlStreamReader(device);
}

XmlStreamReader::XmlStreamReader(const QByteArray& data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStreamReader::XmlStreamReader(const QString& data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStreamReader::XmlStreamReader(const char *data, const QString& filenameParam)
  : filename(filenameParam)
{
  reader = new QXmlStreamReader(data);
}

XmlStreamReader::XmlStreamReader(QXmlStreamReader *streamReader)
{
  reader = streamReader;
  deleteReader = false;
}

XmlStreamReader::~XmlStreamReader()
{
  if(deleteReader)
    delete reader;
}

void XmlStreamReader::readUntilElement(const QString& name)
{
  while(reader->name() != name)
    readNextStartElement();
}

bool XmlStreamReader::readNextStartElement()
{
  bool retval = reader->readNextStartElement();
  checkError();
  return retval;
}

void XmlStreamReader::checkError()
{
  if(reader->hasError())
  {
    // Try to get filename for report
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader->device());
    QString name = df != nullptr ? df->fileName() : QStringLiteral();

    QString msg = tr("Error reading \"%1\" on line %2 column %3: %4").
                  arg(name).arg(reader->lineNumber()).arg(reader->columnNumber()).arg(reader->errorString());
    qWarning() << Q_FUNC_INFO << msg;
    throw atools::Exception(msg);
  }
}

void XmlStreamReader::skipCurrentElement(bool warning)
{
  if(warning)
  {
    // Try to get filename for warning
    QFileDevice *df = dynamic_cast<QFileDevice *>(reader->device());
    QString name = df != nullptr ? df->fileName() : QStringLiteral();
    qWarning() << Q_FUNC_INFO << "Unexpected element" << reader->name() << "in file" << name << "in line" << reader->lineNumber();
  }

  reader->skipCurrentElement();
}

bool XmlStreamReader::readElementTextBool()
{
  QString str = reader->readElementText().simplified().toLower();
  if(str == QStringLiteral("yes") || str == QStringLiteral("true") || str == QStringLiteral("y") || str == QStringLiteral("t") ||
     str == QStringLiteral("1"))
    return true;
  else if(str == QStringLiteral("no") || str == QStringLiteral("false") || str == QStringLiteral("n") || str == QStringLiteral("f") ||
          str == QStringLiteral("0"))
    return false;

  qWarning() << Q_FUNC_INFO << "Reading of bool value failed. File"
             << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return false;
}

int XmlStreamReader::readElementTextInt()
{
  bool ok;
  int retval = reader->readElementText().toInt(&ok);

  if(!ok)
    qWarning() << Q_FUNC_INFO << "Reading of int value failed. File"
               << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return retval;

}

float XmlStreamReader::readElementTextFloat()
{
  bool ok;
  float retval = reader->readElementText().toFloat(&ok);

  if(!ok)
    qWarning() << Q_FUNC_INFO << "Reading of float value failed. File"
               << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return retval;
}

double XmlStreamReader::readElementTextDouble()
{
  bool ok;
  double retval = reader->readElementText().toDouble(&ok);

  if(!ok)
    qWarning() << Q_FUNC_INFO << "Reading of double value failed. File"
               << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return retval;
}

atools::geo::Pos XmlStreamReader::readElementPos()
{
  atools::geo::Pos pos = atools::geo::Pos(readAttributeFloat(QStringLiteral("Lon")), readAttributeFloat(QStringLiteral("Lat")),
                                          readAttributeFloat(QStringLiteral("Alt"), 0.f));
  skipCurrentElement();
  return pos;
}

atools::geo::LineString XmlStreamReader::readElementLineString(const QString& entryName)
{
  atools::geo::LineString values;

  while(readNextStartElement())
  {
    if(name() == entryName)
      values.append(readElementPos());
    else
      skipCurrentElement();
  }
  return values;
}

QList<int> XmlStreamReader::readElementListInt(const QString& entryName)
{
  QList<int> values;

  while(readNextStartElement())
  {
    if(name() == entryName)
      values.append(readElementTextInt());
    else
      skipCurrentElement();
  }
  return values;
}

QList<float> XmlStreamReader::readElementListFloat(const QString& entryName)
{
  QList<float> values;

  while(readNextStartElement())
  {
    if(name() == entryName)
      values.append(readElementTextFloat());
    else
      skipCurrentElement();
  }
  return values;
}

QList<double> XmlStreamReader::readElementListDouble(const QString& entryName)
{
  QList<double> values;

  while(readNextStartElement())
  {
    if(name() == entryName)
      values.append(readElementTextDouble());
    else
      skipCurrentElement();
  }
  return values;
}

QStringList XmlStreamReader::readElementListStr(const QString& entryName)
{
  QStringList values;

  while(readNextStartElement())
  {
    if(name() == entryName)
      values.append(readElementTextStr());
    else
      skipCurrentElement();
  }
  return values;
}

QString XmlStreamReader::readAttributeStr(const QString& name, const QString& defaultValue)
{
  if(reader->attributes().hasAttribute(name))
    return reader->attributes().value(name).toString();
  else
    return defaultValue;
}

bool XmlStreamReader::readAttributeBool(const QString& name, bool defaultValue)
{
  if(reader->attributes().hasAttribute(name))
  {
    QString str = reader->attributes().value(name).toString().simplified();
    if(str == QStringLiteral("yes") || str == QStringLiteral("true") || str == QStringLiteral("y") || str == QStringLiteral("t") ||
       str == QStringLiteral("1"))
      return true;
    else if(str == QStringLiteral("no") || str == QStringLiteral("false") || str == QStringLiteral("n") || str == QStringLiteral("f") ||
            str == QStringLiteral("0"))
      return false;

    qWarning() << Q_FUNC_INFO << "Reading of bool value failed. File"
               << filename << "element" << reader->name() << "attribute" << name << "line" << reader->lineNumber();
    return defaultValue;
  }
  else
    return defaultValue;
}

int XmlStreamReader::readAttributeInt(const QString& name, int defaultValue)
{
  if(reader->attributes().hasAttribute(name))
  {
    bool ok;
    int value = reader->attributes().value(name).toInt(&ok);

    if(!ok)
    {
      qWarning() << Q_FUNC_INFO << "Reading of int value failed. File"
                 << filename << "element" << reader->name() << "attribute" << name << "line" << reader->lineNumber();

      return defaultValue;
    }
    return value;
  }
  else
    return defaultValue;
}

float XmlStreamReader::readAttributeFloat(const QString& name, float defaultValue)
{
  if(reader->attributes().hasAttribute(name))
  {
    bool ok;
    float value = reader->attributes().value(name).toFloat(&ok);

    if(!ok)
    {
      qWarning() << Q_FUNC_INFO << "Reading of float value failed. File"
                 << filename << "element" << reader->name() << "attribute" << name << "line" << reader->lineNumber();

      return defaultValue;
    }
    return value;
  }
  else
    return defaultValue;
}

double XmlStreamReader::readAttributeDouble(const QString& name, double defaultValue)
{
  if(reader->attributes().hasAttribute(name))
  {
    bool ok;
    double value = reader->attributes().value(name).toDouble(&ok);

    if(!ok)
    {
      qWarning() << Q_FUNC_INFO << "Reading of double value failed. File"
                 << filename << "element" << reader->name() << "attribute" << name << "line" << reader->lineNumber();

      return defaultValue;
    }
    return value;
  }
  else
    return defaultValue;
}

} // namespace util
} // namespace atools
