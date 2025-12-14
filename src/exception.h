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

#ifndef ATOOLS_EXCEPTION_H
#define ATOOLS_EXCEPTION_H

#include <QException>
#include <QString>

namespace atools {

/*
 * Exception that will be thrown by most atools in case of error.
 *
 * Qt Concurrent supports throwing and catching exceptions across thread boundaries,
 * provided that the exception inherit from QException and implement two helper functions.
 */
class Exception :
  public QException
{
public:
  explicit Exception()
  {
  }

  explicit Exception(const QString& messageStr)
  {
    setMessage(messageStr);
  }

  virtual const char *what() const Q_DECL_NOEXCEPT override;

  /* Override from QException to allow passing the exception across threads */
  virtual void raise() const override;
  virtual Exception *clone() const override;

protected:
  void setMessage(const QString& messageStr)
  {
    msg = messageStr;
    whatMsg = msg.toUtf8();
  }

  QString msg;
  QByteArray whatMsg;
};

} // namespace atools

#endif // ATOOLS_EXCEPTION_H
