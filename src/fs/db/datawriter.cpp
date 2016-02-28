/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
#include "fs/bglreaderoptions.h"
#include "sql/sqldatabase.h"

#include "fs/db/nav/waypointwriter.h"
#include "fs/db/nav/temproutewriter.h"
#include "fs/db/nav/vorwriter.h"
#include "fs/db/nav/ndbwriter.h"
#include "fs/db/nav/markerwriter.h"
#include "fs/db/nav/ilswriter.h"
#include "fs/db/meta/bglfilewriter.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/db/ap/rw/runwaywriter.h"
#include "fs/db/ap/rw/runwayendwriter.h"
#include "fs/db/runwayindex.h"
#include "fs/db/airportindex.h"
#include "fs/db/ap/approachwriter.h"
#include "fs/db/ap/approachlegwriter.h"
#include "fs/db/ap/transitionlegwriter.h"
#include "fs/db/ap/comwriter.h"
#include "fs/db/ap/transitionwriter.h"
#include "fs/db/ap/parkingwriter.h"
#include "fs/db/ap/startwriter.h"
#include "fs/db/ap/helipadwriter.h"
#include "fs/db/ap/apronwriter.h"
#include "fs/db/ap/apronlightwriter.h"
#include "fs/db/ap/fencewriter.h"
#include "fs/db/ap/taxipathwriter.h"
#include "fs/db/boundarywriter.h"
#include "fs/db/progresshandler.h"

#include "fs/db/ap/deleteairportwriter.h"

#include "fs/scenery/fileresolver.h"
#include "fs/db/meta/sceneryareawriter.h"

#include "fs/db/boundarylinewriter.h"
#include "fs/bgl/bglfile.h"
#include "logging/loggingdefs.h"

#include <QFileInfo>

namespace atools {
namespace fs {
namespace db {

using bgl::BglFile;
using atools::sql::SqlDatabase;
using scenery::SceneryArea;
using atools::fs::bgl::section::SectionType;

extern const QSet<atools::fs::bgl::section::SectionType> SUPPORTED_SECTION_TYPES;

const QSet<atools::fs::bgl::section::SectionType> SUPPORTED_SECTION_TYPES =
{
  bgl::section::AIRPORT, bgl::section::ILS_VOR, bgl::section::NDB, bgl::section::MARKER,
  bgl::section::WAYPOINT, bgl::section::NAME_LIST, bgl::section::BOUNDARY
};

DataWriter::DataWriter(SqlDatabase& sqlDb, const BglReaderOptions& opts, ProgressHandler *progress)
  : db(sqlDb), progressHandler(progress), options(opts)
{
  bglFileWriter = new BglFileWriter(db, *this);
  sceneryAreaWriter = new SceneryAreaWriter(db, *this);
  airportWriter = new AirportWriter(db, *this);
  runwayWriter = new RunwayWriter(db, *this);
  runwayEndWriter = new RunwayEndWriter(db, *this);
  approachWriter = new ApproachWriter(db, *this);
  approachLegWriter = new ApproachLegWriter(db, *this);
  approachTransWriter = new TransitionWriter(db, *this);
  approachTransLegWriter = new TransitionLegWriter(db, *this);
  parkingWriter = new ParkingWriter(db, *this);
  airportHelipadWriter = new HelipadWriter(db, *this);
  airportStartWriter = new StartWriter(db, *this);
  airportApronWriter = new ApronWriter(db, *this);
  airportApronLightWriter = new ApronLightWriter(db, *this);
  airportFenceWriter = new FenceWriter(db, *this);
  airportComWriter = new ComWriter(db, *this);
  airportTaxiPathWriter = new TaxiPathWriter(db, *this);
  deleteAirportWriter = new DeleteAirportWriter(db, *this);
  waypointWriter = new WaypointWriter(db, *this);
  tempRouteWriter = new TempRouteWriter(db, *this);
  vorWriter = new VorWriter(db, *this);
  ndbWriter = new NdbWriter(db, *this);
  markerWriter = new MarkerWriter(db, *this);
  ilsWriter = new IlsWriter(db, *this);

  boundaryWriter = new BoundaryWriter(db, *this);
  boundaryLineWriter = new BoundaryLineWriter(db, *this);

  runwayIndex = new RunwayIndex();
  airportIndex = new AirportIndex();
}

DataWriter::~DataWriter()
{
  delete bglFileWriter;
  delete sceneryAreaWriter;
  delete airportWriter;
  delete runwayWriter;
  delete runwayEndWriter;
  delete approachWriter;
  delete approachLegWriter;
  delete approachTransWriter;
  delete approachTransLegWriter;
  delete parkingWriter;
  delete airportHelipadWriter;
  delete airportStartWriter;
  delete airportApronWriter;
  delete airportApronLightWriter;
  delete airportFenceWriter;
  delete airportTaxiPathWriter;
  delete airportComWriter;
  delete deleteAirportWriter;
  delete waypointWriter;
  delete tempRouteWriter;
  delete vorWriter;
  delete ndbWriter;
  delete markerWriter;
  delete ilsWriter;
  delete boundaryWriter;
  delete boundaryLineWriter;
  delete runwayIndex;
  delete airportIndex;
}

void DataWriter::writeSceneryArea(const SceneryArea& area)
{
  QStringList filepaths, filenames;
  atools::fs::scenery::FileResolver resolver(options);
  resolver.getFiles(area, &filepaths, &filenames);

  if(!filepaths.empty())
  {
    sceneryAreaWriter->writeOne(&area);

    BglFile bglFile(&options);

    bglFile.setSupportedSectionTypes(SUPPORTED_SECTION_TYPES);

    for(int i = 0; i < filepaths.size(); i++)
    {
      progressHandler->setNumFiles(numFiles);
      progressHandler->setNumAirports(numAirports);
      progressHandler->setNumNamelists(numNamelists);
      progressHandler->setNumVors(numVors);
      progressHandler->setNumIls(numIls);
      progressHandler->setNumNdbs(numNdbs);
      progressHandler->setNumMarker(numMarker);
      progressHandler->setNumBoundaries(numBoundaries);
      progressHandler->setNumWaypoints(numWaypoints);
      progressHandler->setNumObjectsWritten(numObjectsWritten);
      if((aborted = progressHandler->reportProgress(filenames.at(i))) == true)
        return;

      QString filepath = filepaths.at(i);
      bglFile.readFile(filepath);
      if(bglFile.hasContent())
      {
        // Execution order is important due to dependencies between the writers
        bglFileWriter->writeOne(&bglFile);

        runwayIndex->clear();
        airportIndex->clear();

        airportWriter->setNameLists(bglFile.getNamelists());
        airportWriter->write(bglFile.getAirports());

        waypointWriter->write(bglFile.getWaypoints());
        vorWriter->write(bglFile.getVors());
        ndbWriter->write(bglFile.getNdbs());
        markerWriter->write(bglFile.getMarker());
        ilsWriter->write(bglFile.getIls());

        boundaryWriter->write(bglFile.getBoundaries());

        numAirports += bglFile.getAirports().size();
        numNamelists += bglFile.getNamelists().size();
        numVors += bglFile.getVors().size();
        numIls += bglFile.getIls().size();
        numNdbs += bglFile.getNdbs().size();
        numMarker += bglFile.getMarker().size();
        numWaypoints += bglFile.getWaypoints().size();
        numBoundaries += bglFile.getBoundaries().size();
        numFiles++;
      }
    }
    db.commit();
  }
}

void DataWriter::logResults()
{
  qInfo().nospace() << "Done. Read "
                    << numFiles << " files, "
                    << numAirports << " airports, "
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
