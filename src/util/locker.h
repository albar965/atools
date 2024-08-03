/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_LOCKER_H
#define ATOOLS_LOCKER_H

#include <QMutex>

/*
 * Simple base class for an object that can be locked.
 */
class Lockable
{
public:
  void lock()
  {
    mutex.lock();
  }

  void unlock()
  {
    mutex.unlock();
  }

private:
  QMutex mutex;
};

/*
 * A convenience class that simplifies locking and unlocking mutexes based on Locker.
 * A lock is established in the constructor and unlocked in the destructor.
 */
template<typename TYPE>
class Locker
{
public:
  explicit Locker(TYPE& lockableParam)
    : lockable(&lockableParam)
  {
    if(lockable != nullptr)
      lockable->lock();
  }

  explicit Locker(TYPE *lockableParam)
    : lockable(lockableParam)
  {
    if(lockable != nullptr)
      lockable->lock();
  }

  ~Locker()
  {
    if(lockable != nullptr)
      lockable->unlock();
  }

private:
  TYPE *lockable = nullptr;
};

#endif // ATOOLS_LOCKER_H
