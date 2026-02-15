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

#include "fs/scenery/materiallib.h"

#include "atools.h"
#include "exception.h"
#include "fs/navdatabaseerrors.h"
#include "fs/navdatabaseoptions.h"
#include "fs/progresshandler.h"
#include "fs/scenery/layoutjson.h"
#include "util/xmlstream.h"

#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include <QXmlStreamReader>

namespace atools {
namespace fs {
namespace scenery {

MaterialLib::MaterialLib(const NavDatabaseOptions& opts, ProgressHandler *progress, SceneryErrors *errors)
  : options(opts), progressHandler(progress), sceneryErrors(errors)
{

}

void MaterialLib::readCommunity(const QString& basePath)
{
  LayoutJson layout;
  layout.read(basePath + atools::SEP + "layout.json");
  for(const QString& str : layout.getMaterialPaths())
  {
    QString filepath = basePath + atools::SEP + str;

    try
    {
      read(filepath);
    }
    catch(atools::Exception& e)
    {
      qCritical() << "Caught exception reading" << filepath << ":" << e.what();
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString(e.what())));
    }
    catch(...)
    {
      qCritical() << "Caught unknown exception reading" << filepath;
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString()));
    }
  }
}

void MaterialLib::readOfficial(const QString& basePath)
{
  LayoutJson layout;
  layout.read(basePath + atools::SEP + "asobo-material-lib" + atools::SEP + "layout.json");

  for(const QString& str : layout.getMaterialPaths())
  {
    QString filepath = basePath + atools::SEP + "asobo-material-lib" + atools::SEP + str;

    try
    {
      read(filepath);
    }
    catch(atools::Exception& e)
    {
      qCritical() << "Caught exception reading" << filepath << ":" << e.what();
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString(e.what())));
    }
    catch(...)
    {
      qCritical() << "Caught unknown exception reading" << filepath;
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString()));
    }
  }

  layout.clear();
  layout.read(basePath + atools::SEP + "fs-base-material-lib" + atools::SEP + "layout.json");

  for(const QString& str : layout.getMaterialPaths())
  {
    QString filepath = basePath + atools::SEP + "fs-base-material-lib" + atools::SEP + str;
    try
    {
      read(filepath);
    }
    catch(atools::Exception& e)
    {
      qCritical() << "Caught exception reading" << filepath << ":" << e.what();
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString(e.what())));
    }
    catch(...)
    {
      qCritical() << "Caught unknown exception reading" << filepath;
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->appendFileError(SceneryFileError(filepath, QString()));
    }
  }
}

/*
 *  <Library Version="1.1.0">
 *  <Material Version="1.4.0" Name="GrassGround06" Guid="{B8D6CC1D-DEA9-4A20-8BB6-123149776A7C}" SurfaceType="ASPHALT"
 * Type="CODE_DIFFUSE" Metal="0.000000" Rough="0.000000" Opacity="1.000000" BlendMode="Transparent">
 *   <TagList>
 *     <Tag>Asobo_Ground</Tag>
 *   </TagList>
 *  ...
 *  </Material>
 *  <Material Version="1.4.0" Name="Runway_Test" Guid="{C2F72DA3-851D-466D-A492-E261E5435395}" SurfaceType="UNDEFINED"
 * Type="CODE_DIFFUSE" Metal="0.000000" Rough="0.000000" Opacity="1.000000" BlendMode="Transparent">
 *   <TagList>
 *     <Tag>Test</Tag>
 */
void MaterialLib::read(const QString& filename)
{
  if(options.isIncludedGui(QFileInfo(filename)))
  {
    if(atools::checkFile(Q_FUNC_INFO, filename))
    {
      QStringList probe = atools::probeFile(filename, 10);

      // Test if this is not another file type like aircraft checklist definitions
      if(!probe.filter("<Library", Qt::CaseInsensitive).isEmpty() &&
         !probe.filter("<Material", Qt::CaseInsensitive).isEmpty())
      {
        QFile xmlFile(filename);
        if(xmlFile.open(QIODevice::ReadOnly))
        {
          atools::util::XmlStream xmlStream(&xmlFile, filename);
          QXmlStreamReader& reader = xmlStream.getReader();

          xmlStream.readUntilElement("Library");

          while(xmlStream.readNextStartElement())
          {
            if(reader.name() == QLatin1String("Material"))
            {
              QString surface = reader.attributes().value("SurfaceType").toString();
              if(surface != "UNDEFINED")
                surfaceMap.insert(QUuid(reader.attributes().value("Guid").toString()), surface);

              // Read only attributes
              xmlStream.skipCurrentElement();
            }
            else
              xmlStream.skipCurrentElement(true /* warn */);
          }
          xmlFile.close();
        }
        else
          qWarning() << Q_FUNC_INFO << "Cannot open file" << filename << xmlFile.errorString();
      }
      else
        qWarning() << Q_FUNC_INFO << "Cannot open file" << filename << "Not a material library file";
    }
  }
}

} // namespace scenery
} // namespace fs
} // namespace atools
