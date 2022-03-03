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

#ifndef ATOOLS_AIRSPACEREADER_BASE_H
#define ATOOLS_AIRSPACEREADER_BASE_H

#include <QCoreApplication>
#include <QVector>

namespace atools {
namespace geo {
class LineString;
class Pos;
}

namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace userdata {

/*
 * Base class for reading airspace text formats. Provides SQL query and error collection methods.
 */
class AirspaceReaderBase
{
  Q_DECLARE_TR_FUNCTIONS(AirspaceWriter)

public:
  AirspaceReaderBase(sql::SqlDatabase *sqlDb);
  virtual ~AirspaceReaderBase();

  AirspaceReaderBase(const AirspaceReaderBase& other) = delete;
  AirspaceReaderBase& operator=(const AirspaceReaderBase& other) = delete;

  /* Read a whole file and write airspaces into table. Returns false if the file is not in the supported format. */
  virtual bool readFile(const QString& filenameParam) = 0;

  /* Read a line from a file and write to file if end of airspace detected */
  virtual void readLine(const QStringList& line, int fileIdParam, const QString& filenameParam, int lineNumberParam);

  /* Writes last airspace to table */
  virtual void finish();

  /* reset internal values back */
  virtual void reset();

  /* Clear list of errors */
  void resetErrors();

  /* Set number of read airspaces back to 0 */
  void resetNumRead();

  /* Error context */
  struct AirspaceErr
  {
    QString file, message;
    int line;
  };

  /* Number of airspaces read */
  int getNumAirspacesRead() const
  {
    return numAirspacesRead;
  }

  /* Set to a function that returns the coordinates for an airport ident. */
  void setFetchAirportCoords(const std::function<atools::geo::Pos(const QString&)>& value)
  {
    fetchAirportCoords = value;
  }

  /* For column "file_id" in database */
  void setFileId(int value)
  {
    fileId = value;
  }

  /* Get errors after reading file or line */
  const QVector<AirspaceErr>& getErrors() const
  {
    return errors;
  }

  /* Set first boundary_id to be used */
  void setAirspaceId(int value)
  {
    airspaceId = value;
  }

  int getNextAirspaceId() const
  {
    return airspaceId;
  }

  enum Format
  {
    UNKNOWN,
    OPEN_AIR, /* OpenAir format */
    IVAO_JSON, /* IVAO special JSON format */
    GEO_JSON /* GeoJSON format as used by various sources for VATSIM */
  };

  /* Try to detect the file format. This might not work 100 percent for JSON in which case a
   * reader returns nothing for a not matching JSON schema */
  static Format detectFileFormat(const QString& file);

protected:
  void initQueries();
  void deInitQueries();

  void errWarn(const QString& msg);
  QString mid(const QStringList& line, int index, bool ignoreError = false);

  atools::sql::SqlQuery *insertAirspaceQuery = nullptr;
  atools::sql::SqlDatabase *db;

  QString filename;
  int airspaceId = 1;
  int lineNumber = 0;
  int numAirspacesRead = 0;
  int fileId = 0;
  QVector<AirspaceErr> errors;

  /* Callback to get airport coodinates by ICAO ident */
  std::function<atools::geo::Pos(const QString&)> fetchAirportCoords;
};

} // namespace userdata
} // namespace fs
} // namespace atools

#endif // ATOOLS_AIRSPACEREADER_BASE_H
