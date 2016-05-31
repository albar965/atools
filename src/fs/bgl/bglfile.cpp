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
#include "fs/bgl/boundary.h"
#include "fs/bgl/recordtypes.h"

#include <QList>
#include "logging/loggingdefs.h"

#include <QFile>
#include <QFileInfo>

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

  QFile ifs(filename);
  if(ifs.open(QIODevice::ReadOnly))
  {
    BinaryStream bs(&ifs);

    this->filename = file;
    this->size = bs.getFileSize();

    readHeaderAndSections(&bs);

    if(options->includeBglObject(type::BOUNDARY))
      readBoundaries(&bs);

    readRecords(&bs);
    ifs.close();
  }
}

bool BglFile::hasContent()
{
  return !(airports.isEmpty() &&
           namelists.isEmpty() &&
           vors.isEmpty() &&
           ils.isEmpty() &&
           ndbs.isEmpty() &&
           marker.isEmpty() &&
           waypoints.isEmpty() &&
           boundaries.isEmpty());
}

void BglFile::handleBoundaries(BinaryStream *bs)
{
  // Read records until end of the file
  int numRecs = 0;
  while(bs->tellg() < bs->getFileSize())
  {
    Record rec(options, bs);
    rec::RecordType type = rec.getId<rec::RecordType>();

    if(type == rec::BOUNDARY)
    {
      rec.seekToStart();
      const Record *r = createRecord<Boundary>(options, bs, &boundaries);
      if(r != nullptr)
        numRecs++;
    }
    else if(type != rec::GEOPOL)
      qWarning().nospace() << "while reading boundaries: unexpected record "
                           << hex << "0x" << type << " at " << hex << "0x" << bs->tellg();

    rec.seekToEnd();
  }
  if(options->isVerbose())
    qDebug() << "Num boundary records" << numRecs;
}

void BglFile::readBoundaries(BinaryStream *bs)
{
  for(Section& it : sections)
    if(it.getType() == atools::fs::bgl::section::BOUNDARY)
    {
      QString fname = QFileInfo(filename).fileName().toLower();
      if(fname == "bvcf.bgl" || fname == "bnxworld0.bgl")
      {
        // jump to the end of the data (probably spatial indexes)
        bs->seekg(bs->tellg() + it.getTotalSubsectionSize());
        handleBoundaries(bs);
      }
      else
      {
        // BNXWorld1.bgl BNXWorld2.bgl BNXWorld3.bgl BNXWorld4.bgl BNXWorld5.bgl
        bs->seekg(it.getStartOffset() + it.getNumSubsections() * 16);

        // Get the lowest offset from the special subsection
        unsigned int minOffset = std::numeric_limits<int>::max();
        while(bs->tellg() < it.getStartOffset() + it.getTotalSubsectionSize())
        {
          unsigned int newOffset = bs->readUInt();
          unsigned int dataSize = bs->readUInt(); // size
          if(newOffset < minOffset && dataSize > 0)
            minOffset = newOffset;

          if(dataSize <= 0)
            qWarning().nospace() << "while reading boundaries: dataSize " << dataSize
                                 << " at " << hex << "0x" << bs->tellg();
        }

        // Read from the first offset
        bs->seekg(minOffset);
        handleBoundaries(bs);
      }
    }
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
    if(supportedSectionTypes.contains(s.getType()))
    {
      if(options->isVerbose())
        qDebug() << s;
      sections.push_back(s);
    }
  }

  for(Section& it : sections)
    if(it.getType() != atools::fs::bgl::section::BOUNDARY && it.getType() != atools::fs::bgl::section::GEOPOL)
    {
      bs->seekg(it.getFirstSubsectionOffset());
      for(unsigned int i = 0; i < it.getNumSubsections(); i++)
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

        case section::BOUNDARY:
        case section::GEOPOL:
        case section::NONE:
        case section::COPYRIGHT:
        case section::GUID:
        case section::SCENERY_OBJECT:
        case section::VOR_ILS_ICAO_INDEX:
        case section::NDB_ICAO_INDEX:
        case section::WAYPOINT_ICAO_INDEX:
        case section::MODEL_DATA:
        case section::AIRPORT_SUMMARY:
        case section::EXCLUSION:
        case section::TIMEZONE:
        case section::TERRAIN_VECTOR_DB:
        case section::TERRAIN_ELEVATION:
        case section::TERRAIN_LAND_CLASS:
        case section::TERRAIN_WATER_CLASS:
        case section::TERRAIN_REGION:
        case section::POPULATION_DENSITY:
        case section::AUTOGEN_ANNOTATION:
        case section::TERRAIN_INDEX:
        case section::TERRAIN_TEXTURE_LOOKUP:
        case section::TERRAIN_SEASON_JAN:
        case section::TERRAIN_SEASON_FEB:
        case section::TERRAIN_SEASON_MAR:
        case section::TERRAIN_SEASON_APR:
        case section::TERRAIN_SEASON_MAY:
        case section::TERRAIN_SEASON_JUN:
        case section::TERRAIN_SEASON_JUL:
        case section::TERRAIN_SEASON_AUG:
        case section::TERRAIN_SEASON_SEP:
        case section::TERRAIN_SEASON_OCT:
        case section::TERRAIN_SEASON_NOV:
        case section::TERRAIN_SEASON_DEC:
        case section::TERRAIN_PHOTO_JAN:
        case section::TERRAIN_PHOTO_FEB:
        case section::TERRAIN_PHOTO_MAR:
        case section::TERRAIN_PHOTO_APR:
        case section::TERRAIN_PHOTO_MAY:
        case section::TERRAIN_PHOTO_JUN:
        case section::TERRAIN_PHOTO_JUL:
        case section::TERRAIN_PHOTO_AUG:
        case section::TERRAIN_PHOTO_SEP:
        case section::TERRAIN_PHOTO_OCT:
        case section::TERRAIN_PHOTO_NOV:
        case section::TERRAIN_PHOTO_DEC:
        case section::TERRAIN_PHOTO_NIGHT:
        case section::FAKE_TYPES:
        case section::ICAO_RUNWAY:
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
  airports.clear();
  namelists.clear();
  ils.clear();
  vors.clear();
  ndbs.clear();
  marker.clear();
  waypoints.clear();
  boundaries.clear();
  sections.clear();
  subsections.clear();

  qDeleteAll(allRecords);
  allRecords.clear();

  filename = "";
  size = 0;
}

} // namespace bgl
} // namespace fs
} // namespace atools
