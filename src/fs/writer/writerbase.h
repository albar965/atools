/*
 * WaypointWriter.h
 *
 *  Created on: 01.05.2015
 *      Author: alex
 */

#ifndef WRITER_WRITERBASE_H_
#define WRITER_WRITERBASE_H_

#include "writerbasebasic.h"

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

namespace writer {
class DataWriter;

template<class TYPE>
class WriterBase :
  public atools::fs::writer::WriterBaseBasic
{
public:
  WriterBase(atools::sql::SqlDatabase& db,
             DataWriter& dataWriter,
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
                                                     DataWriter& dataWriter,
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
      TYPE t = *it;
      writeOne(&t);
    }
}

} // namespace writer
} // namespace fs
} // namespace atools

#endif /* WRITER_WRITERBASE_H_ */
