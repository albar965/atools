/*
 * BglFile.h
 *
 *  Created on: 26.04.2015
 *      Author: alex
 */

#ifndef BGL_BGLFILE_H_
#define BGL_BGLFILE_H_

#include "fs/bgl/sectiontype.h"
#include "fs/bgl/header.h"
#include "fs/bgl/section.h"
#include "fs/bgl/subsection.h"

#include "fs/bglreaderoptions.h"

#include <QString>
#include <QList>
#include <QDebug>

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {
class Ils;
class Vor;
class Ndb;
class Marker;
class Waypoint;
class Airport;
class Namelist;
class Record;

class BglFile
{
public:
  BglFile(const BglReaderOptions& opts);
  virtual ~BglFile();

  void setSupportedSectionTypes(const QList<atools::fs::bgl::section::SectionType>& sects)
  {
    supportedSectionTypes = sects;
  }

  void readFile(QString file);

  QString getFilename() const
  {
    return filename;
  }

  qint64 getFilesize() const
  {
    return size;
  }

  const QList<const atools::fs::bgl::Airport *>& getAirports() const
  {
    return airports;
  }

  const QList<const atools::fs::bgl::Record *>& getAllRecords() const
  {
    return allRecords;
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

  const QList<atools::fs::bgl::Section>& getSections() const
  {
    return sections;
  }

  const QList<atools::fs::bgl::Subsection>& getSubsections() const
  {
    return subsections;
  }

  const QList<const atools::fs::bgl::Vor *>& getVors() const
  {
    return vors;
  }

  const QList<const atools::fs::bgl::Waypoint *>& getWaypoints() const
  {
    return waypoints;
  }

private:
  void freeObjects();
  void readHeaderAndSections(atools::io::BinaryStream *bs);
  void readRecords(atools::io::BinaryStream *bs);
  const Record *handleIlsVor(atools::io::BinaryStream *bs);

  template<typename TYPE>
  const TYPE *createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list);

  QString filename;
  qint64 size;
  const BglReaderOptions& options;

  QList<const atools::fs::bgl::Record *> allRecords;
  QList<const atools::fs::bgl::Airport *> airports;
  QList<const atools::fs::bgl::Namelist *> namelists;
  QList<const atools::fs::bgl::Vor *> vors;
  QList<const atools::fs::bgl::Ils *> ils;
  QList<const atools::fs::bgl::Ndb *> ndbs;
  QList<const atools::fs::bgl::Marker *> marker;
  QList<const atools::fs::bgl::Waypoint *> waypoints;

  QList<atools::fs::bgl::Section> sections;
  QList<atools::fs::bgl::Subsection> subsections;
  atools::fs::bgl::Header header;

  QList<atools::fs::bgl::section::SectionType> supportedSectionTypes;
};

// -------------------------------------------------------------------

template<typename TYPE>
const TYPE *BglFile::createRecord(atools::io::BinaryStream *bs, QList<const TYPE *> *list)
{
  TYPE *rec = new TYPE(bs);

  if(options.isVerbose())
  {
    qDebug() << "----";
    qDebug() << *rec;
  }

  if(list != nullptr)
    list->push_back(rec);
  allRecords.push_back(rec);
  return rec;
}

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_BGLFILE_H_ */
