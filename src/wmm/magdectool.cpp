/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#include "magdectool.h"
#include "io/tempfile.h"
#include "exception.h"
#include "geo/pos.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <locale.h>

#include "wmm/GeomagnetismHeader.h"
#ifdef WRITE_GEOID_BUFFER
#include "EGM9615.h"
#endif
}

#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QList>

#include <cstring>

namespace atools {
namespace wmm {

/* Copy of MAG_Grid function with simplifications also avoiding the need to write the output to a file */
QList<float> MAG_GridInternal(int year, int month,
                                MAGtype_MagneticModel *magneticModel,
                                MAGtype_Geoid *Geoid, MAGtype_Ellipsoid ellipsoid);

// ==============================================================================

MagDecTool::MagDecTool()
{

}

MagDecTool::~MagDecTool()
{
  clear();
}

void MagDecTool::init(const QDate& dateTimeParam)
{
  return init(dateTimeParam.year(), dateTimeParam.month());
}

void MagDecTool::init(int year, int month)
{
  clear();

  if(year <= 0 || month <= 0)
  {
    QDateTime dt = QDateTime::currentDateTimeUtc();

    if(year <= 0)
      year = dt.date().year();

    if(month <= 0)
      month = dt.date().month();
  }

  referenceDate.setDate(year, month, 1);

  // Put coeffizients file into a temporary, so that the C code can read it
  atools::io::TempFile temp(QString(":/atools/resources/wmm/WMM.COF"), "_wmm.cof");

  // Have to change locale to C since sscanf which is used in the geomagnetism library is locale dependent
  char *oldlocale = setlocale(LC_NUMERIC, nullptr);
  setlocale(LC_NUMERIC, "C");

#if defined(Q_OS_WIN32)
  // Windows fopen uses local charset for filename - convert UTF-8 to UTF-16 and use wfopen
  wchar_t *path = new wchar_t[static_cast<unsigned int>(temp.getFilePath().size()) + 1];
  temp.getFilePath().toWCharArray(path);
  path[temp.getFilePath().size()] = L'\0';

  FILE *f = _wfopen(path, L"r");
#else
  FILE *f = fopen(temp.getFilePathData(), "r");
#endif

  // https://stackoverflow.com/questions/30470866/c-to-c-array-of-pointers-conversion-issue
  MAGtype_MagneticModel *magneticModel;
  if(!MAG_robustReadMagModels(f, &magneticModel, 1))
    throw atools::Exception(tr("Magnetic coeffizient file \"%1\" not found.").arg(temp.getFilePath()));

  MAGtype_Ellipsoid ellipsoid;
  MAGtype_Geoid geoid;
  if(!MAG_SetDefaults(&ellipsoid, &geoid))
    throw atools::Exception(tr("Error in MAG_SetDefaults."));

  QList<float> geoidBuffer = readGeoidBuffer();
  geoid.GeoidHeightBuffer = geoidBuffer.data();
  geoid.Geoid_Initialized = 1;

  // Calculate declination grid
  QList<float> declinations = MAG_GridInternal(year, month, magneticModel, &geoid, ellipsoid);
  if(declinations.isEmpty())
    throw atools::Exception(tr("Error in MAG_GridInternal."));

  MAG_FreeMagneticModelMemory(magneticModel);

  // Create new plain float array and copy data before vector is destroyed
  magdecGrid = new float[static_cast<unsigned int>(declinations.size())];
  std::memcpy(magdecGrid, declinations.data(), static_cast<unsigned int>(declinations.size()) * sizeof(float));

  fclose(f);

  // Reset locale to previous value
  setlocale(LC_NUMERIC, oldlocale);

#if defined(Q_OS_WIN32)
  delete[] path;
#endif
}

// Only needed to write the cumbersome large 8MB EGM9615.h file into a plain file
#ifdef WRITE_GEOID_BUFFER
void MagDecTool::writeGeoidBuffer()
{
  size_t bufferSize = sizeof(GeoidHeightBuffer);
  size_t bufferNum = bufferSize / sizeof(GeoidHeightBuffer[0]);

  QFile file("EGM9615.buf");
  if(file.open(QIODevice::WriteOnly))
  {
    QDataStream ds(&file);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    for(float value : GeoidHeightBuffer)
      ds << value;

    file.close();
  }
}

#endif

QString MagDecTool::getVersion() const
{
  return VERSIONDATE_LARGE;
}

float MagDecTool::getMagVar(const geo::Pos& pos)
{
  if(pos.nearGrid(1.f, atools::geo::Pos::POS_EPSILON_500M))
    // Get point value without interpolation if position is close to a grid point
    return getMagVar(atools::roundToInt(pos.getLonX()), atools::roundToInt(pos.getLatY()));
  else
  {
    // Get all four boundaries
    float left = std::floor(pos.getLonX());
    float right = std::ceil(pos.getLonX());
    float top = std::ceil(pos.getLatY());
    float bottom = std::floor(pos.getLatY());

    // Interpolate along top boundary
    float t = atools::interpolate(getMagVar(left, top), getMagVar(right, top), left, right, pos.getLonX());
    // Interpolate along bottom boundary
    float b = atools::interpolate(getMagVar(left, bottom), getMagVar(right, bottom), left, right, pos.getLonX());

    // Interpolate between top and bottom
    return atools::interpolate(t, b, top, bottom, pos.getLatY());
  }
}

void MagDecTool::clear()
{
  delete magdecGrid;
  magdecGrid = nullptr;
}

QList<float> MagDecTool::readGeoidBuffer()
{
  QList<float> retval;
  QFile file(":/atools/resources/wmm/EGM9615.buf");
  if(file.open(QIODevice::ReadOnly))
  {
    QDataStream ds(&file);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    while(!ds.atEnd())
    {
      float value;
      ds >> value;
      retval.append(value);
    }
    file.close();
  }
  else
    throw atools::Exception(tr("Cannot open geoid buffer \"%1\".").arg(file.fileName()));

  return retval;
}

QList<float> MAG_GridInternal(int year, int month, MAGtype_MagneticModel *magneticModel,
                                MAGtype_Geoid *geoid, MAGtype_Ellipsoid ellipsoid)
{
  // Boundary always covers whole world
  MAGtype_CoordGeodetic minimum;
  minimum.phi = -90.;
  minimum.lambda = -180.;
  minimum.HeightAboveGeoid = minimum.HeightAboveEllipsoid = 0.;
  minimum.UseGeoid = 1;

  MAGtype_CoordGeodetic maximum;
  maximum.phi = 90.;
  maximum.lambda = 179.;
  maximum.HeightAboveGeoid = maximum.HeightAboveEllipsoid = 0.;
  maximum.UseGeoid = 1;

  // Only one date - no range
  MAGtype_Date startdate, enddate;
  startdate.DecimalYear = enddate.DecimalYear = year + (month - 1) / 12.;

  int numTerms;
  double b, c;

  MAGtype_MagneticModel *timedMagneticModel;
  MAGtype_CoordSpherical coordSpherical;
  MAGtype_MagneticResults magneticResultsSph, magneticResultsGeo, magneticResultsSphVar, magneticResultsGeoVar;
  MAGtype_SphericalHarmonicVariables *sphericalVariables;
  MAGtype_GeoMagneticElements geoMagneticElements, errors;
  MAGtype_LegendreFunction *legendreFunction;

  double cord_step_size = 1.;
  if(fabs(cord_step_size) < 1.0e-10)
    cord_step_size = 99999.0; // checks to make sure that the step_size is not too small

  numTerms = ((magneticModel->nMax + 1) * (magneticModel->nMax + 2) / 2);
  timedMagneticModel = MAG_AllocateModelMemory(numTerms);
  legendreFunction = MAG_AllocateLegendreFunctionMemory(numTerms); // For storing the ALF functions
  sphericalVariables = MAG_AllocateSphVarMemory(magneticModel->nMax);

  QList<float> retval;
  retval.reserve(360 * 181);

  b = minimum.phi;
  c = minimum.lambda;
  for(minimum.phi = b; minimum.phi <= maximum.phi; minimum.phi += cord_step_size) // Latitude Y loop
  {
    for(minimum.lambda = c; minimum.lambda <= maximum.lambda; minimum.lambda += cord_step_size) // Longitude X loop
    {
      if(geoid->UseGeoid == 1)
        // This converts the height above mean sea level to height above the WGS-84 ellipsoid
        MAG_ConvertGeoidToEllipsoidHeight(&minimum, geoid);
      else
        minimum.HeightAboveEllipsoid = minimum.HeightAboveGeoid;

      MAG_GeodeticToSpherical(ellipsoid, minimum, &coordSpherical);

      // Compute Spherical Harmonic variables
      MAG_ComputeSphericalHarmonicVariables(ellipsoid, coordSpherical, magneticModel->nMax, sphericalVariables);

      // Compute ALF  Equations 5-6, WMM Technical report
      MAG_AssociatedLegendreFunction(coordSpherical, magneticModel->nMax, legendreFunction);

      // This modifies the Magnetic coefficients to the correct date.
      MAG_TimelyModifyMagneticModel(startdate, magneticModel, timedMagneticModel);

      // Accumulate the spherical harmonic coefficients Equations 10:12 , WMM Technical report
      MAG_Summation(legendreFunction, timedMagneticModel, *sphericalVariables, coordSpherical, &magneticResultsSph);

      // Sum the Secular Variation Coefficients, Equations 13:15 , WMM Technical report
      MAG_SecVarSummation(legendreFunction, timedMagneticModel, *sphericalVariables, coordSpherical,
                          &magneticResultsSphVar);

      // Map the computed Magnetic fields to Geodetic coordinates Equation 16 , WMM Technical report
      MAG_RotateMagneticVector(coordSpherical, minimum, magneticResultsSph, &magneticResultsGeo);

      // Map the secular variation field components to Geodetic coordinates, Equation 17 , WMM Technical report
      MAG_RotateMagneticVector(coordSpherical, minimum, magneticResultsSphVar, &magneticResultsGeoVar);

      // Calculate the Geomagnetic elements, Equation 18 , WMM Technical report
      MAG_CalculateGeoMagneticElements(&magneticResultsGeo, &geoMagneticElements);
      MAG_WMMErrorCalc(geoMagneticElements.H, &errors);

      retval.append(static_cast<float>(geoMagneticElements.Decl));
    } // Longitude Loop
  } // Latitude Loop

  MAG_FreeMagneticModelMemory(timedMagneticModel);
  MAG_FreeLegendreMemory(legendreFunction);
  MAG_FreeSphVarMemory(sphericalVariables);

  return retval;
}

} // namespace wmm
} // namespace atools
