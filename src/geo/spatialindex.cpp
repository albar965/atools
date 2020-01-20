/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

  // Must return the number of data points
  size_t kdtree_get_point_count() const
  {
    return static_cast<size_t>(points.size());
  }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate value, the
  // "if/else's" are actually solved at compile time.
  float kdtree_get_pt(const size_t idx, const size_t dim) const
  {
    if(dim == 0)
      return points.at(static_cast<int>(idx)).getX();
    else if(dim == 1)
      return points.at(static_cast<int>(idx)).getY();
    else
      return points.at(static_cast<int>(idx)).getZ();
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

  QVector<Point3D> points; // Must be initialized before the index
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

/* Callback for radius searches. Does min and max distance comparison. */
class RadiusResults
{
public:
  RadiusResults(QVector<std::pair<int, float> >& resultParam, const QVector<Point3D>& pointsParam,
                float radiusMinMeterParam, float radiusMaxMeterParam, const Point3D& originPointParam,
                const Point3D& destPointParam)
    : radiusMinMeter(radiusMinMeterParam), radiusMaxMeter(radiusMaxMeterParam), result(resultParam),
    points(pointsParam), destPoint(destPointParam), originPoint(originPointParam)
  {
    // Calculate distance from origin to destination
    if(destPoint.isValid() && originPoint.isValid())
      destDistMeter = destPoint.gcDistanceMeter(originPoint);
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
    if(dist < radiusMaxMeter)
    {
      bool ok = true;
      if(destDistMeter > 0.f)
        ok &= points.at(index).directDistanceMeter(destPoint) < destDistMeter;

      if(radiusMinMeter > 0.f)
        ok &= points.at(index).gcDistanceMeter(originPoint) > radiusMinMeter;

      if(ok)
        result.push_back(std::make_pair(index, dist));
    }

    // keep adding points
    return true;
  }

  float worstDist() const
  {
    return radiusMaxMeter;
  }

private:
  const float radiusMinMeter, radiusMaxMeter;
  float destDistMeter = 0.f;

  QVector<std::pair<int, float> >& result;
  const QVector<Point3D>& points;
  Point3D destPoint, originPoint;
};

/* Operator for distance sorting of a pair (index/distance) vector. */
struct IndexDistSorter
{
  /** PairType will be typically: std::pair<IndexType,DistanceType> */
  template<typename PairType>
  inline bool operator()(const PairType& p1, const PairType& p2) const
  {
    return p1.second < p2.second;
  }

};

void SpatialIndexPrivate::pointsInRadius(QVector<int>& indexes, const Pos& pos, float radiusMinMeter,
                                         float radiusMaxMeter, bool sort, const Pos *destPos) const
{
  Point3D destPt;
  if(destPos != nullptr)
    destPos->toCartesian(destPt);

  float originPtArr[3];
  pos.toCartesian(originPtArr[0], originPtArr[1], originPtArr[2]);
  Point3D originPt(originPtArr[0], originPtArr[1], originPtArr[2]);

  QVector<std::pair<int, float> > indicesDists;
  indicesDists.reserve(100000);
  RadiusResults resultCallback(indicesDists, p->points, radiusMinMeter, radiusMaxMeter, originPt, destPt);
  nanoflann::SearchParams params;
  params.sorted = false;

  int num = static_cast<int>(p->index.radiusSearchCustomCallback(originPtArr, resultCallback, params));

  if(sort)
  {
    // for(std::pair<int, float>& indexDist : indicesDists)
    // indexDist.second = p->points.at(indexDist.first).gcDistanceMeter(originPt);

    std::sort(indicesDists.begin(), indicesDists.end(), IndexDistSorter());
  }

  for(int i = 0; i < num; i++)
    indexes.append(static_cast<int>(indicesDists.at(i).first));
}

void SpatialIndexPrivate::buildIndex()
{
  p->index.buildIndex();
}

void SpatialIndexPrivate::append(const Point3D& point)
{
  p->points.append(point);
}

void SpatialIndexPrivate::clear()
{
  p->points.clear();
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
