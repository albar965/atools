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

#ifndef ATOOLS_WRITER_WRITERBASE_H
#define ATOOLS_WRITER_WRITERBASE_H

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

class BglReaderOptions;

namespace db {
class DataWriter;

template<class TYPE>
class WriterBase :
  public atools::fs::db::WriterBaseBasic
{
public:
  WriterBase(atools::sql::SqlDatabase& db,
             atools::fs::db::DataWriter& dataWriter,
             const QString& tablename,
             const QString& sqlParam = "");

  virtual ~WriterBase();

  typedef QList<const TYPE *> TypePtrVector;
  typedef typename TypePtrVector::const_iterator TypePtrConstIter;
  typedef QList<TYPE> TypeVector;
  typedef typename TypeVector::const_iterator TypeConstIter;

  void writeOne(const TYPE *t);
  void writeOne(const TYPE& t);
  void write(const TypePtrVector& types);
  void write(const TypeVector& types);

  int getCurrentId() const
  {
    return id;
  }

  int getNextId()
  {
    return ++id;
  }

protected:
  virtual void writeObject(const TYPE *type) = 0;

  int getCurrentFileId() const;

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
  if(!types.empty())
    for(TypePtrConstIter it = types.begin(); it != types.end(); ++it)
      writeOne(*it);
}

template<typename TYPE>
void WriterBase<TYPE>::write(const TypeVector& types)
{
  if(!types.empty())
    for(TypeConstIter it = types.begin(); it != types.end(); ++it)
    {
      const TYPE& t = *it;
      writeOne(&t);
    }
}

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ATOOLS_WRITER_WRITERBASE_H
