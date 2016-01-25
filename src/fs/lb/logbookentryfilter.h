#ifndef LOGBOOKENTRYFILTER_H
#define LOGBOOKENTRYFILTER_H

namespace atools {
namespace fs {
namespace lb {

class LogbookEntry;

/*
 * Defines which logbook entries should be omitted when loading the BIN-file.
 * Per default nothing is omitted.
 */
class LogbookEntryFilter
{
public:
  LogbookEntryFilter()
  {

  }

  /* Do not include entry if flight time is lower than the given in minutes. */
  LogbookEntryFilter& flightTimeLowerThan(int minutes = 5);

  /* Do not include entry if start and destination ICAO are the same. */
  LogbookEntryFilter& startAndDestSame(bool flag = true);

  /* Do not include if start AND destination ICAO are empty. */
  LogbookEntryFilter& startAndDestEmpty(bool flag = true);

  /* Do not include if either start or destination ICAO are empty. */
  LogbookEntryFilter& startOrDestEmpty(bool flag = true);

  /* Omit entries with invalid date */
  LogbookEntryFilter& invalidDate(bool flag = true);

  /* @return true if the entries passes the filter and can be stored */
  bool canStore(const atools::fs::lb::LogbookEntry& entry) const;

private:
  int minFlightTime = 0;
  bool startAndDestSameFlag = false;
  bool startAndDestEmptyFlag = false;
  bool startOrDestEmptyFlag = false;
  bool invalidDateFlag = false;

};

} // namespace lb
} // namespace fs
} // namespace atools

#endif // LOGBOOKENTRYFILTER_H
