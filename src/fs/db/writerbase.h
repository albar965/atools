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

#ifndef ATOOLS_FS_DB_WRITERBASE_H
#define ATOOLS_FS_DB_WRITERBASE_H

#include "fs/db/writerbasebasic.h"

#include <QList>

namespace atools {
namespace sql {
class SqlDatabase;
}

namespace fs {
namespace bgl {
class BglFile;
}

class NavDatabaseOptions;

namespace db {
class DataWriter;

/*
 * Template base class for all writers classes that store BGL record data into the database.
 */
template<class TYPE>
class WriterBase :
  public atools::fs::db::WriterBaseBasic
{
public:
  /*
   * @param sqlDb an open database
   * @param dw datawriter as parent that keeps all writers
   * @param tablename table to insert content. An prepared insert statement including all columns
   *        will be generated from this
   * @param sqlParam custom insert statement to insert data. If this is set tablename will be ignored.
   */
  WriterBase(atools::sql::SqlDatabase& db,
             atools::fs::db::DataWriter& dataWriter,
             const QString& tablename,
             const QString& sqlParam = QString());

  virtual ~WriterBase();

  typedef QList<const TYPE *> TypePtrVector;
  typedef QList<TYPE> TypeVector;

  /* convenience methods for writing references, pointers and lists of TYPE */
  void writeOne(const TYPE& t);
  void writeOne(const TYPE *t);
  void write(const TypePtrVector& types);
  void write(const TypeVector& types);

  /*
   * @return current unchanged database id for this writer
   */
  int getCurrentId() const
  {
    return id;
  }

  /*
   * Increase database id and return it to caller
   * @return database id for this writer
   */
  int getNextId()
  {
    return ++id;
  }

protected:
  /*
   * Actual writing of BGL records to the database is done here which has to
   * be implemented by the concrete classes
   * @param type record to write
   */
  virtual void writeObject(const TYPE *type) = 0;

private:
  static int id;
};

// -----------------------------------------------------------------------------

template<typename TYPE>
int WriterBase<TYPE>::id = 0;

template<typename TYPE> WriterBase<TYPE>::WriterBase(sql::SqlDatabase& db,
                                                     atools::fs::db::DataWriter& dataWriter,
                                                     const QString& tablename,
                                                     const QString& sqlParam)
  : WriterBaseBasic(db, dataWriter, tablename, sqlParam)
{
}

template<typename TYPE> WriterBase<TYPE>::~WriterBase()
{
}

template<typename TYPE>
void WriterBase<TYPE>::writeOne(const TYPE *t)
{
  writeObject(t);
}

template<typename TYPE>
void WriterBase<TYPE>::writeOne(const TYPE& t)
{
  writeObject(&t);
}

template<typename TYPE>
void WriterBase<TYPE>::write(const TypePtrVector& types)
{
  for(const TYPE *type : types)
    writeOne(type);
}

template<typename TYPE>
void WriterBase<TYPE>::write(const TypeVector& types)
{
  for(const TYPE& type : types)
    writeOne(type);
}

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_DB_WRITERBASE_H
