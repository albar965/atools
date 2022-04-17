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

#include "fs/scenery/contentxml.h"

#include "util/xmlstream.h"
#include "exception.h"
#include "atools.h"
#include "fs/scenery/sceneryarea.h"

#include <QFile>
#include <QDir>
#include <QDebug>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace scenery {

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
using Qt::endl;
#endif

/*
 *  <Content>
 *  <Package name="fs-base" active="true"/>
 *  <Package name="asobo-airport-kord-chicago-ohare" active="true"/>
 *  ...
 *  <Package name="asobo-airport-vqpr-paro" active="true"/>
 *  <Package name="asobo-liveevent" active="true"/>
 *  <Package name="asobo-airport-nzqn-queenstown" active="true"/>
 *  <Package name="asobo-airport-sbgl-riodejaneiro" active="true"/>
 *  <Package name="fs-base-nav" active="true"/>
 *  <Package name="asobo-modellib-props" active="true"/>
 *  <Package name="asobo-modellib-airport-generic" active="true"/>
 *  <Package name="asobo-modellib-buildings" active="true"/>
 *  <Package name="asobo-cameras" active="true"/>
 *  <Package name="asobo-jetways" active="true"/>
 *  <Package name="fs-base-ai-traffic" active="true"/>
 *  </Content>
 */
void ContentXml::read(const QString& filename)
{
  areaEntries.clear();
  disabledAreas.clear();
  number = 5;

  if(atools::checkFile(filename))
  {
    QFile xmlFile(filename);
    if(xmlFile.open(QIODevice::ReadOnly))
    {
      atools::util::XmlStream xmlStream(&xmlFile, filename);
      QXmlStreamReader& reader = xmlStream.getReader();

      xmlStream.readUntilElement("Content");

      while(xmlStream.readNextStartElement())
      {
        if(reader.name() == "Package")
        {
          QString name = reader.attributes().value("name").toString();
          QString activeStr = reader.attributes().value("active").toString().simplified().toLower();
          bool active = activeStr.startsWith('y') || activeStr.startsWith('t');

          int num;
          QString title;
          bool navdata = false;
          if(name == "fs-base")
          {
            num = 0;
            title = tr("Base Airports");
          }
          else if(name == "fs-base-genericairports")
          {
            num = 1;
            title = tr("Generic Airports");
          }
          else if(name == "fs-base-nav")
          {
            num = 2;
            navdata = true;
            title = tr("Base Navigation");
          }
          else
            num = number++;

          SceneryArea area(num, num, title, name);
          area.setActive(active);
          area.setNavdata(navdata);
          areaEntries.append(area);
          number++;

          if(!active)
            disabledAreas.insert(name.toLower());

          // Read only attributes
          xmlStream.skipCurrentElement();
        }
        else
          xmlStream.skipCurrentElement(true /* warn */);
      }
      xmlFile.close();
    }
    else
      throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
  }
}

QDebug operator<<(QDebug out, const ContentXml& cfg)
{
  QDebugStateSaver saver(out);

  out.nospace() << "ContentXml[" << endl;

  for(const SceneryArea& area : cfg.areaEntries)
    out.nospace().noquote() << area << endl;

  out.nospace().noquote() << endl << "]";
  return out;
}

} // namespace scenery
} // namespace fs
} // namespace atools
