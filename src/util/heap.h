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

#ifndef ATOOLS_UTIL_HEAP_H
#define ATOOLS_UTIL_HEAP_H

#include <QVector>
#include <algorithm>
#include <functional>
#include <queue>

namespace atools {
namespace util {

/*
 * Heap data structure.
 */
template<typename TYPE, typename COST>
class Heap
{
public:
  Heap(int reserve)
  {
    heap.reserve(reserve);
  }

  /* Take an element from the top of the heap. This will be the one with the lowest cost assigned */
  COST pop(TYPE& data);

  /* Return data directly. Use this if TYPE is an integral type like int. */
  TYPE popData();

  void pop(TYPE& data, COST& cost)
  {
    cost = pop(data);
  }

  /* Add element to the heap. The heap will be sorted/rearranged accordingly. */
  void push(const TYPE& type, COST cost)
  {
    heap.push_back({type, cost});
    std::push_heap(heap.begin(), heap.end());
  }

  /* Push data directly. Use this if TYPE is an integral type like int. */
  void pushData(TYPE data, COST cost)
  {
    heap.push_back({data, cost});
    std::push_heap(heap.begin(), heap.end());
  }

  bool contains(const TYPE& data)
  {
    return std::find(heap.begin(), heap.end(), HeapNode(data)) != heap.end();
  }

  /* Update the costs of an element. The heap will be updated.  */
  void change(const TYPE& data, COST cost);
  void changeOrPush(const TYPE& data, COST cost);

  bool isEmpty() const
  {
    return heap.empty();
  }

  int size() const
  {
    return heap.size();
  }

private:
  struct HeapNode
  {
    HeapNode(const TYPE& heapData)
      : data(heapData), cost(0.f)
    {

    }

    HeapNode(const TYPE& heapData, COST heapCost)
      : data(heapData), cost(heapCost)
    {

    }

    TYPE data;
    COST cost;

    /* Only data is compared */
    bool operator==(const HeapNode& other) const
    {
      return this->data == other.data;
    }

    bool operator!=(const HeapNode& other) const
    {
      return this->data != other.data;
    }

    bool operator<(const atools::util::Heap<TYPE, COST>::HeapNode& other) const
    {
      return cost > other.cost;
    }

  };

  std::vector<HeapNode> heap;
};

template<typename TYPE, typename COST>
COST Heap<TYPE, COST>::pop(TYPE& data)
{
  std::pop_heap(heap.begin(), heap.end());
  HeapNode curNode = heap.back();
  heap.pop_back();

  data = curNode.data;
  return curNode.cost;
}

template<typename TYPE, typename COST>
TYPE Heap<TYPE, COST>::popData()
{
  std::pop_heap(heap.begin(), heap.end());
  HeapNode curNode = heap.back();
  heap.pop_back();
  return curNode.data;
}

template<typename TYPE, typename COST>
void Heap<TYPE, COST>::change(const TYPE& data, COST cost)
{
  typename std::vector<HeapNode>::iterator it = std::find(heap.begin(), heap.end(), HeapNode(data));

  if(it != heap.end())
    it->cost = cost;
  std::make_heap(heap.begin(), heap.end());
}

template<typename TYPE, typename COST>
void Heap<TYPE, COST>::changeOrPush(const TYPE& data, COST cost)
{
  typename std::vector<HeapNode>::iterator it = std::find(heap.begin(), heap.end(), HeapNode(data));

  if(it != heap.end())
  {
    it->cost = cost;
    std::make_heap(heap.begin(), heap.end());
  }
  else
  {
    heap.push_back({data, cost});
    std::push_heap(heap.begin(), heap.end());
  }
}

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_HEAP_H
