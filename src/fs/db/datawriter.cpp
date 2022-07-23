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

#include "fs/db/datawriter.h"

#include "fs/bgl/bglfile.h"
#include "fs/scenery/fileresolver.h"
#include "fs/scenery/languagejson.h"
#include "fs/scenery/materiallib.h"
#include "fs/navdatabaseoptions.h"
#include "sql/sqldatabase.h"
#include "fs/db/nav/waypointwriter.h"
#include "fs/db/nav/airwaysegmentwriter.h"
#include "fs/db/nav/vorwriter.h"
#include "fs/db/nav/tacanwriter.h"
#include "fs/db/nav/ndbwriter.h"
#include "fs/db/nav/markerwriter.h"
#include "fs/db/nav/ilswriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/airportfilewriter.h"
#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db/ap/rw/runwayendwriter.h"
#include "fs/db/runwayindex.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/ap/transitionlegwriter.h"
#include "fs/db/ap/comwriter.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/db/ap/sidstarwriter.h"
#include "fs/db/ap/sidstarapproachlegwriter.h"
#include "fs/db/ap/sidstartransitionwriter.h"
#include "fs/db/ap/sidstartransitionlegwriter.h"
#include "fs/db/ap/parkingwriter.h"
#include "fs/db/ap/startwriter.h"
#include "fs/db/ap/helipadwriter.h"
#include "fs/db/ap/apronwriter.h"
#include "fs/db/ap/taxipathwriter.h"
#include "fs/db/nav/boundarywriter.h"
#include "fs/progresshandler.h"
#include "fs/scenery/fileresolver.h"
#include "fs/db/meta/sceneryareawriter.h"
#include "atools.h"
#include "fs/common/magdecreader.h"
#include "settings/settings.h"
#include "exception.h"

#include <QDebug>
#include <QFileInfo>

namespace atools {
namespace fs {
namespace db {

using bgl::BglFile;
using atools::fs::common::MagDecReader;
using atools::sql::SqlDatabase;
using scenery::SceneryArea;
using atools::fs::bgl::section::SectionType;

static const QSet<atools::fs::bgl::section::SectionType> SUPPORTED_SECTION_TYPES =
{
  bgl::section::AIRPORT, bgl::section::AIRPORT_ALT, bgl::section::ILS_VOR, bgl::section::NDB,
  bgl::section::MARKER, bgl::section::WAYPOINT, bgl::section::NAME_LIST, bgl::section::BOUNDARY,
  bgl::section::P3D_TACAN // , bgl::section::MSFS_DELETE_AIRPORT_NAV, bgl::section::MSFS_DELETE_NAV
};

DataWriter::DataWriter(SqlDatabase& sqlDb, const NavDatabaseOptions& opts, atools::fs::ProgressHandler *progress)
  : db(sqlDb), progressHandler(progress), options(opts)
{
  bglFileWriter = new BglFileWriter(db, *this);
  sceneryAreaWriter = new SceneryAreaWriter(db, *this);
  airportWriter = new AirportWriter(db, *this);
  airportFileWriter = new AirportFileWriter(db, *this);
  runwayWriter = new RunwayWriter(db, *this);
  runwayEndWriter = new RunwayEndWriter(db, *this);
  approachWriter = new ApproachWriter(db, *this);
  approachLegWriter = new ApproachLegWriter(db, *this);
  approachTransWriter = new TransitionWriter(db, *this);
  approachTransLegWriter = new TransitionLegWriter(db, *this);
  sidStarWriter = new SidStarWriter(db, *this);
  sidStarApproachLegWriter = new SidStarApproachLegWriter(db, *this);
  sidStarTransWriter = new SidStarTransitionWriter(db, *this);
  sidStarTransLegWriter = new SidStarTransitionLegWriter(db, *this);
  parkingWriter = new ParkingWriter(db, *this);
  airportHelipadWriter = new HelipadWriter(db, *this);
  airportStartWriter = new StartWriter(db, *this);
  airportApronWriter = new ApronWriter(db, *this);
  airportComWriter = new ComWriter(db, *this);
  airportTaxiPathWriter = new TaxiPathWriter(db, *this);
  waypointWriter = new WaypointWriter(db, *this);
  airwaySegmentWriter = new AirwaySegmentWriter(db, *this);
  vorWriter = new VorWriter(db, *this);
  tacanWriter = new TacanWriter(db, *this);
  ndbWriter = new NdbWriter(db, *this);
  markerWriter = new MarkerWriter(db, *this);
  ilsWriter = new IlsWriter(db, *this);

  boundaryWriter = new BoundaryWriter(db, *this);

  runwayIndex = new RunwayIndex();
  magDecReader = new MagDecReader();
}

DataWriter::~DataWriter()
{
  close();
}

void DataWriter::close()
{
  delete bglFileWriter;
  bglFileWriter = nullptr;
  delete sceneryAreaWriter;
  sceneryAreaWriter = nullptr;
  delete airportWriter;
  airportWriter = nullptr;
  delete airportFileWriter;
  airportFileWriter = nullptr;
  delete runwayWriter;
  runwayWriter = nullptr;
  delete runwayEndWriter;
  runwayEndWriter = nullptr;
  delete approachWriter;
  approachWriter = nullptr;
  delete approachLegWriter;
  approachLegWriter = nullptr;
  delete approachTransWriter;
  approachTransWriter = nullptr;
  delete approachTransLegWriter;
  approachTransLegWriter = nullptr;
  delete sidStarWriter;
  sidStarWriter = nullptr;
  delete sidStarApproachLegWriter;
  sidStarApproachLegWriter = nullptr;
  delete sidStarTransWriter;
  sidStarTransWriter = nullptr;
  delete sidStarTransLegWriter;
  sidStarTransLegWriter = nullptr;
  delete parkingWriter;
  parkingWriter = nullptr;
  delete airportHelipadWriter;
  airportHelipadWriter = nullptr;
  delete airportStartWriter;
  airportStartWriter = nullptr;
  delete airportApronWriter;
  airportApronWriter = nullptr;
  delete airportTaxiPathWriter;
  airportTaxiPathWriter = nullptr;
  delete airportComWriter;
  airportComWriter = nullptr;
  delete waypointWriter;
  waypointWriter = nullptr;
  delete airwaySegmentWriter;
  airwaySegmentWriter = nullptr;
  delete vorWriter;
  vorWriter = nullptr;
  delete tacanWriter;
  tacanWriter = nullptr;
  delete ndbWriter;
  ndbWriter = nullptr;
  delete markerWriter;
  markerWriter = nullptr;
  delete ilsWriter;
  ilsWriter = nullptr;
  delete boundaryWriter;
  boundaryWriter = nullptr;
  delete runwayIndex;
  runwayIndex = nullptr;
  delete magDecReader;
  magDecReader = nullptr;
}

float DataWriter::getMagVar(const geo::Pos& pos, float defaultValue) const
{
  if(magDecReader->isValid())
    return magDecReader->getMagVar(pos);
  else
    return defaultValue;
}

QString DataWriter::getSurface(const QUuid& key)
{
  if(materialLibScenery != nullptr)
  {
    QString surface = materialLibScenery->getSurfaceForUuid(key);
    if(!surface.isEmpty())
      return surface;
  }

  if(materialLib != nullptr)
    return materialLib->getSurfaceForUuid(key);
  else
    return QString();
}

QString DataWriter::getLanguage(const QString& key)
{
  if(languageIndex != nullptr)
  {
    if(key.startsWith("TT:"))
      return languageIndex->getName(key);

    return key;
  }
  else
    return key;
}

void DataWriter::writeSceneryArea(const SceneryArea& area)
{
  QStringList filepaths, filenames;

  // Get all BGL files in this scenery area
  atools::fs::scenery::FileResolver resolver(options);
  resolver.getFiles(area, &filepaths, &filenames);

  if(sceneryErrors != nullptr)
    sceneryErrors->sceneryErrorsMessages.append(resolver.getErrorMessages());
  progressHandler->reportErrors(resolver.getErrorMessages().size());

  if(!filepaths.empty())
  {
    // Write the scenery area metadata
    sceneryAreaWriter->writeOne(area);

    BglFile bglFile(&options);

    bglFile.setSupportedSectionTypes(SUPPORTED_SECTION_TYPES);

    for(int i = 0; i < filepaths.size(); i++)
    {
      progressHandler->setNumFiles(numFiles);
      progressHandler->setNumAirports(airportIdents.size());
      progressHandler->setNumNamelists(numNamelists);
      progressHandler->setNumVors(numVors);
      progressHandler->setNumIls(numIls);
      progressHandler->setNumNdbs(numNdbs);
      progressHandler->setNumMarker(numMarker);
      progressHandler->setNumBoundaries(numBoundaries);
      progressHandler->setNumWaypoints(numWaypoints);
      progressHandler->setNumObjectsWritten(numObjectsWritten);

      if((aborted = progressHandler->reportBglFile(filepaths.at(i))) == true)
        return;

      QString currentBglFilePath = filepaths.at(i);

      try
      {
        // ================================================================================
        // Read all records into a internal object tree (atools::fs::bgl namespace)
        bglFile.readFile(currentBglFilePath, area);

        if(bglFile.hasContent() && bglFile.isValid())
        {
          // ================================================================================
          // Write to the database

          // if(!bglFile.getHeader().hasValidMagicNumber())
          // qWarning() << "Content in file with invalid magic number";

          // Write BGL file metadata
          bglFileWriter->writeOne(bglFile);

          // Clear the indexes
          runwayIndex->clear();

          // Execution order is important due to dependencies between the writers
          // (i.e. ILS writer looks for runway end ids)
          // Writer also need to access the ids of their parent record objects
          // (i.e. runway needs the current airport ID

          airportWriter->setNameLists(bglFile.getNamelists());

          // Write airport and all subrecords like runways, approaches, parking and so on
          airportWriter->write(bglFile.getAirports());

          airportFileWriter->write(bglFile.getAirports());

          if(!area.isNavigraphNavdataUpdate())
          {
            // Write all navaids to the database
            waypointWriter->write(bglFile.getWaypoints());
            vorWriter->write(bglFile.getVors());
            tacanWriter->write(bglFile.getTacans());
            ndbWriter->write(bglFile.getNdbs());
            markerWriter->write(bglFile.getMarker());
          }
          ilsWriter->write(bglFile.getIls());

          if(!area.isNavigraphNavdataUpdate())
            boundaryWriter->write(bglFile.getBoundaries());

          for(const atools::fs::bgl::Airport *ap : bglFile.getAirports())
            airportIdents.insert(ap->getIdent());

          numNamelists += bglFile.getNamelists().size();

          if(!area.isNavigraphNavdataUpdate())
          {
            numVors += bglFile.getVors().size() + bglFile.getTacans().size();
            numNdbs += bglFile.getNdbs().size();
            numMarker += bglFile.getMarker().size();
            numWaypoints += bglFile.getWaypoints().size();
            numBoundaries += bglFile.getBoundaries().size();
          }
          numIls += bglFile.getIls().size();
          numFiles++;
        }

        // Print a one line short report on airports that were found in the BGL
        if(!bglFile.getAirports().isEmpty())
        {
          QStringList apIcaos;
          for(const atools::fs::bgl::Airport *ap : bglFile.getAirports())
          {
            // Truncate at 10
            if(apIcaos.size() < 10)
              apIcaos.append(ap->getIdent());
            else
              break;
          }
          if(bglFile.getAirports().size() > 10)
            apIcaos.append("...");
          qDebug() << "Found" << bglFile.getAirports().size() << "airports. idents:" << apIcaos.join(",");
        }
      }
      catch(atools::Exception& e)
      {
        qCritical() << "Caught exception reading" << currentBglFilePath << ":" << e.what();
        progressHandler->reportError();
        if(sceneryErrors != nullptr)
          sceneryErrors->fileErrors.append({currentBglFilePath, QString(e.what()), 0});
      }
      catch(...)
      {
        qCritical() << "Caught unknown exception reading" << currentBglFilePath;
        progressHandler->reportError();
        if(sceneryErrors != nullptr)
          sceneryErrors->fileErrors.append({currentBglFilePath, QString(), 0});
      }
    }
    db.commit();
  }
}

void DataWriter::readMagDeclBgl(const QString& fileScenery)
{
  QString fileSettings = atools::buildPath({atools::settings::Settings::getPath(), "magdec.bgl"});
  QString fileApp = atools::buildPath({QCoreApplication::applicationDirPath(), "magdec", "magdec.bgl"});

  QString file;
  if(QFileInfo::exists(fileScenery) && QFileInfo(fileScenery).isFile())
    // Check if there is a file in the simulator scenery directory
    file = fileScenery;
  else if(QFileInfo::exists(fileSettings) && QFileInfo(fileSettings).isFile())
    // Check if there is a file in the settings directory
    file = fileSettings;
  else if(QFileInfo::exists(fileApp) && QFileInfo(fileApp).isFile())
    // Check if there is a file in the application directory
    file = fileApp;

  qInfo() << "Reading" << file;

  bool loaded = false;

  if(!file.isEmpty())
  {
    try
    {
      magDecReader->readFromBgl(file);
    }
    catch(atools::Exception& e)
    {
      qCritical() << "Caught exception reading" << file << ":" << e.what();
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->fileErrors.append({file, QString(e.what()), 0});
    }
    catch(...)
    {
      qCritical() << "Caught unknown exception reading" << file;
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->fileErrors.append({file, tr("Cannot read file. Falling back to world magnetic model."), 0});
    }

    if(magDecReader->isValid())
    {
      magDecReader->writeToTable(db);
      db.commit();
      loaded = true;
    }
    else
    {
      progressHandler->reportError();
      if(sceneryErrors != nullptr)
        sceneryErrors->fileErrors.append({file, tr("File not valid. Falling back to world magnetic model."), 0});
    }
  }
  else
  {
    progressHandler->reportError();
    if(sceneryErrors != nullptr)
      sceneryErrors->fileErrors.append({"magdec.bgl", tr("File not found. Falling back to world magnetic model."), 0});
  }

  if(!loaded)
  {
    magDecReader->readFromWmm();
    magDecReader->writeToTable(db);
    db.commit();
  }
}

void DataWriter::logResults()
{
  qInfo().nospace() << "Done. Read "
                    << numFiles << " files, "
                    << airportIdents.size() << " airports, "
                    << numNamelists << " namelists, "
                    << numVors << " VORs, "
                    << numIls << " ILS, "
                    << numNdbs << " NDBs, "
                    << numMarker << " markers and "
                    << numBoundaries << " boundaries and "
                    << numWaypoints << " waypoints.";
  qInfo().nospace() << "Wrote " << numObjectsWritten << " objects.";
}

} // namespace writer
} // namespace fs
} // namespace atools
