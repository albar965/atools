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

#ifndef ATOOLS_SIMPLESPATIALINDEX_H
#define ATOOLS_SIMPLESPATIALINDEX_H

#include "geo/pos.h"

#include <QCache>
#include <QVector>
#include <functional>

namespace atools {
namespace geo {

/*
 * Simple and inefficient spatial index using full search.
 * Do not use for performance critical applications or large datasets.
 */
template<typename KEY, typename TYPE>
class SimpleSpatialIndex
{
public:
  SimpleSpatialIndex(int cacheSize = 1000)
    : cache(cacheSize)
  {
  }

  ~SimpleSpatialIndex()
  {
  }

  /* Add entry, key and position to index */
  void insert(const KEY& key, const TYPE& type, const atools::geo::Pos& pos);
  void insert(const KEY& key, const atools::geo::Pos& pos);

  /* Returned KEY will differ if only nearest was found.
   * Will be equal to passed key is exact was found.
   * Key is empty if nothing was found. */
  KEY getTypeOrNearest(TYPE& type, const KEY& key, const atools::geo::Pos& pos);
  KEY getTypeOrNearest(const KEY& key, const atools::geo::Pos& pos);

  bool contains(const KEY& key) const
  {
    return index.contains(key);
  }

  TYPE value(const KEY& key) const;
  bool value(TYPE& result, const KEY& key) const;

  void clear();

  QList<KEY> keys() const
  {
    return index.keys();
  }

  bool isEmpty() const
  {
    return index.isEmpty();
  }

  int size() const
  {
    return index.size();
  }

private:
  /* Simple index to allow a full search for nearest */
  struct Entry
  {
    KEY key;
    TYPE type;
    atools::geo::Pos pos;
  };

  QHash<KEY, Entry> index;

  /* Maps keys to nearest entries */
  QCache<KEY, Entry> cache;

};

template<typename KEY, typename TYPE>
void SimpleSpatialIndex<KEY, TYPE>::insert(const KEY& key, const TYPE& type, const geo::Pos& pos)
{
  index.insert(key, {key, type, pos});
}

template<typename KEY, typename TYPE>
void SimpleSpatialIndex<KEY, TYPE>::insert(const KEY& key, const Pos& pos)
{
  index.insert(key, {key, TYPE(), pos});
}

template<typename KEY, typename TYPE>
KEY SimpleSpatialIndex<KEY, TYPE>::getTypeOrNearest(TYPE& type, const KEY& key,
                                                    const atools::geo::Pos& pos)
{
  if(!isEmpty())
  {
    if(index.contains(key))
    {
      // Found exact match on key
      type = index.value(key).type;
      return key;
    }
    else
    {
      // Check cache to avoid costly calculations
      Entry *nearest = cache.object(key);
      if(nearest == nullptr)
      {
        // Create new entry for cache
        nearest = new Entry;
        float nearestDistance = std::numeric_limits<float>::max();

        for(const Entry& entry : index.values())
        {
          float dist = entry.pos.distanceMeterTo(pos);
          if(dist < nearestDistance)
          {
            nearestDistance = dist;
            *nearest = entry;
          }
        }

        if(nearestDistance < std::numeric_limits<float>::max())
          // Found an entry
          cache.insert(key, nearest);
        else
        {
          delete nearest;
          nearest = nullptr;
        }
      }

      if(nearest != nullptr)
      {
        type = nearest->type;
        return nearest->key;
      }
    }
  }
  return KEY();
}

template<typename KEY, typename TYPE>
KEY SimpleSpatialIndex<KEY, TYPE>::getTypeOrNearest(const KEY& key, const Pos& pos)
{
  TYPE dummy;
  return getTypeOrNearest(dummy, key, pos);
}

template<typename KEY, typename TYPE>
TYPE SimpleSpatialIndex<KEY, TYPE>::value(const KEY& key) const
{
  TYPE type;
  value(type, key);
  return type;
}

template<typename KEY, typename TYPE>
bool SimpleSpatialIndex<KEY, TYPE>::value(TYPE& result, const KEY& key) const
{
  if(index.contains(key))
  {
    result = index.value(key).type;
    return true;
  }
  else
    return false;
}

template<typename KEY, typename TYPE>
void SimpleSpatialIndex<KEY, TYPE>::clear()
{
  index.clear();
  cache.clear();
}

} // namespace geo
} // namespace atools

#endif // ATOOLS_SIMPLESPATIALINDEX_H
