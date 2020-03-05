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

#include "geo/spatialindex.h"
#include "geo/nanoflann.h"
#include "geo/pos.h"
#include "geo/calculations.h"

using namespace std;
using namespace nanoflann;
using atools::geo::Pos;

namespace atools {
namespace geo {
namespace internal {

/* Private wrapper to keep nanoflann structures out of the header */
struct DataSource
{
  DataSource()
    : index(DIMENSIONS, *this, KDTreeSingleIndexAdaptorParams(MAX_LEAF_SIZE))
  {
  }

  ~DataSource()
  {
    free();
  }

  void init(int size)
  {
    free();
    points = new Point3D[static_cast<size_t>(size)];
    pointsSize = size;
  }

  void free()
  {
    delete[] points;
    points = nullptr;
    pointsSize = 0;
  }

  // Must return the number of data points
  size_t kdtree_get_point_count() const
  {
    return static_cast<size_t>(pointsSize);
  }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate value, the
  // "if/else's" are actually solved at compile time.
  float kdtree_get_pt(const size_t idx, const size_t dim) const
  {
    if(dim == 0)
      return points[idx].getX();
    else if(dim == 1)
      return points[idx].getY();
    else
      return points[idx].getZ();
  }

  // Optional bounding-box computation: return false to default to a standard bbox computation loop.
  // Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
  // Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
  template<class BBOX>
  bool kdtree_get_bbox(BBOX& /* bb */) const
  {
    return false;
  }

  constexpr static int DIMENSIONS = 3;
  constexpr static int MAX_LEAF_SIZE = 20;

  int pointsSize = 0;
  Point3D *points = nullptr; // Must be initialized before the index
  KDTreeSingleIndexAdaptor<L1_Adaptor<float, DataSource>, DataSource, DIMENSIONS, int> index;
};

/* Methods *************************************************************************************/

int SpatialIndexPrivate::nearestPoint(const Pos& pos) const
{
  float pt[3];
  pos.toCartesian(pt[0], pt[1], pt[2]);

  int resultIndex;
  float resultSqDist;
  size_t num = p->index.knnSearch(pt, 1, &resultIndex, &resultSqDist);

  return num == 1 ? static_cast<int>(resultIndex) : -1;
}

void SpatialIndexPrivate::nearestPoints(QVector<int>& indexes, const Pos& pos, int number) const
{
  float pt[3];
  pos.toCartesian(pt[0], pt[1], pt[2]);

  QVector<float> resultSqDist(number);
  indexes.clear();
  indexes.fill(0.f, number);
  size_t numFound = p->index.knnSearch(pt, static_cast<size_t>(number), indexes.data(), resultSqDist.data());
  indexes.resize(static_cast<int>(numFound));
}

struct IndexEntry
{
  int index;
  float distance;
};

/* Callback for radius searches. Does min and max distance comparison. All distances in meter. */
class RadiusResults
{
public:
  RadiusResults(QVector<IndexEntry>& resultParam, float radiusMaxParam, const RadiusCallbackType& radiusCallback)
    : radiusMax(radiusMaxParam), result(resultParam), callback(radiusCallback)
  {
  }

  size_t size() const
  {
    return static_cast<size_t>(result.size());
  }

  bool full() const
  {
    return true;
  }

  /**
   * Called during search to add an element matching the criteria.
   * @return true if the search should be continued, false if the results are
   * sufficient
   */
  bool addPoint(float dist, int index)
  {
    if(dist < radiusMax && (!callback || callback(dist, index)))
      result.push_back({index, dist});

    // keep adding points
    return true;
  }

  float worstDist() const
  {
    return radiusMax;
  }

private:
  float radiusMax;
  QVector<IndexEntry>& result;
  RadiusCallbackType callback;
};

void SpatialIndexPrivate::pointsInRadius(QVector<int>& indexes, const Pos& origin, float radiusMaxMeter,
                                         const RadiusCallbackType& callback) const
{
  float originPtArr[3];
  origin.toCartesian(originPtArr[0], originPtArr[1], originPtArr[2]);
  Point3D originPt(originPtArr[0], originPtArr[1], originPtArr[2]);

  QVector<IndexEntry> indicesDists;
  indicesDists.reserve(100000);
  RadiusResults resultCallback(indicesDists, radiusMaxMeter, callback);

  nanoflann::SearchParams params;
  params.sorted = false;

  int num = static_cast<int>(p->index.radiusSearchCustomCallback(originPtArr, resultCallback, params));

  for(int i = 0; i < num; i++)
    indexes.append(indicesDists.at(i).index);
}

void SpatialIndexPrivate::buildIndex()
{
  p->index.buildIndex();
}

void SpatialIndexPrivate::set(const Point3D& point, int index)
{
  p->points[index] = point;
}

void SpatialIndexPrivate::clear()
{
  p->free();
}

void SpatialIndexPrivate::reserve(int size)
{
  p->init(size);
}

const atools::geo::Point3D *SpatialIndexPrivate::points3D()
{
  return p->points;
}

SpatialIndexPrivate::SpatialIndexPrivate()
{
  p = new DataSource;
}

SpatialIndexPrivate::~SpatialIndexPrivate()
{
  delete p;
}

}

} // namespace geo
} // namespace atools

Q_DECLARE_TYPEINFO(atools::geo::internal::IndexEntry, Q_PRIMITIVE_TYPE);
