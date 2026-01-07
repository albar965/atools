/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "util/average.h"

#include <QDebug>

namespace atools {
namespace util {

// ================================================================================================

void atools::util::MovingAverage::addSample(float value)
{
  if(currentSample < numSamples)
  {
    samples[currentSample++] = value;
    total += value;
  }
  else
  {
    float& oldest = samples[currentSample % numSamples];
    total += value - oldest;
    oldest = value;

    if(currentSample > numSamples * 2)
      // Avoid overflow
      currentSample = numSamples;
    currentSample++;
  }
}

float atools::util::MovingAverage::getAverage() const
{
  return total / std::min(currentSample, numSamples);
}

void MovingAverage::reset()
{
  currentSample = 0;
  samples.clear();
  samples.fill(0, numSamples);
  total = 0.f;
}

// ================================================================================================

void MovingAverageTime::startSamples(qint64 timestampMs)
{
  // First one is ignored since it has no duration value
  reset();
  beforeFirstTimestampMs = timestampMs;
}

void atools::util::MovingAverageTime::addSamples(float value1, float value2, qint64 timestampMs)
{
  if(samples.isEmpty())
    beforeFirstTimestampMs = timestampMs;

  // Check for oldest entries and remove if needed
  while(!samples.isEmpty() && beforeFirstTimestampMs < samples.constLast().timestamp - timeRangeMs)
  {
    Sample first = samples.takeFirst();

    // Adjust total by removing this weighted value
    qint64 diff = first.timestamp - beforeFirstTimestampMs;

    total1 -= first.value1 * diff;
    total2 -= first.value2 * diff;

    beforeFirstTimestampMs = first.timestamp;
  }

  // Add new value and update totals with weighted value
  qint64 duration = timestampMs - (samples.isEmpty() ? beforeFirstTimestampMs : samples.constLast().timestamp);

  if(duration < 0L)
  {
    // Time jump backwards - reset all
    reset();
    beforeFirstTimestampMs = timestampMs;
  }
  else
  {
    total1 += value1 * duration;
    total2 += value2 * duration;
    samples.append(Sample(value1, value2, timestampMs));
  }
}

void MovingAverageTime::addSample(float value1, qint64 timestampMs)
{
  addSamples(value1, 0.f, timestampMs);
}

void atools::util::MovingAverageTime::getAverages(float& average1, float& average2) const
{
  average1 = average2 = 0.f;

  if(samples.isEmpty())
    return;

  qint64 totalDuration = samples.constLast().timestamp - beforeFirstTimestampMs;

  if(totalDuration > 0)
  {
    average1 = total1 / totalDuration;
    average2 = total2 / totalDuration;
  }
  else
  {
    average1 = total1;
    average2 = total2;
  }
}

float MovingAverageTime::getAverage1() const
{
  if(samples.isEmpty())
    return 0.f;

  qint64 totalDuration = samples.constLast().timestamp - beforeFirstTimestampMs;

  if(totalDuration > 0)
    return total1 / totalDuration;
  else
    return total1;
}

float MovingAverageTime::getAverage2() const
{
  if(samples.isEmpty())
    return 0.f;

  qint64 totalDuration = samples.constLast().timestamp - beforeFirstTimestampMs;

  if(totalDuration > 0)
    return total2 / totalDuration;
  else
    return total2;
}

void MovingAverageTime::debugDumpContainerSizes() const
{
  qDebug() << Q_FUNC_INFO << "samples.size()" << samples.size();
}

} // namespace util
} // namespace atools
