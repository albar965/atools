/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_BGL_BGLFILE_H
#define ATOOLS_BGL_BGLFILE_H

#include "fs/bgl/header.h"
#include "fs/bgl/sectiontype.h"
#include "fs/navdatabaseoptions.h"
#include "util/flags.h"

#include <QString>
#include <QList>
#include <QDebug>
#include <QCoreApplication>

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {

class NavDatabaseOptions;

namespace scenery {
class SceneryArea;
}
namespace bgl {

// -------------------------------------------------------------------
enum CreateFlag : quint32
{
  NO_CREATE_FLAGS = 0,
  AIRPORT_FS9_FORMAT = 1 << 0,
  AIRPORT_FSX_FORMAT = 1 << 1,
  AIRPORT_MSFS_DUMMY = 1 << 2,
  AIRPORT_MSFS_NAVIGRAPH_NAVDATA = 1 << 3,
};

ATOOLS_DECLARE_FLAGS_32(CreateFlags, atools::fs::bgl::CreateFlag)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::bgl::CreateFlags)

class Section;
class Subsection;
class Header;
class Ils;
class Vor;
class Tacan;
class Ndb;
class Marker;
class Waypoint;
class Airport;
class Namelist;
class Record;
class Boundary;

/*
 * Class for reading a full BGL file into its containers.
 */
class BglFile
{
  Q_DECLARE_TR_FUNCTIONS(BglFile)

public:
  /*
   * @param readerOptions Configuration.
   */
  explicit BglFile(const NavDatabaseOptions *readerOptions);
  virtual ~BglFile();

  void setSupportedSectionTypes(const QSet<atools::fs::bgl::section::SectionType>& sects)
  {
    supportedSectionTypes = sects;
  }

  /*
   * Reads the full content of the BGL file into the internal lists including header, sections,
   * airports and so on.
   * @param file BGL filename
   */
  void readFile(const QString& filenameParam, const scenery::SceneryArea& area);

  QString getFilepath() const
  {
    return filename;
  }

  /*
   * @return Size on the filesystem
   */
  qint64 getFilesize() const
  {
    return size;
  }

  const QList<const atools::fs::bgl::Airport *>& getAirports() const
  {
    return airports;
  }

  const atools::fs::bgl::Header& getHeader() const
  {
    return header;
  }

  const QList<const atools::fs::bgl::Ils *>& getIls() const
  {
    return ils;
  }

  const QList<const atools::fs::bgl::Marker *>& getMarker() const
  {
    return marker;
  }

  const QList<const atools::fs::bgl::Namelist *>& getNamelists() const
  {
    return namelists;
  }

  const QList<const atools::fs::bgl::Ndb *>& getNdbs() const
  {
    return ndbs;
  }

  const QList<const atools::fs::bgl::Vor *>& getVors() const
  {
    return vors;
  }

  const QList<const atools::fs::bgl::Tacan *>& getTacans() const
  {
    return tacans;
  }

  const QList<const atools::fs::bgl::Waypoint *>& getWaypoints() const
  {
    return waypoints;
  }

  const QList<const atools::fs::bgl::Boundary *>& getBoundaries() const
  {
    return boundaries;
  }

  /*
   * @return true if any relevant content is available. Header and sections do not count.
   */
  bool hasContent();

  /*
   * @return true if header and section structure is valid
   */
  bool isValid();

private:
  void deleteAllObjects();
  void readHeader(atools::io::BinaryStream *bs);
  void readSections(atools::io::BinaryStream *bs);

  void readRecords(atools::io::BinaryStream *bs, const atools::fs::scenery::SceneryArea& area);
  const Record *handleIlsVor(atools::io::BinaryStream *bs);

  /* Boundaries are a special mess since it is not well documented */
  void readBoundaryRecords(atools::io::BinaryStream *bs);
  void handleBoundaries(atools::io::BinaryStream *bs);

  template<typename TYPE>
  const TYPE *createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list);

  template<typename TYPE>
  const TYPE *createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list,
                           atools::fs::bgl::CreateFlags flags);

  QString filename;
  qint64 size;
  const NavDatabaseOptions *options = nullptr;

  /* Keep a list of all records to make object deletion easier */
  QList<const atools::fs::bgl::Record *> allRecords;

  QList<const atools::fs::bgl::Airport *> airports;
  QList<const atools::fs::bgl::Namelist *> namelists;
  QList<const atools::fs::bgl::Vor *> vors;
  QList<const atools::fs::bgl::Tacan *> tacans;
  QList<const atools::fs::bgl::Ils *> ils;
  QList<const atools::fs::bgl::Ndb *> ndbs;
  QList<const atools::fs::bgl::Marker *> marker;
  QList<const atools::fs::bgl::Waypoint *> waypoints;
  QList<const atools::fs::bgl::Boundary *> boundaries;

  QList<atools::fs::bgl::Section> sections;
  QList<atools::fs::bgl::Subsection> subsections;
  atools::fs::bgl::Header header;

  /* Only these sections will be scanned for records */
  QSet<atools::fs::bgl::section::SectionType> supportedSectionTypes;
};

// -------------------------------------------------------------------

template<typename TYPE>
const TYPE *BglFile::createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list)
{
  TYPE *rec = new TYPE(options, bs);

  if(rec->isExcluded())
  {
    delete rec;
    return nullptr;
  }

  if(!rec->isValid())
  {
    // Print warning only for navaids that are not disabled
    if(!rec->isDisabled())
      qWarning() << "Found invalid record: " << rec->getObjectName();
    rec->seekToStart();
    delete rec;
    return nullptr;
  }

  if(options->isVerbose())
  {
    qDebug() << "----";
    qDebug() << *rec;
  }

  if(list != nullptr)
    list->append(rec);
  allRecords.append(rec);
  return rec;
}

template<typename TYPE>
const TYPE *BglFile::createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list,
                                  atools::fs::bgl::CreateFlags flags)
{
  TYPE *rec = new TYPE(options, bs, flags);

  if(rec->isExcluded())
  {
    delete rec;
    return nullptr;
  }

  if(!rec->isValid())
  {
    // Print warning only for navaids that are not disabled
    if(!rec->isDisabled())
      qWarning() << "Found invalid record: " << rec->getObjectName();
    rec->seekToStart();
    delete rec;
    return nullptr;
  }

  if(options->isVerbose())
  {
    qDebug() << "----";
    qDebug() << *rec;
  }

  if(list != nullptr)
    list->append(rec);
  allRecords.append(rec);

  return rec;
}

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_BGLFILE_H
