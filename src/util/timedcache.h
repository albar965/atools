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

#ifndef ATOOLS_UTIL_TIMEDCACHE_H
#define ATOOLS_UTIL_TIMEDCACHE_H

#include <QDateTime>

namespace atools {
namespace util {

/* Simple hash that removes entries on timeout when they are accessed */
template<typename KEY, typename TYPE>
class TimedCache
{
public:
  TimedCache(int timeoutSeconds)
    : timeout(timeoutSeconds)
  {
  }

  /* Add an entry and assign a timestamp to it **/
  void insert(const KEY& key, const TYPE& type);

  /* Check if entry exists. If timed out entry will be removed and method will return null */
  bool contains(const KEY& key)
  {
    return checkTimeout(key) != nullptr;
  }

  /* Get entry. If timed out entry will be removed and method will return null */
  TYPE *value(const KEY& key)
  {
    return checkTimeout(key);
  }

  void clear()
  {
    hash.clear();
  }

  /* true if object is old. does not modify cache */
  bool isTimedOut(const KEY& key) const;

  /* Flush from cache if old. true if timed out */
  bool timeOut(const KEY& key);

  /* true if in cache. Timmout is not triggered */
  bool containsNoTimeout(const KEY& key) const
  {
    return hash.contains(key);
  }

  /* Get from cache. Timeout is not triggered */
  TYPE *valueNoTimeout(const KEY& key);

  /* Return a copy and then timeout value */
  TYPE valueCopyAndTimeout(const KEY& key);

  void remove(const KEY& key);

  int size() const
  {
    return hash.size();
  }

private:
  struct Entry
  {
    TYPE value;
    QDateTime timestamp;
  };

  TYPE *checkTimeout(const KEY& key);

  QHash<KEY, Entry> hash;
  int timeout;
};

template<typename KEY, typename TYPE>
void TimedCache<KEY, TYPE>::insert(const KEY& key, const TYPE& type)
{
  hash.insert(key, {type, QDateTime::currentDateTime()});
}

template<typename KEY, typename TYPE>
TYPE *TimedCache<KEY, TYPE>::checkTimeout(const KEY& key)
{
  if(!hash.contains(key))
    return nullptr;
  else
  {
    if(isTimedOut(key))
    {
      hash.remove(key);
      return nullptr;
    }
    else
      return &hash[key].value;
  }
}

template<typename KEY, typename TYPE>
bool TimedCache<KEY, TYPE>::isTimedOut(const KEY& key) const
{
  return hash.value(key).timestamp.addSecs(timeout) < QDateTime::currentDateTime();
}

template<typename KEY, typename TYPE>
bool TimedCache<KEY, TYPE>::timeOut(const KEY& key)
{
  return checkTimeout(key) == nullptr;
}

template<typename KEY, typename TYPE>
TYPE *TimedCache<KEY, TYPE>::valueNoTimeout(const KEY& key)
{
  if(containsNoTimeout(key))
    return &hash[key].value;
  else
    return nullptr;
}

template<typename KEY, typename TYPE>
TYPE TimedCache<KEY, TYPE>::valueCopyAndTimeout(const KEY& key)
{
  TYPE type;
  TYPE *ptr = valueNoTimeout(key);
  if(ptr != nullptr)
    type = *ptr;
  timeOut(key);
  return type;
}

template<typename KEY, typename TYPE>
void TimedCache<KEY, TYPE>::remove(const KEY& key)
{
  hash.remove(key);
}

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_TIMEDCACHE_H
