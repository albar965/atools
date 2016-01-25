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

BglFile::BglFile(const BglReaderOptions& opts)
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

  if(options.isVerbose())
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

void BglFile::readHeaderAndSections(BinaryStream *bs)
{
  header = Header(bs);
  if(options.isVerbose())
    qDebug() << header;

  // Section pointer
  for(unsigned int i = 0; i < header.getNumSections(); i++)
  {
    Section s = Section(bs);
    if(std::binary_search(supportedSectionTypes.begin(), supportedSectionTypes.end(), s.getType()))
    {
      if(options.isVerbose())
        qDebug() << s;
      sections.push_back(s);
    }
  }

  for(Section& it : sections)
  {
    bs->seekg(it.getFirstSubsectionOffset());
    for(int i = 0; i < it.getNumSubsections(); i++)
    {
      Subsection s(bs, it);
      if(options.isVerbose())
        qDebug() << s;
      subsections.push_back(s);
    }
  }
}

const Record *BglFile::handleIlsVor(BinaryStream *bs)
{
  IlsVor iv(bs);
  iv.seekToStart();
  switch(iv.getType())
  {
    case nav::TERMINAL:
    case nav::LOW:
    case nav::HIGH:
    case nav::VOT:
      return createRecord<Vor>(bs, &vors);

    case nav::ILS:
      return createRecord<Ils>(bs, &ils);
  }
  return nullptr;
}

void BglFile::readRecords(BinaryStream *bs)
{
  for(Subsection& it : subsections)
  {
    section::SectionType type = it.getParent().getType();

    if(options.isVerbose())
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
          rec = createRecord<Airport>(bs, &airports);
          break;
        case section::NAME_LIST:
          rec = createRecord<Namelist>(bs, &namelists);
          break;
        case section::ILS_VOR:
          rec = handleIlsVor(bs);
          break;
        case section::NDB:
          rec = createRecord<Ndb>(bs, &ndbs);
          break;
        case section::MARKER:
          rec = createRecord<Marker>(bs, &marker);
          break;
        case section::WAYPOINT:
          rec = createRecord<Waypoint>(bs, &waypoints);
          break;
        default:
          rec = createRecord<Record>(bs, nullptr);
          break;
      }
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
