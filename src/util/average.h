/*****************************************************************************
* Copyright 2015-2021 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_AVERAGE_H
#define ATOOLS_AVERAGE_H

#include <QList>
#include <QVector>

namespace atools {
namespace util {

class MovingAverage
{
public:
  MovingAverage(int numberOfSamples)
    : numSamples(numberOfSamples)
  {
    samples.fill(0, numSamples);
  }

  void addSample(float value);

  float getAverage() const;

  void reset();

private:
  int currentSample = 0, numSamples = 0;
  QVector<float> samples;
  float total = 0.f;
};

/*
 * Calculate moving average over a time series for two values.
 */
class MovingAverageTime
{
public:
  /*
   * rangeMs Maximum time in milliseconds to calculate average;
   */
  MovingAverageTime(qint64 rangeMs)
    : timeRangeMs(rangeMs)
  {
  }

  /* Start sampling and set initial timestamp in milliseconds */
  void startSamples(qint64 timestampMs);

  /* Add two samples with timestamp. The timestamp - previous timestamp will be used as duration and used to weight the sample */
  void addSamples(float value1, float value2, qint64 timestampMs);

  void getAverages(float& average1, float& average2) const;

  /* Methods for single values */
  void addSample(float value1, qint64 timestampMs);
  float getAverage1() const;
  float getAverage2() const;

  void reset()
  {
    samples.clear();
    total1 = total2 = 0.f;
    beforeFirstTimestampMs = 0;
  }

  int size() const
  {
    return samples.size();
  }

  /* Print the size of all container classes to detect overflow or memory leak conditions */
  void debugDumpContainerSizes() const;

  struct Sample
  {
    Sample()
    {
    }

    Sample(float value1Param, float value2Param, qint64 timestampParam)
      : value1(value1Param), value2(value2Param), timestamp(timestampParam)
    {
    }

    float value1, value2;
    qint64 timestamp;
  };

private:
  qint64 timeRangeMs, beforeFirstTimestampMs;
  QList<Sample> samples;
  float total1 = 0.f, total2 = 0.f;
};

} // namespace util
} // namespace atools

Q_DECLARE_TYPEINFO(atools::util::MovingAverageTime::Sample, Q_PRIMITIVE_TYPE);

#endif // ATOOLS_AVERAGE_H
