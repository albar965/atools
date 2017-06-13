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

namespace atools {
namespace fs {
namespace scenery {

AddOnPackage::AddOnPackage(const QString& file)
{
  filename = file;
  baseDirectory = QFileInfo(filename).path();

  QFile xmlFile(filename);

  if(xmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QXmlStreamReader xml;
    xml.setDevice(&xmlFile);

    if(xml.readNextStartElement())
    {
      if(xml.name() == "SimBase.Document" && xml.attributes().value("Type") == "AddOnXml")
      {
        while(xml.readNextStartElement())
        {
          if(xml.error() != QXmlStreamReader::NoError)
            throw Exception("Error reading \"" + filename + "\": " + xml.errorString());

          if(xml.name() == "AddOn.Name")
            name = xml.readElementText();
          else if(xml.name() == "AddOn.Description")
            description = xml.readElementText();
          else if(xml.name() == "AddOn.Component")
          {
            AddOnComponent component(xml);
            if(component.getCategory() == "Scenery")
              components.append(component);
          }
          else
            xml.skipCurrentElement();
        }
      }
    }
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
