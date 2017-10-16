/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#ifndef ATOOLS_DFDDATACOMPILER_H
#define ATOOLS_DFDDATACOMPILER_H

#include "geo/rect.h"

#include <QString>

namespace atools {
namespace sql {
class SqlDatabase;
class SqlQuery;
class SqlRecordVector;
class SqlRecord;
}
namespace fs {

namespace common {
class MagDecReader;
class MetadataWriter;
class AirportIndex;
}

class NavDatabaseOptions;
class NavDatabaseErrors;
class ProgressHandler;

namespace ng {

class DfdCompiler
{
public:
  DfdCompiler(atools::sql::SqlDatabase& sqlDb, const atools::fs::NavDatabaseOptions& opts,
                        atools::fs::ProgressHandler *progressHandler, atools::fs::NavDatabaseErrors *navdatabaseErrors);
  virtual ~DfdCompiler();

  void close();

  const QString& getAiracCycle() const
  {
    return airacCycle;
  }

  void compileMagDeclBgl();
  void readHeader();

  void writeFileAndSceneryMetadata();

  void attachDatabase();
  void detachDatabase();

  void writeAirports();
  void writeRunways();

  void writeNavaids();
  void updateMagvar();
  void updateTacanChannel();
  void updateIlsGeometry();
  void writeAirways();

  void initQueries();
  void deInitQueries();

private:
  const int ILS_FEATHER_LEN_NM = 9;

  void writeRunwaysForAirport(sql::SqlRecordVector& runways, const QString& apt);
  void pairRunways(QVector<std::pair<atools::sql::SqlRecord, atools::sql::SqlRecord> >& runwaypairs,
                   sql::SqlRecordVector& runways);

  const int ID_OFFSET = 100000000;

  const atools::fs::NavDatabaseOptions& options;
  atools::sql::SqlDatabase& db;
  atools::fs::ProgressHandler *progress = nullptr;
  atools::fs::NavDatabaseErrors *errors = nullptr;
  atools::fs::common::MagDecReader *magDecReader = nullptr;
  atools::fs::common::AirportIndex *airportIndex = nullptr;
  atools::fs::common::MetadataWriter *metadataWriter = nullptr;
  int curAirportId = ID_OFFSET, curRunwayId = ID_OFFSET, curRunwayEndId = ID_OFFSET;
  const int FILE_ID = 1, SCENERY_ID = 1;
  QString airacCycle;

  QHash<QString, QString> longestRunwaySurfaceMap;
  QHash<QString, atools::geo::Rect> airportRectMap;

  atools::sql::SqlQuery *airportQuery = nullptr, *airportWriteQuery = nullptr, *airportUpdateQuery = nullptr,
                        *runwayQuery = nullptr, *runwayWriteQuery = nullptr, *runwayEndWriteQuery = nullptr,
                        *metadataQuery = nullptr;

};

} // namespace ng
} // namespace fs
} // namespace atools

#endif // ATOOLS_DFDDATACOMPILER_H
