/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_CONTEXTSAVER_H
#define ATOOLS_CONTEXTSAVER_H

namespace atools {
namespace util {

/*
 * Saves a value reference, sets variable to value and restores it on destruction.
 */
template<typename TYPE>
class ContextSaver
{
public:
  /* Sets value and resets back to previous value on destruction. */
  ContextSaver(TYPE& valueReference, TYPE value)
    : valueSaved(valueReference), valueRef(valueReference)
  {
    // Set new value
    valueRef = value;
  }

  /* Sets value and resets back to resetValue on destruction. */
  ContextSaver(TYPE& valueReference, TYPE value, TYPE resetValue)
    : valueSaved(resetValue), valueRef(valueReference)
  {
    // Set new value
    valueRef = value;
  }

  ~ContextSaver()
  {
    // Restore prevous value
    valueRef = valueSaved;
  }

private:
  TYPE valueSaved;
  TYPE& valueRef;
};

/*
 * Saves a boolean value reference, sets variable to value and restores it on destruction.
 */
class ContextSaverBool
  : public atools::util::ContextSaver<bool>
{
public:
  /* Sets value and resets back to resetValue on destruction. */
  ContextSaverBool(bool& valueReference, bool value = true, bool resetValue = false)
    : atools::util::ContextSaver<bool>(valueReference, value, resetValue)
  {

  }

};

} // namespace util
} // namespace atools

#endif // ATOOLS_CONTEXTSAVER_H
