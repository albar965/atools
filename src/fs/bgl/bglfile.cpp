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

#include "fs/bgl/bglfile.h"

#include "io/binarystream.h"
#include "fs/bgl/section.h"
#include "fs/bgl/subsection.h"
#include "fs/bgl/header.h"
#include "fs/bgl/ap/airport.h"
#include "fs/bgl/nl/namelist.h"
#include "fs/bgl/nav/ilsvor.h"
#include "fs/bgl/nav/vor.h"
#include "fs/bgl/nav/ils.h"
#include "fs/bgl/nav/marker.h"
#include "fs/bgl/nav/ndb.h"
#include "fs/bgl/nav/waypoint.h"

#include <QList>
#include "logging/loggingdefs.h"

#include <QFile>

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

BglFile::BglFile(const BglReaderOptions *opts)
  : size(0), options(opts)
{
}

BglFile::~BglFile()
{
  freeObjects();
}

void BglFile::readFile(QString file)
{
  freeObjects();
  filename = file;

  if(options->isVerbose())
    qDebug() << "==============================";

  qDebug() << "Reading file " << filename;

  QFile ifs(filename);
  if(ifs.open(QIODevice::ReadOnly))
  {
    BinaryStream bs(&ifs);

    this->filename = file;
    this->size = bs.getFileSize();

    readHeaderAndSections(&bs);
    readRecords(&bs);
    ifs.close();
  }
}

bool BglFile::hasContent()
{
  return !(airports.isEmpty() &&
           vors.isEmpty() &&
           ils.isEmpty() &&
           ndbs.isEmpty() &&
           marker.isEmpty() &&
           waypoints.isEmpty());
}

void BglFile::readHeaderAndSections(BinaryStream *bs)
{
  header = Header(options, bs);
  if(options->isVerbose())
    qDebug() << header;

  // Section pointer
  for(unsigned int i = 0; i < header.getNumSections(); i++)
  {
    Section s = Section(options, bs);
    if(std::binary_search(supportedSectionTypes.begin(), supportedSectionTypes.end(), s.getType()))
    {
      if(options->isVerbose())
        qDebug() << s;
      sections.push_back(s);
    }
  }

  for(Section& it : sections)
  {
    bs->seekg(it.getFirstSubsectionOffset());
    for(int i = 0; i < it.getNumSubsections(); i++)
    {
      Subsection s(options, bs, it);
      if(options->isVerbose())
        qDebug() << s;
      subsections.push_back(s);
    }
  }
}

const Record *BglFile::handleIlsVor(BinaryStream *bs)
{
  // Read only type before creating concrete object
  IlsVor iv(options, bs);
  iv.seekToStart();

  switch(iv.getType())
  {
    case nav::TERMINAL:
    case nav::LOW:
    case nav::HIGH:
    case nav::VOT:
      if(options->includeBglObject(type::VOR))
        return createRecord<Vor>(options, bs, &vors);

      break;
    case nav::ILS:
      if(options->includeBglObject(type::ILS))
        return createRecord<Ils>(options, bs, &ils);
  }
  return nullptr;
}

void BglFile::readRecords(BinaryStream *bs)
{
  for(Subsection& it : subsections)
  {
    section::SectionType type = it.getParent().getType();

    if(options->isVerbose())
    {
      qDebug() << "=======================";
      qDebug().nospace().noquote() << "Records of 0x" << hex << it.getFirstDataRecordOffset() << dec
                                   << " type " << sectionTypeStr(type);
    }

    bs->seekg(it.getFirstDataRecordOffset());

    int numRec = it.getNumDataRecords();

    if(type == section::NAME_LIST)
      numRec = 1;

    for(int i = 0; i < numRec; i++)
    {
      const Record *rec = nullptr;

      switch(type)
      {
        case section::AIRPORT:
          if(options->includeBglObject(type::AIRPORT))
            // Will return null iF ICAO is excluded
            rec = createRecord<Airport>(options, bs, &airports);
          break;
        case section::NAME_LIST:
          rec = createRecord<Namelist>(options, bs, &namelists);
          break;
        case section::ILS_VOR:
          rec = handleIlsVor(bs);
          break;
        case section::NDB:
          if(options->includeBglObject(type::NDB))
            rec = createRecord<Ndb>(options, bs, &ndbs);
          break;
        case section::MARKER:
          if(options->includeBglObject(type::MARKER))
            rec = createRecord<Marker>(options, bs, &marker);
          break;
        case section::WAYPOINT:
          if(options->includeBglObject(type::WAYPOINT))
            rec = createRecord<Waypoint>(options, bs, &waypoints);
          break;

        case atools::fs::bgl::section::NONE:
        case atools::fs::bgl::section::COPYRIGHT:
        case atools::fs::bgl::section::GUID:
        case atools::fs::bgl::section::BOUNDARY:
        case atools::fs::bgl::section::GEOPOL:
        case atools::fs::bgl::section::SCENERY_OBJECT:
        case atools::fs::bgl::section::VOR_ILS_ICAO_INDEX:
        case atools::fs::bgl::section::NDB_ICAO_INDEX:
        case atools::fs::bgl::section::WAYPOINT_ICAO_INDEX:
        case atools::fs::bgl::section::MODEL_DATA:
        case atools::fs::bgl::section::AIRPORT_SUMMARY:
        case atools::fs::bgl::section::EXCLUSION:
        case atools::fs::bgl::section::TIMEZONE:
        case atools::fs::bgl::section::TERRAIN_VECTOR_DB:
        case atools::fs::bgl::section::TERRAIN_ELEVATION:
        case atools::fs::bgl::section::TERRAIN_LAND_CLASS:
        case atools::fs::bgl::section::TERRAIN_WATER_CLASS:
        case atools::fs::bgl::section::TERRAIN_REGION:
        case atools::fs::bgl::section::POPULATION_DENSITY:
        case atools::fs::bgl::section::AUTOGEN_ANNOTATION:
        case atools::fs::bgl::section::TERRAIN_INDEX:
        case atools::fs::bgl::section::TERRAIN_TEXTURE_LOOKUP:
        case atools::fs::bgl::section::TERRAIN_SEASON_JAN:
        case atools::fs::bgl::section::TERRAIN_SEASON_FEB:
        case atools::fs::bgl::section::TERRAIN_SEASON_MAR:
        case atools::fs::bgl::section::TERRAIN_SEASON_APR:
        case atools::fs::bgl::section::TERRAIN_SEASON_MAY:
        case atools::fs::bgl::section::TERRAIN_SEASON_JUN:
        case atools::fs::bgl::section::TERRAIN_SEASON_JUL:
        case atools::fs::bgl::section::TERRAIN_SEASON_AUG:
        case atools::fs::bgl::section::TERRAIN_SEASON_SEP:
        case atools::fs::bgl::section::TERRAIN_SEASON_OCT:
        case atools::fs::bgl::section::TERRAIN_SEASON_NOV:
        case atools::fs::bgl::section::TERRAIN_SEASON_DEC:
        case atools::fs::bgl::section::TERRAIN_PHOTO_JAN:
        case atools::fs::bgl::section::TERRAIN_PHOTO_FEB:
        case atools::fs::bgl::section::TERRAIN_PHOTO_MAR:
        case atools::fs::bgl::section::TERRAIN_PHOTO_APR:
        case atools::fs::bgl::section::TERRAIN_PHOTO_MAY:
        case atools::fs::bgl::section::TERRAIN_PHOTO_JUN:
        case atools::fs::bgl::section::TERRAIN_PHOTO_JUL:
        case atools::fs::bgl::section::TERRAIN_PHOTO_AUG:
        case atools::fs::bgl::section::TERRAIN_PHOTO_SEP:
        case atools::fs::bgl::section::TERRAIN_PHOTO_OCT:
        case atools::fs::bgl::section::TERRAIN_PHOTO_NOV:
        case atools::fs::bgl::section::TERRAIN_PHOTO_DEC:
        case atools::fs::bgl::section::TERRAIN_PHOTO_NIGHT:
        case atools::fs::bgl::section::FAKE_TYPES:
        case atools::fs::bgl::section::ICAO_RUNWAY:
          break;
      }
      if(rec == nullptr)
        // Create empty record, just to skip it
        rec = createRecord<Record>(options, bs, nullptr);
      rec->seekToEnd();
    }
  }
}

void BglFile::freeObjects()
{
  qDeleteAll(allRecords);

  allRecords.clear();

  airports.clear();
  namelists.clear();
  ils.clear();
  vors.clear();
  ndbs.clear();
  marker.clear();
  waypoints.clear();
  sections.clear();
  subsections.clear();

  filename = "";
  size = 0;
}

} // namespace bgl
} // namespace fs
} // namespace atools
