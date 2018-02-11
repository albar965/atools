/*****************************************************************************
* Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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
    if(hash.value(key).timestamp.addSecs(timeout) < QDateTime::currentDateTime())
    {
      hash.remove(key);
      return nullptr;
    }
    else
      return &hash[key].value;
  }
}

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_TIMEDCACHE_H
