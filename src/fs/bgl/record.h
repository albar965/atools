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

#ifndef ATOOLS_BGL_RECORD_H
#define ATOOLS_BGL_RECORD_H

#include "fs/bgl/bglbase.h"

namespace atools {
namespace io {
class BinaryStream;
}

namespace fs {
namespace bgl {

class Subsection;

/* Identifier for simulator type which is filled dependent on record type and passed to records. */
enum StructureType
{
  STRUCT_FS9,
  STRUCT_FSX,
  STRUCT_P3DV4,
  STRUCT_P3DV5,
  STRUCT_MSFS
};

/*
 * Base for all record types.
 */
class Record :
  public atools::fs::bgl::BglBase
{
public:
  Record()
    : id(0), size(0)
  {
  }

  Record(const atools::fs::bgl::Record& other)
    : atools::fs::bgl::BglBase(other)
  {
    this->operator=(other);

  }

  Record& operator=(const atools::fs::bgl::Record& other)
  {
    id = other.id;
    size = other.size;
    excluded = other.excluded;
    return *this;
  }

  /*
   * Reads the first part of the record namely the record id and the record size.
   * The stream is advanced by 6 bytes for this.
   */
  Record(const atools::fs::NavDatabaseOptions *options, atools::io::BinaryStream *bs);

  virtual ~Record() override;

  /*
   * Seek past the end of the record to a new record.
   */
  void seekToEnd() const;

  /*
   * @return total record size in bytes
   */
  int getSize() const
  {
    return static_cast<int>(size);
  }

  /* Get the record id casted into an enum */
  template<typename ENUM>
  ENUM getId() const
  {
    return static_cast<ENUM>(id);
  }

  unsigned int getId() const
  {
    return id;
  }

  /*
   * @return true if this record was found to be excluded after reading due to configuration or reading errors
   */
  virtual bool isExcluded() const
  {
    return excluded;
  }

  /*
   * @return true if navaid was disabled by moving to pole
   */
  virtual bool isDisabled() const
  {
    return false;
  }

  /* Byte size that will be read by this class */
  const int SIZE = 6;

  /* Default to be implemented by derived. */
  virtual bool isValid() const
  {
    return true;
  }

  /* Checks if id and record size is within bounds. */
  virtual bool isFullyValid() const;

  virtual QString getObjectName() const;

  /* Validate the size in the subrecord to avoid an endless loop when it is zero.
   * If zero rewind current record, print a warning, set to excluded and return true */
  bool checkSubRecord(const Record& r);

protected:
  friend QDebug operator<<(QDebug out, const atools::fs::bgl::Record& record);

  unsigned int id, size;
  bool excluded = false;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif // ATOOLS_BGL_RECORD_H
