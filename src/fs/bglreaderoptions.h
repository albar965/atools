/*
 * BglReaderOptions.h
 *
 *  Created on: 21.05.2015
 *      Author: alex
 */

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

private:
  friend QDebug operator<<(QDebug out, const BglReaderOptions& opts);

  QString sceneryFile, basepath;
  bool verbose, noDeletes, noFilterRunways, noIncomplete, debugAutocommit;

  QStringList fileFilterRegexpStr, airportIcaoFilterRegexpStr;
  QList<QRegularExpression> fileFilterRegexp, airportIcaoFilterRegexp;
};

} // namespace fs
} // namespace atools

#endif /* BGLREADEROPTIONS_H_ */
