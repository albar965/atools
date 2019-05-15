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

#ifndef ATOOLS_GRIBCOMMON_H
#define ATOOLS_GRIBCOMMON_H

#include <QDateTime>
#include <QVector>

namespace atools {
namespace grib {

class GribReader;

/* Momentum parameters. Here wind speed.
 * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2-0-2.shtml
 * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2.shtml */
enum ParameterType
{
  U_WIND = 2, /* U component of wind; eastward wind */
  V_WIND = 3 /* V component of wind; northward wind */
};

/*
 * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-5.shtml
 */
enum SurfaceType
{
  MBAR = 100, /* surface is isobar in mbar or hpa */
  METER_AGL = 103 /* surface is meter above ground level */
};

/*
 * Contains a decoded GRIB2 dataset based on U and V wind vectors.
 */
class GribDataset
{
public:
  /* Surface either in mBar (positive value) or meter AGL (negative value)*/
  float getSurface() const
  {
    return surface;
  }

  /* Only U or V wind components */
  atools::grib::ParameterType getParameterType() const
  {
    return parameterType;
  }

  /* Date from metadata */
  const QDateTime& getDatetime() const
  {
    return datetime;
  }

  /* Wind vectors in meters per second organized in 360 columns (0-359) and 181 rows (0-180)
   * Index 0,0 contains information for 90° North and 0° E/W
   *
   *  Ni — number of points along a parallel - 360
   *  Nj — number of points along a meridian - 181
   *  multiplying Ni (octets 31-34) by Nj (octets 35-38) yields the total number of points
   *  i direction - west to east along a parallel or left to right along an x-axis.
   *  j direction - south to north along a meridian, or bottom to top along a y-axis.
   */
  const QVector<float>& getData() const
  {
    return data;
  }

  /* Altitude calculated from surface based on ISA atmosphere */
  float getAltFeetCalculated() const
  {
    return altFeetCalculated;
  }

  /* Altitude rounded to next 2000 feet */
  float getAltFeetRounded() const
  {
    return altFeetRounded;
  }

  /* Only mbar or meter surfaces */
  atools::grib::SurfaceType getSurfaceType() const
  {
    return surfaceType;
  }

private:
  friend class atools::grib::GribReader;

  float surface;
  float altFeetCalculated;
  float altFeetRounded;

  atools::grib::ParameterType parameterType;
  atools::grib::SurfaceType surfaceType;

  QDateTime datetime;
  QVector<float> data;
};

typedef  QVector<GribDataset> GribDatasetVector;

} // namespace grib
} // namespace atools

#endif // ATOOLS_GRIBCOMMON_H
