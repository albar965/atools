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

#ifndef ATOOLS_GEO_SPATIALINDEX_H
#define ATOOLS_GEO_SPATIALINDEX_H

#include "geo/point3d.h"

#include <QVector>
#include <cmath>

namespace atools {
namespace geo {

class Pos;

namespace internal {
class SpatialIndexPrivate;
}

/*
 * Spatial index wrapping the nanoflann librar< which uses KD-tree for nearest neighbor search.
 *
 * Changing the underlying vector needs a call of buildIndex() afterwards.
 *
 * Note that squared distance is used internally for lookup and resulting distances are therefore not accurate.
 */
template<typename T>
class SpatialIndex :
  public QVector<T>
{
public:
  SpatialIndex();
  ~SpatialIndex();

  /* Get one nearest object from the vector. */
  const T& getNearest(const atools::geo::Pos& pos) const;
  void getNearest(T& obj, const atools::geo::Pos& pos) const;

  /* Get index of one nearest object. Index can be used to access objects from the underlying vector or
   * the Point3D vector using getPoints3D().*/
  int getNearestIndex(const atools::geo::Pos& pos) const;

  /* Get number nearest objects or indexes from the vector. */
  void getNearest(QVector<T>& objects, const atools::geo::Pos& pos, int number) const;
  void getNearestIndexes(QVector<int>& indexes, const atools::geo::Pos& pos, int number) const;

  /* Get all nearest objects or indexes from the vector fulfilling criteria.
   *  radiusMinMeter: Minimum distance to position. Measured accurately.
   *  radiusMeter: Maximum distance from position. Measured using squared distance and therefore not accurate.
   *  sort: Sort the result by distance to point.
   *  destPos: Use distance to a destination point to filter out all points farther away than pos if not null.
   *  Can be used to achieve a fast directional filtering of results.
   */
  void getRadius(QVector<T>& objects, const atools::geo::Pos& pos, float radiusMinMeter, float radiusMeter,
                 float directDistanceFactor,
                 bool sort = false, const atools::geo::Pos *destPos = nullptr,
                 const QSet<int> *excludeIndexes = nullptr) const;
  void getRadiusIndexes(QVector<int>& indexes, const atools::geo::Pos& pos, float radiusMinMeter, float radiusMeter,
                        float directDistanceFactor,
                        bool sort = true, const atools::geo::Pos *destPos = nullptr,
                        const QSet<int> *excludeIndexes = nullptr) const;

  /* Rebuild the KD-tree and Point3D vector. Call this after changing the base class vector. */
  void updateIndex();

  /* Get points converted to 3D euclidian space from base vector. */
  const QVector<Point3D>& getPoints3D() const;
  const Point3D& atPoint3D(int index) const;

private:
  /* Copy objects from base vector to result set. */
  void copyData(QVector<T>& objects, QVector<int>& indexes) const
  {
    for(int idx : indexes)
      objects.append(this->at(idx));
  }

  atools::geo::internal::SpatialIndexPrivate *p = nullptr;
};

/* Private parts *************************************************************************************/

namespace internal {

struct DataSource;

/* Wraps nanoflann structures and detaches functionality from template class. */
class SpatialIndexPrivate
{
  template<typename T>
  friend class atools::geo::SpatialIndex;

  SpatialIndexPrivate();
  ~SpatialIndexPrivate();

  int nearestPoint(const atools::geo::Pos& pos) const;
  void nearestPoints(QVector<int>& indexes, const atools::geo::Pos& pos, int number) const;
  void pointsInRadius(QVector<int>& indexes, const atools::geo::Pos& pos, float radiusMinMeter, float radiusMaxMeter,
                      float directDistanceFactor,
                      bool sort, const Pos *destPos, const QSet<int> *excludeIndexes) const;
  void buildIndex();
  void append(const Point3D& point);
  void clear();
  void reserve(int size);
  QVector<Point3D>& points3D();

  /* Data source containing nanoflann structures. */
  DataSource *p = nullptr;
};

} // namespace internal

/* Methods *************************************************************************************/

template<typename T>
SpatialIndex<T>::SpatialIndex()
{
  p = new atools::geo::internal::SpatialIndexPrivate;
}

template<typename T>
SpatialIndex<T>::~SpatialIndex()
{
  delete p;
}

template<typename T>
const T& SpatialIndex<T>::getNearest(const Pos& pos) const
{
  static const T EMPTY;
  int idx = p->nearestPoint(pos);
  return idx >= 0 ? this->at(idx) : EMPTY;
}

template<typename T>
void SpatialIndex<T>::getNearest(T& obj, const Pos& pos) const
{
  static const T EMPTY;
  int idx = p->nearestPoint(pos);
  obj = idx >= 0 ? this->at(idx) : EMPTY;
}

template<typename T>
void SpatialIndex<T>::getNearest(QVector<T>& objects, const Pos& pos, int number) const
{
  QVector<int> indexes;
  p->nearestPoints(indexes, pos, number);
  copyData(objects, indexes);
}

template<typename T>
void SpatialIndex<T>::getRadius(QVector<T>& objects, const Pos& pos, float radiusMinMeter, float radiusMaxMeter,
                                float directDistanceFactor, bool sort, const atools::geo::Pos *destPos,
                                const QSet<int> *excludeIndexes) const
{
  QVector<int> indexes;
  p->pointsInRadius(indexes, pos, radiusMinMeter, radiusMaxMeter, directDistanceFactor, sort, destPos, excludeIndexes);
  copyData(objects, indexes);
}

template<typename T>
int SpatialIndex<T>::getNearestIndex(const Pos& pos) const
{
  return p->nearestPoint(pos);
}

template<typename T>
void SpatialIndex<T>::getNearestIndexes(QVector<int>& indexes, const Pos& pos, int number) const
{
  p->nearestPoints(indexes, pos, number);
}

template<typename T>
void SpatialIndex<T>::getRadiusIndexes(QVector<int>& indexes, const Pos& pos, float radiusMinMeter,
                                       float radiusMaxMeter, float directDistanceFactor, bool sort,
                                       const geo::Pos *destPos, const QSet<int> *excludeIndexes) const
{
  p->pointsInRadius(indexes, pos, radiusMinMeter, radiusMaxMeter, directDistanceFactor, sort, destPos, excludeIndexes);
}

template<typename T>
void SpatialIndex<T>::updateIndex()
{
  QVector<T>::squeeze();
  p->clear();
  p->reserve(QVector<T>::size());

  for(const T& data : *this)
    p->append(data.getPosition().toCartesian());

  p->buildIndex();
}

template<typename T>
const QVector<Point3D>& SpatialIndex<T>::getPoints3D() const
{
  return p->points3D();
}

template<typename T>
const Point3D& SpatialIndex<T>::atPoint3D(int index) const
{
  return p->points3D().at(index);
}

} // namespace geo
} // namespace atools

#endif // ATOOLS_GEO_SPATIALINDEX_H
