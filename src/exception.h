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

#ifndef ATOOLS_EXCEPTION_H
#define ATOOLS_EXCEPTION_H

#include <QString>

namespace atools {

/*
 * Exception that will be thrown by most atools in case of error.
 */
class Exception :
  public std::exception
{
public:
  Exception(const QString& messageStr);

  Exception();

  virtual ~Exception();

  virtual const char *what() const Q_DECL_NOEXCEPT override;

  QString getMessage() const
  {
    return message;
  }

protected:
  QString message;
  QByteArray whatMessage;
};

} // namespace atools

#endif // ATOOLS_EXCEPTION_H
