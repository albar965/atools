/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
#include <functional>

namespace atools {
namespace geo {

class Pos;
template<typename T>
class SpatialIndex;

/* A callback that can be used as a secondary filter stage for the radius search
 * after filtering by manhattan distance to origin. */
typedef std::function<bool (float, int)> RadiusCallbackType;

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
  void pointsInRadius(QVector<int>& indexes, const atools::geo::Pos& origin, float radiusMaxMeter,
                      const RadiusCallbackType& callback) const;
  void set(const Point3D& point, int index);
  void buildIndex();
  void clear();
  void reserve(int size);
  const Point3D *points3D();

  /* Data source containing nanoflann structures. */
  DataSource *p = nullptr;

};

} // namespace internal
/* End of private parts *************************************************************************************/

/*
 * Spatial index wrapping the nanoflann library which uses KD-tree for nearest neighbor search.
 *
 * Changing the underlying vector needs a call of updateIndex() afterwards.
 *
 * Note that squared distance is used internally for lookup and resulting distances are therefore not accurate.
 *
 * T needs a method const atools::geo::Pos& getPosition() const .
 */
template<typename T>
class SpatialIndex :
  public QVector<T>
{
public:
  SpatialIndex()
  {
    p = new atools::geo::internal::SpatialIndexPrivate;
  }

  ~SpatialIndex()
  {
    delete p;
  }

  SpatialIndex(const SpatialIndex& other) = delete;
  SpatialIndex& operator=(const SpatialIndex& other) = delete;

  /* Get one nearest object from the vector. */
  const T& getNearest(const atools::geo::Pos& pos) const;
  void getNearest(T& obj, const atools::geo::Pos& pos) const;

  /* Get index of one nearest object. Index can be used to access objects from the underlying vector or
   * the Point3D vector using getPoints3D().*/
  int getNearestIndex(const atools::geo::Pos& pos) const
  {
    return p->nearestPoint(pos);
  }

  /* Get number nearest objects or indexes from the vector. */
  void getNearest(QVector<T>& objects, const atools::geo::Pos& pos, int number) const;

  void getNearestIndexes(QVector<int>& indexes, const atools::geo::Pos& pos, int number) const
  {
    p->nearestPoints(indexes, pos, number);
  }

  /* Get all nearest objects or indexes from the vector fulfilling criteria.
   *  radiusMeter: Maximum distance from position. Measured using squared distance and therefore not accurate.
   */
  void getRadius(QVector<T>& objects, const atools::geo::Pos& pos, float radiusMeter, const RadiusCallbackType& callback) const;

  void getRadiusIndexes(QVector<int>& indexes, const atools::geo::Pos& pos, float radiusMaxMeter, const RadiusCallbackType& callback) const
  {
    p->pointsInRadius(indexes, pos, radiusMaxMeter, callback);
  }

  void getRadius(QVector<T>& objects, const atools::geo::Pos& pos, float radiusMeter) const;

  void getRadiusIndexes(QVector<int>& indexes, const atools::geo::Pos& pos, float radiusMaxMeter) const
  {
    p->pointsInRadius(indexes, pos, radiusMaxMeter, RadiusCallbackType());
  }

  /* Rebuild the KD-tree and Point3D vector. Call this after changing the base class vector. */
  void updateIndex();

  /* Get points converted to 3D euclidian space from base vector.
   * Size is the same as in the underlying parent QVector. */
  const Point3D *getPoints3D() const
  {
    return p->points3D();
  }

  const Point3D& atPoint3D(int index) const
  {
    return p->points3D()[index];
  }

  /* Clears vector and updates index to clear it */
  void clearIndex()
  {
    QVector<T>::clear();
    updateIndex();
  }

private:
  using QVector<T>::clear;

  /* Copy objects from base vector to result set. */
  void copyData(QVector<T>& objects, QVector<int>& indexes) const
  {
    for(int idx : indexes)
      objects.append(this->at(idx));
  }

  atools::geo::internal::SpatialIndexPrivate *p = nullptr;
};

/* Methods *************************************************************************************/

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
void SpatialIndex<T>::getRadius(QVector<T>& objects, const Pos& pos, float radiusMaxMeter, const RadiusCallbackType& callback) const
{
  QVector<int> indexes;
  p->pointsInRadius(indexes, pos, radiusMaxMeter, callback);
  copyData(objects, indexes);
}

template<typename T>
void SpatialIndex<T>::getRadius(QVector<T>& objects, const Pos& pos, float radiusMaxMeter) const
{
  QVector<int> indexes;
  p->pointsInRadius(indexes, pos, radiusMaxMeter, RadiusCallbackType());
  copyData(objects, indexes);
}

template<typename T>
void SpatialIndex<T>::updateIndex()
{
  p->reserve(QVector<T>::size());

  for(int i = 0; i < QVector<T>::size(); i++)
    p->set(QVector<T>::at(i).getPosition().toCartesian(), i);

  p->buildIndex();
}

} // namespace geo
} // namespace atools

#endif // ATOOLS_GEO_SPATIALINDEX_H
