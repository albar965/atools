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
  bgl::section::WAYPOINT,
  bgl::section::NAME_LIST
};

DataWriter::DataWriter(SqlDatabase& sqlDb, const BglReaderOptions& opts)
  : numFiles(0), numAirports(0), numNamelists(0), numVors(0), numIls(0), numNdbs(0), numMarker(0),
    numWaypoints(0), numObjectsWritten(0), resolver(opts), db(sqlDb), bglFileWriter(db,
                                                                                    *
                                                                                    this),
    sceneryAreaWriter(db,
                      *
                      this),
    airportWriter(db,
                  *
                  this),
    runwayWriter(db,
                 *
                 this),
    runwayEndWriter(db,
                    *
                    this),
    approachWriter(db,
                   *
                   this),
    approachTransWriter(db,
                        *
                        this),
    parkingWriter(db,
                  *
                  this),
    airportComWriter(db,
                     *
                     this),
    deleteAirportWriter(db,
                        *
                        this),
    waypointWriter(db,
                   *
                   this),
    tempRouteWriter(db,
                    *
                    this),
    vorWriter(db,
              *
              this),
    ndbWriter(db, *this), markerWriter(db, *this), ilsWriter(db, *this), options(opts)
{
  // APX airports
  // ATX waypoints and boundaries
  // NVX navids
  // Read all except: BRX (bridges), OBX (airport objects) and cvx (terrain)
  // resolver("brx")("obx")("cvx");
}

void DataWriter::writeSceneryArea(const SceneryArea& area)
{
  QStringList files;

  qInfo() << area;
  resolver.getFiles(area, files);

  if(!files.empty())
  {
    sceneryAreaWriter.writeOne(&area);

    BglFile bglFile(options);

    QList<SectionType> types;
    for(bgl::section::SectionType t : supportedSectionTypes)
      types.append(t);

    bglFile.setSupportedSectionTypes(types);

    for(QString filename  : files)
      if(options.doesFilenameMatch(filename))
      {
        bglFile.readFile(filename);

        bglFileWriter.writeOne(&bglFile);

        runwayIndex.clear();
        airportIndex.clear();

        airportWriter.setNameLists(bglFile.getNamelists());
        airportWriter.write(bglFile.getAirports());

        waypointWriter.write(bglFile.getWaypoints());
        vorWriter.write(bglFile.getVors());
        ndbWriter.write(bglFile.getNdbs());
        markerWriter.write(bglFile.getMarker());
        ilsWriter.write(bglFile.getIls());

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
