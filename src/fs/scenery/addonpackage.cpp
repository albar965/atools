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

#include "fs/scenery/addonpackage.h"

#include "exception.h"
#include "atools.h"
#include "util/xmlstream.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QFileInfo>

namespace atools {
namespace fs {
namespace scenery {

AddOnPackage::AddOnPackage(const QString& file)
{
  filename = file;
  if(atools::checkFile(Q_FUNC_INFO, file))
  {
    baseDirectory = QFileInfo(filename).path();

    QFile xmlFile(filename);
    if(xmlFile.open(QIODevice::ReadOnly))
    {
      atools::util::XmlStream xmlStream(&xmlFile, file);
      QXmlStreamReader& xmlReader = xmlStream.getReader();

      if(xmlReader.readNextStartElement())
      {
        if(xmlReader.name() == QStringLiteral("SimBase.Document"))
        {
          while(xmlReader.readNextStartElement())
          {
            if(xmlReader.error() != QXmlStreamReader::NoError)
              throw Exception("Error reading \"" + filename + "\": " + xmlReader.errorString());

            if(xmlReader.name() == QStringLiteral("AddOn.Name"))
              name = xmlReader.readElementText();
            else if(xmlReader.name() == QStringLiteral("AddOn.Description"))
              description = xmlReader.readElementText();
            else if(xmlReader.name() == QStringLiteral("AddOn.Component"))
            {
              AddOnComponent component(xmlReader);
              if(component.getCategory() == "Scenery")
                components.append(component);
            }
            else
              xmlReader.skipCurrentElement();
          }
        }
      }
      if(xmlReader.hasError())
        throw Exception(tr("Cannot read file %1. Reason: %2").arg(file).arg(xmlReader.errorString()));
    }
    else
      throw Exception(tr("Cannot open file %1. Reason: %2").arg(file).arg(xmlFile.errorString()));
  }
}

AddOnPackage::~AddOnPackage()
{

}

} // namespace scenery
} // namespace fs
} // namespace atools
