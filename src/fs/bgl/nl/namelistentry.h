/*
 * NamelistIcaoRecord.h
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#ifndef BGL_NAMELISTICAOIDENT_H_
#define BGL_NAMELISTICAOIDENT_H_

#include "../record.h"

#include <QString>

namespace atools {
namespace fs {
namespace bgl {

class NamelistEntry
{
public:
  NamelistEntry()
  {
  }

  virtual ~NamelistEntry();

  const QString& getAirportIdent() const
  {
    return airportIdent;
  }

  const QString& getAirportName() const
  {
    return airportName;
  }

  const QString& getCityName() const
  {
    return cityName;
  }

  const QString& getCountryName() const
  {
    return countryName;
  }

  const QString& getRegionIdent() const
  {
    return regionIdent;
  }

  const QString& getRegionName() const
  {
    return regionName;
  }

  const QString& getStateName() const
  {
    return stateName;
  }

private:
  friend class Namelist;
  friend QDebug operator<<(QDebug out, const NamelistEntry& record);

  QString regionName, countryName, stateName, cityName, airportName, airportIdent, regionIdent;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_NAMELISTICAOIDENT_H_ */
