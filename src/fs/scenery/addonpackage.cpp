/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/scenery/addonpackage.h"

#include "exception.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include <QFileInfo>
#include <QTextCodec>

namespace atools {
namespace fs {
namespace scenery {

AddOnPackage::AddOnPackage(const QString& file)
{
  filename = file;
  baseDirectory = QFileInfo(filename).path();

  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly))
  {
    QTextCodec *codec = nullptr;

    // Load a part of the file and detect the BOM/codec
    const qint64 PROBE_SIZE = 128;
    char *buffer = new char[PROBE_SIZE];
    qint64 bytesRead = xmlFile.read(buffer, PROBE_SIZE);
    if(bytesRead > 0)
      codec = QTextCodec::codecForUtfText(QByteArray(buffer, static_cast<int>(bytesRead)), nullptr);
    delete[] buffer;

    xmlFile.seek(0);

    QScopedPointer<QXmlStreamReader> xml;

    if(codec != nullptr)
    {
      // Load the file into a text file to avoid BOM / xml encoding mismatches
      qDebug() << "Encoding" << codec->name();
      QTextStream stream(&xmlFile);
      stream.setCodec(codec);
      QString str = stream.readAll();

      // The reader ignores the XML encoding header when reading from a string
      xml.reset(new QXmlStreamReader(str));
    }
    else
    {
      // Let the stream reader detect the encoding in the PI
      qDebug() << "No UTF Encoding found";
      xml.reset(new QXmlStreamReader(&xmlFile));
    }

    if(xml->readNextStartElement())
    {
      if(xml->name() == "SimBase.Document")
      {
        while(xml->readNextStartElement())
        {
          if(xml->error() != QXmlStreamReader::NoError)
            throw Exception("Error reading \"" + filename + "\": " + xml->errorString());

          if(xml->name() == "AddOn.Name")
            name = xml->readElementText();
          else if(xml->name() == "AddOn.Description")
            description = xml->readElementText();
          else if(xml->name() == "AddOn.Component")
          {
            AddOnComponent component(*xml);
            if(component.getCategory() == "Scenery")
              components.append(component);
          }
          else
            xml->skipCurrentElement();
        }
      }
    }
    if(xml->hasError())
      throw Exception(tr("Cannot read file %1. Reason: %2").arg(file).arg(xml->errorString()));
  }
  else
    throw Exception(tr("Cannot open file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
}

AddOnPackage::~AddOnPackage()
{

}

} // namespace scenery
} // namespace fs
} // namespace atools
