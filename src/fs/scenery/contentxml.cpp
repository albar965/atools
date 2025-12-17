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

using Qt::endl;

/*
 * Pre SU10:
 *
 *  <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
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

/*
 * Post SU10:
 *
 * <Priorities>
 * <Package name="fs-base" priority="-3"/>
 * <Package name="fs-base-genericairports" priority="-3"/>
 * <Package name="fs-base-ai-traffic" priority="-3"/>
 * <Package name="fs-base-nav" priority="-3"/>
 * <Package name="navigraph-navdata-base" priority="-2"/>
 * <Package name="asobo-airport-lowi-innsbruck" priority="-1"/>
 * <Package name="navigraph-navdata" priority="3"/>
 * </Priorities>
 */
void ContentXml::read(const QString& filename)
{
  areaEntries.clear();
  areaIndex.clear();
  curPriority = 5;

  if(atools::checkFile(Q_FUNC_INFO, filename))
  {
    QFile xmlFile(filename);
    if(xmlFile.open(QIODevice::ReadOnly))
    {
      atools::util::XmlStream xmlStream(&xmlFile, filename);
      QXmlStreamReader& reader = xmlStream.getReader();

      while(xmlStream.getReader().readNextStartElement())
      {
        if(reader.name() == QLatin1String("Content"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == QLatin1String("Package"))
            {
              QString name = reader.attributes().value("name").toString();
              QString activeStr = reader.attributes().value("active").toString().simplified().toLower();
              bool active = activeStr.startsWith('y') || activeStr.startsWith('t');

              int num;
              QString title;
              bool navdata = false;
              priorityTitleNavdata(name, num, title, navdata);

              SceneryArea area(num, num, title, name);
              area.setActive(active);
              area.setNavdata(navdata);

              areaIndex.insert(name.toLower(), areaEntries.size());
              areaEntries.append(area);

              // Read only attributes
              xmlStream.skipCurrentElement();
            }
            else
              xmlStream.skipCurrentElement(true /* warn */);
          } // while(xmlStream.readNextStartElement())

          break;
        }
        else if(reader.name() == QLatin1String("Priorities"))
        {
          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == QLatin1String("Package"))
            {
              QString name = reader.attributes().value("name").toString();

              bool ok;
              int priority = reader.attributes().value("priority").toInt(&ok);

              int num;
              QString title;
              bool navdata = false;
              priorityTitleNavdata(name, num, title, navdata);

              if(ok)
                num = priority;

              SceneryArea area(num, num, title, name);
              area.setActive(true);
              area.setNavdata(navdata);

              areaIndex.insert(name.toLower(), areaEntries.size());
              areaEntries.append(area);

              // Read only attributes
              xmlStream.skipCurrentElement();
              curPriority++;
            }
            else
              xmlStream.skipCurrentElement(true /* warn */);
          } // while
        }
        else
          xmlStream.skipCurrentElement(true /* warn */);

        break;
      } // while
      xmlFile.close();
    }
    else
      throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2").arg(filename).arg(xmlFile.errorString()));
  }
}

bool ContentXml::isDisabled(const QString& areaPath) const
{
  if(areaIndex.contains(areaPath.toLower()))
    return !areaEntries.at(areaIndex.value(areaPath.toLower())).isActive();

  return false;
}

int ContentXml::getPriority(const QString& areaPath, int defaultPriority) const
{
  if(areaIndex.contains(areaPath.toLower()))
    return areaEntries.at(areaIndex.value(areaPath.toLower())).getAreaNumber();

  return defaultPriority;
}

void ContentXml::priorityTitleNavdata(const QString& name, int& priority, QString& title, bool& navdata)
{
  if(name == "fs-base")
  {
    // magdec.bgl and POI
    priority = 0;
    title = tr("Base");
  }
  else if(name == "fs-base-genericairports")
  {
    // Since SU9 - Airports
    priority = 1;
    title = tr("Generic Airports");
  }
  else if(name == "fs-base-nav")
  {
    // Airport procedures
    priority = 2;
    navdata = true;
    title = tr("Base Navigation");
  }
  else
    priority = curPriority++;
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
