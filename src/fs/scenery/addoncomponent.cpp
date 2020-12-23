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

#include "fs/scenery/addoncomponent.h"

#include <QFile>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace scenery {

AddOnComponent::AddOnComponent()
{

}

AddOnComponent::AddOnComponent(QXmlStreamReader& xml)
{
  while(xml.readNextStartElement())
  {
    if(xml.name() == "Category")
      category = xml.readElementText();
    else if(xml.name() == "Path")
      path = xml.readElementText();
    else if(xml.name() == "Name")
      name = xml.readElementText();
    else if(xml.name() == "Layer")
      layer = xml.readElementText().toInt();
    else
      xml.skipCurrentElement();
  }
}

} // namespace scenery
} // namespace fs
} // namespace atools
