/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_FS_COMMON_MAGDECREADER_H
#define ATOOLS_FS_COMMON_MAGDECREADER_H

#include <QByteArray>
#include <QDate>

namespace atools {
namespace geo {
class Pos;
}
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace common {

/*
 * Loads and parses the magdec.bgl file. Allows to store declination into the magdecl table in a database.
 */
class MagDecReader
{
public:
  MagDecReader();
  virtual ~MagDecReader();

  /* Read values from magdec.bgl file */
  void readFromBgl(const QString& filename);

  /* Read values from table "magdecl" returns true if successfull and table exists. */
  bool readFromTable(atools::sql::SqlDatabase& db);

  /* Writes values to table "magdecl". Object has to be valid */
  void writeToTable(atools::sql::SqlDatabase& db) const;

  /* Frees memory and sets state to invalid */
  void clear();

  /* true if loaded */
  bool isValid() const
  {
    return magDeclValues != nullptr;
  }

  /* East values are positive while West values are negative.
   * Throws exception if object is not valid.
   */
  float getMagVar(const atools::geo::Pos& pos) const;

  const QDate& getReferenceDate() const
  {
    return referenceDate;
  }

private:
  QByteArray writeToBytes() const;
  void readFromBytes(const QByteArray& bytes);

  int offset(int lonX, int latY) const;
  float magvar(int offset) const;

  QDate referenceDate;
  quint32 numValues = 0;
  float *magDeclValues = nullptr;
};

} // namespace common
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_COMMON_MAGDECREADER_H
