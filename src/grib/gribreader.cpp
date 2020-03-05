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

#include "grib/gribreader.h"
#include "geo/calculations.h"
#include "io/tempfile.h"
#include "exception.h"

extern "C" {
#include "g2clib/grib2.h"
}

#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>

namespace atools {
namespace grib {

/* Print an int array for debug output */
void printArrInt(const QString& name, g2int *arr, g2int num)
{
  if(arr == nullptr)
    qDebug() << name << "null";
  else
  {
    QStringList list;
    for(int i = 0; i < num; i++)
      list.append(QString::number(arr[i]));
    qDebug().noquote().nospace() << name << "(" << num << ")" << "[" << list.join(", ") << "]";
  }
}

/* Print a float array for debug output */
void printArrFloat(const QString& name, g2float *arr, g2int num)
{
  if(arr == nullptr)
    qDebug() << name << "null";
  else
  {
    QStringList list;
    for(int i = 0; i < num; i++)
      list.append(QString::number(arr[i]));
    qDebug().noquote().nospace() << name << "(" << num << ")" << "[" << list.join(", ") << "]";
  }
}

/* Check value if it matches expected and return false if not */
template<typename TYPE>
Q_REQUIRED_RESULT inline bool checkValue(const QString& message, TYPE value, TYPE expected)
{
  if(expected != value)
  {
    qWarning() << QString("GribReader: Error reading grib file: %1: value %2 not equal to expected value %3").
    arg(message).arg(value).arg(expected);
    return false;
  }
  return true;
}

/* Check value if it matches one of the expected and return false if not */
template<typename TYPE>
Q_REQUIRED_RESULT bool checkValue(const QString& message, TYPE value, const QVector<TYPE>& expected)
{
  if(!expected.contains(value))
  {
    QStringList expectedStr;
    for(TYPE val : expected)
      expectedStr.append(QString::number(static_cast<int>(val)));

    qWarning() << QString("GribReader: Error reading grib file: %1: value %2 not in expected range %3").
    arg(message).arg(value).arg(expectedStr.join(GribReader::tr(", ")));
    return false;
  }
  return true;
}

// =====================================================================================
GribReader::GribReader(bool verboseParam)
  : verbose(verboseParam)
{

}

void GribReader::readFile(const QString& filename)
{
  unsigned char *cgrib;
  g2int listSection0[3], listSection1[13], numlocal, numfields;
  long skipBytes, numGribBytes, seekBytes = 0L;
  g2int expand = 1, unpack = 1, ret, ierr;

#if defined(Q_OS_WIN32)
  // Windows fopen uses local charset for filename - convert UTF-8 to UTF-16 and use wfopen
  wchar_t *path = new wchar_t[static_cast<unsigned int>(filename.size()) + 1];
  filename.toWCharArray(path);
  path[filename.size()] = L'\0';

  FILE *fptr = _wfopen(path, L"rb");
#else
  FILE *fptr = fopen(filename.toUtf8().data(), "rb");
#endif

  if(fptr != nullptr)
  {
    while(true)
    {
      if(verbose)
        qDebug() << "======================================================================";

      // Search for next/first GRIB message ========================================
      seekgb(fptr, seekBytes, 128000, &skipBytes, &numGribBytes);
      if(numGribBytes == 0)
        break;  // end loop at EOF or problem

      cgrib = new unsigned char[static_cast<size_t>(numGribBytes)];
      ret = fseek(fptr, skipBytes, SEEK_SET);
      if(ret != g2int(0))
        throw atools::Exception(tr("Cannot seek in file %1").arg(filename));

      // Read message ========================================
      size_t lengrib = fread(cgrib, sizeof(unsigned char), static_cast<size_t>(numGribBytes), fptr);
      if(lengrib <= 0)
        throw atools::Exception(tr("Cannot read file %1").arg(filename));

      seekBytes = skipBytes + numGribBytes;
      ierr = g2_info(cgrib, listSection0, listSection1, &numfields, &numlocal);
      if(ierr != g2int(0))
        throw atools::Exception(tr("Cannot read file %1").arg(filename));

      if(verbose)
        qDebug() << "numfields" << numfields << "numlocal" << numlocal;
      printArrInt(QString(Q_FUNC_INFO) + " Section 0: ", listSection0, 3);
      printArrInt(QString(Q_FUNC_INFO) + " Section 1: ", listSection1, 13);

      // Read datasets / GRIB messages ========================================
      for(long n = 0; n < numfields; n++)
      {
        GribDataset dataset;

        gribfield *gribField;
        ierr = g2_getfld(cgrib, n + 1, unpack, expand, &gribField);

        if(verbose)
        {
          // gfld->version = GRIB edition number ( currently 2 )
          // gfld->discipline = Message Discipline ( see Code Table 0.0 )
          qDebug() << "===================================";
          qDebug() << "field" << n << "version" << gribField->version << "discipline" << gribField->discipline;
        }

        // ID section ====================================================================================
        // gfld->idsect = Contains the entries in the Identification
        // Section ( Section 1 )
        // This element is a pointer to an array
        // that holds the data.
        // gfld->idsect[0]  = Identification of originating Centre
        // ( see Common Code Table C-1 )
        // 7 - US National Weather Service
        // gfld->idsect[1]  = Identification of originating Sub-centre
        // gfld->idsect[2]  = GRIB Master Tables Version Number
        // ( see Code Table 1.0 )
        // 0 - Experimental
        // 1 - Initial operational version number
        // gfld->idsect[3]  = GRIB Local Tables Version Number
        // ( see Code Table 1.1 )
        // 0     - Local tables not used
        // 1-254 - Number of local tables version used
        // gfld->idsect[4]  = Significance of Reference Time (Code Table 1.2)
        // 0 - Analysis
        // 1 - Start of forecast
        // 2 - Verifying time of forecast
        // 3 - Observation time
        // gfld->idsect[5]  = Year ( 4 digits )
        // gfld->idsect[6]  = Month
        // gfld->idsect[7)  = Day
        // gfld->idsect[8]  = Hour
        // gfld->idsect[9]  = Minute
        // gfld->idsect[10]  = Second
        // gfld->idsect[11]  = Production status of processed data
        // ( see Code Table 1.3 )
        // 0 - Operational products
        // 1 - Operational test products
        // 2 - Research products
        // 3 - Re-analysis products
        // gfld->idsect[12]  = Type of processed data ( see Code Table 1.4 )
        // 0  - Analysis products
        // 1  - Forecast products
        // 2  - Analysis and forecast products
        // 3  - Control forecast products
        // 4  - Perturbed forecast products
        // 5  - Control and perturbed forecast products
        // 6  - Processed satellite observations
        // 7  - Processed radar observations
        if(verbose)
          printArrInt("idsect", gribField->idsect, gribField->idsectlen);

        if(gribField->idsectlen > 11)
        {
          // Read timestamp  ========================================
          dataset.datetime = QDateTime(QDate(static_cast<int>(gribField->idsect[5]),
                                             static_cast<int>(gribField->idsect[6]),
                                             static_cast<int>(gribField->idsect[7])),
                                       QTime(static_cast<int>(gribField->idsect[8]),
                                             static_cast<int>(gribField->idsect[9]),
                                             static_cast<int>(gribField->idsect[10])), Qt::UTC);
        }
        if(!checkValue("Datetime is not valid", dataset.datetime.isValid(), true))
          continue;

        // gfld->ifldnum = field number within GRIB message
        if(verbose)
          qDebug() << "ifldnum" << gribField->ifldnum;

        // Grid definition ====================================================================================
        // gfld->griddef = Source of grid definition (see Code Table 3.0)
        // 0 - Specified in Code table 3.1
        // 1 - Predetermined grid Defined by originating centre
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-0.shtml
        if(verbose)
          qDebug() << "griddef" << gribField->griddef;
        if(!checkValue("Grid definition", gribField->griddef, g2int(0)))
          continue;

        // gfld->igdtnum = Grid Definition Template Number (Code Table 3.1)
        // Latitude/Longitude (See Template 3.0)
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml
        if(verbose)
          qDebug() << "igdtnum" << gribField->igdtnum;
        if(!checkValue("Grid Definition Template Number", gribField->igdtnum, g2int(0)))
          continue;

        // gfld->igdtmpl  = Contains the data values for the specified Grid
        // Definition Template ( NN=gfld->igdtnum ).  Each
        // element of this integer array contains an entry (in
        // the order specified) of Grid Defintion Template 3.NN
        // This element is a pointer to an array
        // that holds the data.
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_temp3-0.shtml

        // 0	/  15	Shape of the Earth (See Code Table 3.2)
        // 1	/  16	Scale Factor of radius of spherical Earth
        // 2	/  17-20	Scale value of radius of spherical Earth
        // 3	/  21	Scale factor of major axis of oblate spheroid Earth
        // 4	/  22-25	Scaled value of major axis of oblate spheroid Earth
        // 5	/  26	Scale factor of minor axis of oblate spheroid Earth
        // 6	/  27-30	Scaled value of minor axis of oblate spheroid Earth
        // 7	/  31-34	Ni — number of points along a parallel
        // 8	/  35-38	Nj — number of points along a meridian
        // 9	/  39-42	Basic angle of the initial production domain (see Note 1)
        // 10	/  43-46	Subdivisions of basic angle used to define extreme longitudes and latitudes, and direction increments (see Note 1)
        // 11	/  47-50	La1 — latitude of first grid point (see Note 1)
        // 12	/  51-54	Lo1 — longitude of first grid point (see Note 1)
        // 13	/  55	Resolution and component flags (see Flag Table 3.3)
        // 14	/  56-59	La2 — latitude of last grid point (see Note 1)
        // 15	/  60-63	Lo2 — longitude of last grid point (see Note 1)
        // 16	/  64-67	Di — i direction increment (see Notes 1 and 5)
        // 17	/  68-71	Dj — j direction increment (see Note 1 and 5)
        // 18	/  72	Scanning mode (flags — see Flag Table 3.4 and Note 6)
        // List of number of points along each meridian or parallel
        // (These octets are only present for quasi-regular grids as described in notes 2 and 3)

        if(verbose)
          // -      [0, 1, 2, 3, 4, 5, 6,   7,   8, 9,         10,       11,12, 13,        14,        15,      16,      17,18]
          // igdtmpl[6, 0, 0, 0, 0, 0, 0, 360, 181, 0, 4294967295, 90000000, 0, 48, -90000000, 359000000, 1000000, 1000000, 0]
          printArrInt("igdtmpl", gribField->igdtmpl, gribField->igdtlen);

        if(!checkValue("shape of earth", gribField->igdtmpl[0], g2int(6)))
          continue;
        if(!checkValue("radius scale factor", gribField->igdtmpl[1], g2int(0)))
          continue;
        if(!checkValue("scale value", gribField->igdtmpl[2], g2int(0)))
          continue;
        if(!checkValue("scale factor of major axis", gribField->igdtmpl[3], g2int(0)))
          continue;
        if(!checkValue("scale value of major axis", gribField->igdtmpl[4], g2int(0)))
          continue;
        if(!checkValue("scale factor of minor axis", gribField->igdtmpl[5], g2int(0)))
          continue;
        if(!checkValue("scale value of minor axis", gribField->igdtmpl[6], g2int(0)))
          continue;
        if(!checkValue("Ni", gribField->igdtmpl[7], g2int(360)))
          continue;
        if(!checkValue("Nj", gribField->igdtmpl[8], g2int(181)))
          continue;
        if(!checkValue("Basic angle", gribField->igdtmpl[9], g2int(0)))
          continue;
        if(!checkValue("resolution component flags", gribField->igdtmpl[13], g2int(48)))
          continue;
        if(!checkValue("scanning mode flags", gribField->igdtmpl[18], g2int(0)))
          continue;

        // if(!checkValue("i increment", gfld->igdtmpl[16], g2int(1))) continue;
        // if(!checkValue("j increment", gfld->igdtmpl[17], g2int(1))) continue;

        // g2int di = gfld->igdtmpl[16], dj = gfld->igdtmpl[17];
        // dataset.firstLatY = gfld->igdtmpl[11] / dj;
        // dataset.firstLonX = gfld->igdtmpl[12] / di;
        // dataset.lastLatY = gfld->igdtmpl[14] / dj;
        // dataset.lastLonX = gfld->igdtmpl[15] / di;

        // Product definition ====================================================================================
        // gfdl->ipdtnum = Product Definition Template Number(see Code Table 4.0)
        // Analysis or forecast at a horizontal level or in a horizontal layer at a point in time.
        if(verbose)
          qDebug() << "ipdtnum" << gribField->ipdtnum;

        // gfld->ipdtmpl  = Contains the data values for the specified Product
        // Definition Template ( N=gfdl->ipdtnum ). Each element
        // of this integer array contains an entry (in the
        // order specified) of Product Defintion Template 4.N.
        // This element is a pointer to an array
        // that holds the data.
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_temp4-0.shtml
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2-0-2.shtml
        // 0	/ 10 Parameter category (see Code table 4.1)
        // 1	/ 11 Parameter number (see Code table 4.2)
        // 2	/ 12 Type of generating process (see Code table 4.3)
        // 3	/ 13 Background generating process identifier (defined by originating centre)
        // 4	/ 14 Analysis or forecast generating process identified (see Code ON388 Table A)
        // 5	/ 15-16 Hours of observational data cutoff after reference time (see Note)
        // 6	/ 17 Minutes of observational data cutoff after reference time (see Note)
        // 7	/ 18 Indicator of unit of time range (see Code table 4.4)
        // 8	/ 19-22 Forecast time in units defined by octet 18
        // 9	/ 23 Type of first fixed surface (see Code table 4.5)
        // 10	/ 24 Scale factor of first fixed surface
        // 11	/ 25-28 Scaled value of first fixed surface
        // 12	/ 29 Type of second fixed surfaced (see Code table 4.5)
        // 13	/ 30 Scale factor of second fixed surface
        // 14	/ 31-34 Scaled value of second fixed surfaces
        // -          [0, 1, 2, 3,  4, 5, 6, 7, 8,   9,10,    11,  12,13,14
        // ipdtmpl(15)[2, 2, 0, 0, 81, 0, 0, 1, 0, 100, 0, 20000, 255, 0, 0]
        if(verbose)
          printArrInt("ipdtmpl", gribField->ipdtmpl, gribField->ipdtlen);

        if(!checkValue("Parameter category", gribField->ipdtmpl[0], g2int(2)))
          continue;
        if(!checkValue("Parameter number", gribField->ipdtmpl[1], {g2int(2), g2int(3)}))
          continue;
        if(gribField->ipdtmpl[1] == 2)
          dataset.parameterType = U_WIND;
        else if(gribField->ipdtmpl[1] == 3)
          dataset.parameterType = V_WIND;

        if(!checkValue("Time range", gribField->ipdtmpl[7], g2int(1)))
          continue;
        if(!checkValue("Surface type", gribField->ipdtmpl[9], {g2int(100), g2int(103)}))
          continue;
        if(gribField->ipdtmpl[9] == 100)
        {
          dataset.surfaceType = MBAR;
          dataset.surface =
            (gribField->ipdtmpl[11] / (gribField->ipdtmpl[10] > 0 ? gribField->ipdtmpl[10] : 1.f)) / 100.f;
          dataset.altFeetCalculated = atools::geo::meterToFeet(atools::geo::altMeterForPressureMbar(dataset.surface));
          // Round altitude to the next 2000 feet
          dataset.altFeetRounded = std::round(dataset.altFeetCalculated / 2000.f) * 2000.f;
        }
        else if(gribField->ipdtmpl[9] == 103)
        {
          dataset.surfaceType = METER_AGL;
          dataset.surface = gribField->ipdtmpl[11] / (gribField->ipdtmpl[10] > 0 ? gribField->ipdtmpl[10] : 1.f);
          dataset.altFeetCalculated = atools::geo::meterToFeet(dataset.surface);
          // Round altitude to the next 2000 feet
          dataset.altFeetRounded = std::round(dataset.altFeetCalculated / 10.f) * 10.f;
        }

        if(!checkValue("Second surface scale factor", gribField->ipdtmpl[13], g2int(0)))
          continue;
        if(!checkValue("Second surface value", gribField->ipdtmpl[14], g2int(0)))
          continue;

        if(verbose)
          qDebug() << "Calculated altitude" << dataset.altFeetCalculated
                   << "rounded altitude" << dataset.altFeetRounded;

        // Pack/unpack flags (ignored) ====================================================================================
        // gfld->unpacked = logical value indicating whether the bitmap and
        // data values were unpacked.  If false,
        if(!checkValue("Unpacked", gribField->unpacked, g2int(1)))
          continue;
        // gfld->bmap and gfld->fld pointers are nullified.
        // gfld->expanded = Logical value indicating whether the data field
        // was expanded to the grid in the case where a
        // bit-map is present.  If true, the data points in
        // gfld->fld match the grid points and zeros were
        // inserted at grid points where data was bit-mapped
        // out.  If false, the data values in gfld->fld were
        // not expanded to the grid and are just a consecutive
        // array of data points corresponding to each value of
        // "1" in gfld->bmap.
        if(!checkValue("Unpacked", gribField->expanded, g2int(1)))
          continue;
        // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-3.shtml

        // Data ====================================================================================
        // gfld->fld  = Array of gfld->ndpts unpacked data points.
        if(verbose)
          printArrFloat("fld", gribField->fld, std::min(gribField->ndpts, g2int(100)));

        qDebug() << Q_FUNC_INFO
                 << "param type" << dataset.parameterType
                 << "surface" << dataset.surface
                 << "surface type" << dataset.surfaceType
                 << "alt calculated" << dataset.altFeetCalculated
                 << "alt rounded" << dataset.altFeetRounded;

        // Copy data as is
        for(int i = 0; i < gribField->ndpts; i++)
          dataset.data.append(gribField->fld[i]);

        datasets.append(dataset);

        // checkValue("Number of values", g2int(dataset.data.size()),
        // g2int(std::abs(dataset.lastLonX - dataset.firstLonX + 1) *
        // std::abs(dataset.firstLatY - dataset.lastLatY + 1)));

        g2_free(gribField);
      }
      delete[] cgrib;
    }

    // Sort first by altitude from low to high and second by parameter type from U to V
    std::sort(datasets.begin(), datasets.end(),
              [] (const atools::grib::GribDataset & d1, const atools::grib::GribDataset & d2)->bool
              {
                if(atools::almostEqual(d1.altFeetCalculated, d2.altFeetCalculated))
                  return d1.parameterType < d2.parameterType;
                else
                  return d1.altFeetCalculated < d2.altFeetCalculated;
              });

    fclose(fptr);
  }
  else
    throw atools::Exception(tr("Cannot open file %1").arg(filename));

#if defined(Q_OS_WIN32)
  delete[] path;
#endif
}

void GribReader::readData(const QByteArray& data)
{
  if(data.isEmpty())
    throw atools::Exception(tr("GRIB data empty"));

  if(!validateGribData(data))
    throw atools::Exception(tr("Not a GRIB file"));

  // Create a temporary file since the GRIB reader can only deal with files
  atools::io::TempFile temp(data, ".grib");

  datasets.clear();
  readFile(temp.getFilePath());

  if(verbose)
  {
    qDebug() << Q_FUNC_INFO << "Datasets ============================================================";
    for(const GribDataset& dataset : datasets)
      qDebug() << dataset;
  }
}

void GribReader::clear()
{
  datasets.clear();
}

bool GribReader::validateGribFile(const QString& path)
{
  QFile file(path);
  if(file.open(QIODevice::ReadOnly))
    return validateGribData(file.read(8));
  else
    qWarning() << "cannot open" << file.fileName() << "reason" << file.errorString();
  return false;
}

bool GribReader::validateGribData(QByteArray bytes)
{
  QDataStream ds(&bytes, QIODevice::ReadOnly);
  qint8 c1 = 0, c2 = 0, c3 = 0, c4 = 0;
  qint32 version = 0;
  ds >> c1 >> c2 >> c3 >> c4 >> version;
  return c1 == 'G' && c2 == 'R' && c3 == 'I' && c4 == 'B' && version == 2;
}

} // namespace grib
} // namespace atools
