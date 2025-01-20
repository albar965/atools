/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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
namespace db {

class SimConnectWriter;
class SimConnectLoaderPrivate;

/* Message, incProgress = true to increment progress, otherwise "still alive" message. Numbers are
 * loaded features so far.  */
typedef std::function<bool (const QString& message, bool incProgress, int airportsLoaded,
                            int waypointsLoaded, int vorLoaded, int ilsLoaded, int ndbLoaded)> SimConnectLoaderProgressCallback;

/*
 * Fetches airports, runways, starts, parking taxiways, all procedures and navaids via
 * SimConnect and writes them to the given database.
 * Usable for MSFS 2024.
 *
 * Note that navaids are limited to the ones connected to airways and procedures.
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

  SimConnectLoader(const SimConnectLoader& other) = delete;
  SimConnectLoader& operator=(const SimConnectLoader& other) = delete;

  /* Prepare loading before calling any of the load...() methods. Opens SimConnect and optionally initializes database queries */
  bool prepareLoading(bool loadFacilityDefinitions, bool initSqlQueries);

  /* Close queries and clear containers to free memory */
  bool finishLoading();

  /* Number of times the callback will be called with incProgress = true */
  int getNumSteps() const;

  /* Get all airport details for all airport idents and write airports, runways and procedures to the database.
   * fileId is used to fill airport.file_id field for all airports.
   * Also catches navaids from procedure references.
   * A globbing filter can be set in setAirportIdents() to limit number of airports loaded. */
  bool loadAirports(int fileId);

  /* Load all navaids (VOR, NDB, waypoints, airways and ILS) that were refernced from loading airport procedures and
   * write VOR, NDB, waypoints, airways and ILS to the database.
   * Traverses airway network to load more navaids. Navaids catched in loadAirports() are used as starting point
   * to traverse airway network. */
  bool loadNavaids(int fileId);

  /* Load VOR and NDB which are not connected to procedures or airways. Requires previous call to loadNavaids() to
   * avoid loading duplicates. */
  bool loadDisconnectedNavaids(int fileId);

  /* Progress callback returned true */
  bool isAborted() const;

  /* Set batch size that is used to fetch facilities.
   * The program reads until the batch size is exceeded and then calls the dispatch function until done. */
  void setBatchSize(int value);

  /* Process failed and loading is probably incomplete if errors are returned. */
  const QStringList& getErrors() const;

  bool hasErrors() const
  {
    return !getErrors().isEmpty();
  }

  /* Set airport idents to load detailed information from. This overrides idents in loadAirports().
   * Globbing with * and ? is used. */
  void setAirportIdents(const QList<QRegExp>& airportIcaoFiltersInc);

  /* Progress callback called before writing a batch to the database. Return true to abort loading. */
  void setProgressCallback(const SimConnectLoaderProgressCallback& callback);

  /* Delay in milliseconds between CallDispatch calls */
  void setAirportFetchDelay(int value);
  void setNavaidFetchDelay(int value);

private:
  // Hides implementation details from the header
  SimConnectLoaderPrivate *p = nullptr;
};

} // namespace db
} // namespace sc
} // namespace fs
} // namespace atools

#endif // ATOOLS_SIMCONNECTLOADER_H
