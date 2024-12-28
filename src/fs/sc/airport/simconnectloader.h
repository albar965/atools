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

#ifndef ATOOLS_SIMCONNECTLOADER_H
#define ATOOLS_SIMCONNECTLOADER_H

#include <QMap>
#include <QCoreApplication>

class QRegExp;

namespace atools {

namespace win {
class ActivationContext;
}

namespace sql {
class SqlDatabase;
}
namespace fs {
namespace sc {
namespace airport {

class SimConnectWriter;
class SimConnectLoaderPrivate;

/* First and last airport ident in batch, size of batch and total airports written after filtering */
typedef std::function<bool (const QString& identFrom, const QString& identTo, int batchSize,
                            int totalWritten)> SimConnectLoaderProgressCallback;

/*
 * Fetches airports, runways, starts, parking taxiways and all procedures via SimConnect and writes them to the given database.
 * Usable for MSFS 2024.
 *
 * Class is not reentrant.
 */
class SimConnectLoader
{
  Q_DECLARE_TR_FUNCTIONS(SimConnectLoader)

public:
  /* Library has to be loaded before */
  SimConnectLoader(const win::ActivationContext *activationContext, const QString& libraryName, atools::sql::SqlDatabase& sqlDb,
                   bool verbose);
  ~SimConnectLoader();

  /* Fetch all world-wide available airport idents from SimConnect. Returns empty list on error. */
  QStringList loadAirportIdents() const;

  /* Get all airport details for given airport idents. fileId is used to fill airport.file_id field for all airports. */
  bool loadAirports(const QStringList& idents, int fileId);

  /* Progress callback returned true */
  bool isAborted() const;

  /* Set batch size that is used to fetch airports.
   * The program reads airports until the batch size is exceeded and then writes the structures to the database. */
  void setBatchSize(int value);

  /* Process failed and loading is probably incomplete if errors are returned. */
  const QStringList& getErrors() const;

  bool hasErrors() const
  {
    return !getErrors().isEmpty();
  }

  /* Set airport idents to load detailed information from. This overrides idents in loadAirports() */
  void setAirportIdents(const QList<QRegExp>& airportIcaoFiltersInc);

  /* Progress callback called before writing a batch to the database. Return true to abort loading. */
  void setProgressCallback(const SimConnectLoaderProgressCallback& callback);

private:
  // Hides implementation details from the header
  SimConnectLoaderPrivate *p = nullptr;
};

} // namespace airport
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTLOADER_H
