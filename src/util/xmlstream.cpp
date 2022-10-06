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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::encodingForName(codec->name().constData()).value_or(QStringConverter::Utf8));
#else
    stream.setCodec(codec);
#endif

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

bool XmlStream::readElementTextBool()
{
  QString str = reader->readElementText().simplified().toLower();
  if(str == "yes" || str == "true" || str == "y" || str == "t" || str == "1")
    return true;
  else if(str == "no" || str == "false" || str == "n" || str == "f" || str == "0")
    return false;

  qWarning() << Q_FUNC_INFO << "Reading of bool value failed. File"
             << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return false;
}

int XmlStream::readElementTextInt()
{
  bool ok;
  int retval = reader->readElementText().toInt(&ok);

  if(!ok)
    qWarning() << Q_FUNC_INFO << "Reading of int value failed. File"
               << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return retval;

}

float XmlStream::readElementTextFloat()
{
  bool ok;
  float retval = reader->readElementText().toFloat(&ok);

  if(!ok)
    qWarning() << Q_FUNC_INFO << "Reading of float value failed. File"
               << filename << "element" << reader->name() << "line" << reader->lineNumber();
  return retval;
}

bool XmlStream::readAttributeBool(const QString& name, bool defaultValue)
{
  if(reader->attributes().hasAttribute(name))
  {
    QString str = reader->attributes().value(name).toString();
    if(str == "yes" || str == "true" || str == "y" || str == "t" || str == "1")
      return true;
    else if(str == "no" || str == "false" || str == "n" || str == "f" || str == "0")
      return false;

    qWarning() << Q_FUNC_INFO << "Reading of bool value failed. File"
               << filename << "element" << reader->name() << "attribute" << name << "line" << reader->lineNumber();
    return defaultValue;
  }
  else
    return defaultValue;
}

int XmlStream::readAttributeInt(const QString& name, int defaultValue)
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

float XmlStream::readAttributeFloat(const QString& name, float defaultValue)
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

} // namespace util
} // namespace atools
