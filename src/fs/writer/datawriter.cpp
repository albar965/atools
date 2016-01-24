/*
 * WriterList.cpp
 *
 *  Created on: 02.05.2015
 *      Author: alex
 */

#include "datawriter.h"

#include "../bgl/bglfile.h"
#include "../scenery/fileresolver.h"
#include "../bglreaderoptions.h"
#include "sql/sqldatabase.h"

#include "nav/waypointwriter.h"
#include "nav/temproutewriter.h"
#include "nav/vorwriter.h"
#include "nav/ndbwriter.h"
#include "nav/markerwriter.h"
#include "nav/ilswriter.h"
#include "meta/bglfilewriter.h"
#include "ap/airportwriter.h"
#include "ap/rw/runwaywriter.h"
#include "ap/rw/runwayendwriter.h"
#include "runwayindex.h"
#include "airportindex.h"
#include "ap/approachwriter.h"
#include "ap/comwriter.h"
#include "ap/transitionwriter.h"
#include "ap/parkingwriter.h"
#include "ap/deleteairportwriter.h"

#include "../scenery/fileresolver.h"
#include "meta/sceneryareawriter.h"

#include <QString>
#include "../bgl/bglfile.h"
#include <QDebug>
namespace atools {
namespace fs {
namespace writer {

using bgl::BglFile;
using atools::sql::SqlDatabase;
using scenery::SceneryArea;
using atools::fs::bgl::section::SectionType;

static SectionType supportedSectionTypes[] =
{
  bgl::section::AIRPORT, bgl::section::ILS_VOR, bgl::section::NDB, bgl::section::MARKER,
  bgl::section::WAYPOINT, bgl::section::NAME_LIST
};

DataWriter::DataWriter(SqlDatabase& sqlDb, const BglReaderOptions& opts)
  : db(sqlDb), options(opts)
{
  // APX airports
  // ATX waypoints and boundaries
  // NVX navids
  // Read all except: BRX (bridges), OBX (airport objects) and cvx (terrain)
  // resolver("brx")("obx")("cvx");

  bglFileWriter = new BglFileWriter(db, *this);
  sceneryAreaWriter = new SceneryAreaWriter(db, *this);
  airportWriter = new AirportWriter(db, *this);
  runwayWriter = new RunwayWriter(db, *this);
  runwayEndWriter = new RunwayEndWriter(db, *this);
  approachWriter = new ApproachWriter(db, *this);
  approachTransWriter = new TransitionWriter(db, *this);
  parkingWriter = new ParkingWriter(db, *this);
  airportComWriter = new ComWriter(db, *this);
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
  QStringList files;
  atools::fs::scenery::FileResolver resolver(options);

  qInfo() << area;
  resolver.getFiles(area, files);

  if(!files.empty())
  {
    sceneryAreaWriter->writeOne(&area);

    BglFile bglFile(options);

    QList<SectionType> types;
    for(bgl::section::SectionType t : supportedSectionTypes)
      types.append(t);

    bglFile.setSupportedSectionTypes(types);

    for(QString filename  : files)
      if(options.doesFilenameMatch(filename))
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
