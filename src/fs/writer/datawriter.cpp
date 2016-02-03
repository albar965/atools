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

#include "fs/writer/datawriter.h"

#include "fs/bgl/bglfile.h"
#include "fs/scenery/fileresolver.h"
#include "fs/bglreaderoptions.h"
#include "sql/sqldatabase.h"

#include "fs/writer/nav/waypointwriter.h"
#include "fs/writer/nav/temproutewriter.h"
#include "fs/writer/nav/vorwriter.h"
#include "fs/writer/nav/ndbwriter.h"
#include "fs/writer/nav/markerwriter.h"
#include "fs/writer/nav/ilswriter.h"
#include "fs/writer/meta/bglfilewriter.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/writer/ap/rw/runwaywriter.h"
#include "fs/writer/ap/rw/runwayendwriter.h"
#include "fs/writer/runwayindex.h"
#include "fs/writer/airportindex.h"
#include "fs/writer/ap/approachwriter.h"
#include "fs/writer/ap/comwriter.h"
#include "fs/writer/ap/transitionwriter.h"
#include "fs/writer/ap/parkingwriter.h"
#include "fs/writer/ap/startwriter.h"
#include "fs/writer/ap/helipadwriter.h"
#include "fs/writer/ap/apronwriter.h"
#include "fs/writer/ap/apronlightwriter.h"
#include "fs/writer/ap/fencewriter.h"
#include "fs/writer/ap/taxipathwriter.h"

#include "fs/writer/ap/deleteairportwriter.h"

#include "fs/scenery/fileresolver.h"
#include "fs/writer/meta/sceneryareawriter.h"

#include "fs/bgl/bglfile.h"
#include "logging/loggingdefs.h"

namespace atools {
namespace fs {
namespace writer {

using bgl::BglFile;
using atools::sql::SqlDatabase;
using scenery::SceneryArea;
using atools::fs::bgl::section::SectionType;

extern QList<atools::fs::bgl::section::SectionType> supportedSectionTypes;

QList<atools::fs::bgl::section::SectionType> supportedSectionTypes =
{
  bgl::section::AIRPORT, bgl::section::ILS_VOR, bgl::section::NDB, bgl::section::MARKER,
  bgl::section::WAYPOINT, bgl::section::NAME_LIST
};

DataWriter::DataWriter(SqlDatabase& sqlDb, const BglReaderOptions& opts)
  : db(sqlDb), options(opts)
{
  bglFileWriter = new BglFileWriter(db, *this);
  sceneryAreaWriter = new SceneryAreaWriter(db, *this);
  airportWriter = new AirportWriter(db, *this);
  runwayWriter = new RunwayWriter(db, *this);
  runwayEndWriter = new RunwayEndWriter(db, *this);
  approachWriter = new ApproachWriter(db, *this);
  approachTransWriter = new TransitionWriter(db, *this);
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
  delete approachTransWriter;
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
  delete runwayIndex;
  delete airportIndex;
}

void DataWriter::writeSceneryArea(const SceneryArea& area)
{
  if(!options.includePath(area.getLocalPath()))
    return;

  qInfo() << area;

  QStringList files;
  atools::fs::scenery::FileResolver resolver(options);

  // APX airports
  // ATX waypoints and boundaries
  // NVX navids
  // Read all except: BRX (bridges), OBX (airport objects) and cvx (terrain)
  // resolver("brx")("obx")("cvx");
  resolver.getFiles(area, files);

  if(!files.empty())
  {
    sceneryAreaWriter->writeOne(&area);

    BglFile bglFile(&options);

    bglFile.setSupportedSectionTypes(supportedSectionTypes);

    for(QString filename  : files)
      if(options.includeFilename(filename))
      {
        bglFile.readFile(filename);

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

        numAirports += bglFile.getAirports().size();
        numNamelists += bglFile.getNamelists().size();
        numVors += bglFile.getVors().size();
        numIls += bglFile.getIls().size();
        numNdbs += bglFile.getNdbs().size();
        numMarker += bglFile.getMarker().size();
        numWaypoints += bglFile.getWaypoints().size();
        numFiles++;
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
                    << numWaypoints << " waypoints.";
  qInfo().nospace() << "Wrote " << numObjectsWritten << " objects.";
}

} // namespace writer
} // namespace fs
} // namespace atools
