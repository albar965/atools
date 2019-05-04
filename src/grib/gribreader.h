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

#ifndef ATOOLS_GRIBREADER_H
#define ATOOLS_GRIBREADER_H

#include "grib/gribcommon.h"

#include <QVector>

namespace atools {
namespace grib {

/*
 * Reads and decodes a GRIB2 data file into a GribDatasetVector.
 * Only U/V wind, full earth bounding rectangle and one-degree raster supported.
 * Throws atools::Exception if parameters are not correct.
 *
 * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/
 * https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml
 */
class GribReader
{
public:
  GribReader(bool verboseParam = false);

  /* Reads a GRIB dataset from a file or byte array.
   * Throws atools::Exception if parameters are not correct. */
  void readFile(const QString& filename);
  void readData(const QByteArray& data);

  /* Clear dataset for reuse */
  void clear();

  /* Get decoded datasets for read file */
  const atools::grib::GribDatasetVector& getDatasets() const
  {
    return datasets;
  }

private:
  atools::grib::GribDatasetVector datasets;
  bool verbose = false;
};

} // namespace grib
} // namespace atools

#endif // ATOOLS_GRIBREADER_H
