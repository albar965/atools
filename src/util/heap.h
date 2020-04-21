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
template<typename TYPE>
class Heap
{
public:
  Heap(int reserve)
  {
    heap.reserve(reserve);
  }

  /* Take an element from the top of the heap. This will be the one with the lowest cost assigned */
  float pop(TYPE& data);
  void pop(TYPE& data, float& cost);

  /* Add element to the heap. The heap will be sorted/rearranged accordingly. */
  void push(const TYPE& node, float cost);

  bool contains(const TYPE& data);

  /* Update the costs of an element. The heap will be updated.  */
  void change(const TYPE& data, float cost);

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

    HeapNode(const TYPE& heapData, float heapCost)
      : data(heapData), cost(heapCost)
    {

    }

    TYPE data;
    float cost;

    /* Only data is compared */
    bool operator==(const HeapNode& other) const
    {
      return this->data == other.data;
    }

    bool operator!=(const HeapNode& other) const
    {
      return this->data != other.data;
    }

  };

  bool compare(const typename atools::util::Heap<TYPE>::HeapNode& n1,
               const typename atools::util::Heap<TYPE>::HeapNode& n2)
  {
    return n1.cost > n2.cost;
  }

  std::function<bool(const HeapNode& n1, const HeapNode& n2)> compareFunc =
    std::bind(&Heap::compare, this, std::placeholders::_1, std::placeholders::_2);

  std::vector<HeapNode> heap;
};

template<typename TYPE>
float Heap<TYPE>::pop(TYPE& data)
{
  std::pop_heap(heap.begin(), heap.end(), compareFunc);
  HeapNode curNode = heap.back();
  heap.pop_back();

  data = curNode.data;
  return curNode.cost;
}

template<typename TYPE>
void Heap<TYPE>::pop(TYPE& data, float& cost)
{
  cost = pop(data);
}

template<typename TYPE>
void Heap<TYPE>::push(const TYPE& node, float cost)
{
  heap.push_back({node, cost});
  std::push_heap(heap.begin(), heap.end(), compareFunc);
}

template<typename TYPE>
bool Heap<TYPE>::contains(const TYPE& data)
{
  return std::find(heap.begin(), heap.end(), HeapNode(data)) != heap.end();
}

template<typename TYPE>
void Heap<TYPE>::change(const TYPE& data, float cost)
{
  typename std::vector<HeapNode>::iterator it =
    std::find(heap.begin(), heap.end(), HeapNode(data));

  if(it != heap.end())
    it->cost = cost;
  std::make_heap(heap.begin(), heap.end(), compareFunc);
}

} // namespace util
} // namespace atools

#endif // ATOOLS_UTIL_HEAP_H
