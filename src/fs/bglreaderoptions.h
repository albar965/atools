/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#ifndef BGLREADEROPTIONS_H_
#define BGLREADEROPTIONS_H_

#include <QRegularExpression>

namespace atools {
namespace fs {

class BglReaderOptions
{
public:
  BglReaderOptions();

  const QString& getBasepath() const
  {
    return basepath;
  }

  bool execDeletes() const
  {
    return !noDeletes;
  }

  bool filterRunways() const
  {
    return !noFilterRunways;
  }

  const QString& getSceneryFile() const
  {
    return sceneryFile;
  }

  bool isVerbose() const
  {
    return verbose;
  }

  bool isDebugAutocommit() const
  {
    return debugAutocommit;
  }

  bool noIncompleteObjects() const
  {
    return noIncomplete;
  }

  bool doesFilenameMatch(const QString& filename) const;
  bool doesAirportIcaoMatch(const QString& icao) const;

  void setSceneryFile(const QString& value)
  {
    sceneryFile = value;
  }

  void setBasepath(const QString& value)
  {
    basepath = value;
  }

  void setVerbose(bool value)
  {
    verbose = value;
  }

private:
  friend QDebug operator<<(QDebug out, const atools::fs::BglReaderOptions& opts);

  QString sceneryFile, basepath;
  bool verbose, noDeletes, noFilterRunways, noIncomplete, debugAutocommit;

  QStringList fileFilterRegexpStr, airportIcaoFilterRegexpStr;
  QList<QRegularExpression> fileFilterRegexp, airportIcaoFilterRegexp;
};

} // namespace fs
} // namespace atools

#endif /* BGLREADEROPTIONS_H_ */
