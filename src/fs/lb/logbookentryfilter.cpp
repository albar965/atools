#include "fs/lb/logbookentryfilter.h"
#include "fs/lb/logbookentry.h"

namespace atools {
namespace fs {
namespace lb {

LogbookEntryFilter& LogbookEntryFilter::flightTimeLowerThan(int minutes)
{
  minFlightTime = minutes;
  return *this;
}

LogbookEntryFilter& LogbookEntryFilter::startAndDestSame(bool flag)
{
  startAndDestSameFlag = flag;
  return *this;
}

LogbookEntryFilter& LogbookEntryFilter::startAndDestEmpty(bool flag)
{
  startAndDestEmptyFlag = flag;
  return *this;
}

LogbookEntryFilter& LogbookEntryFilter::startOrDestEmpty(bool flag)
{
  startOrDestEmptyFlag = flag;
  return *this;
}

LogbookEntryFilter& LogbookEntryFilter::invalidDate(bool flag)
{
  invalidDateFlag = flag;
  return *this;
}

bool LogbookEntryFilter::canStore(const LogbookEntry& entry) const
{
  if(entry.getTotalTimeMin() * 60.f < minFlightTime)
    return false;

  if(startAndDestSameFlag && (entry.getAirportTo() == entry.getAirportFrom()))
    return false;

  if(startAndDestEmptyFlag && entry.getAirportFrom().isEmpty() && entry.getAirportTo().isEmpty())
    return false;

  if(startOrDestEmptyFlag && (entry.getAirportFrom().isEmpty() || entry.getAirportTo().isEmpty()))
    return false;

  if(invalidDateFlag && !entry.getDateTime().isValid())
    return false;

  return true;
}

} // namespace lb
} // namespace fs
} // namespace atools
