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

#ifndef ATOOLS_SQL_SQLTYPES_H
#define ATOOLS_SQL_SQLTYPES_H

#include <QList>

class QSqlResult;

namespace atools {
namespace sql {

class SqlRecord;

typedef QList<atools::sql::SqlRecord> SqlRecordList;

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLTYPES_H
