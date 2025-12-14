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

#ifndef ATOOLS_FS_NAVDATABASEFLAGS_H
#define ATOOLS_FS_NAVDATABASEFLAGS_H

#include "util/flags.h"

#include <QCoreApplication>

namespace atools {
namespace fs {

/* Result flags for navdatabase creation */
enum ResultFlag : quint32
{
  COMPILE_NONE = 0,
  COMPILE_BASIC_VALIDATION_ERROR = 1 << 0, /* Basic validation enabled and found error */
  COMPILE_MSFS_NAVIGRAPH_FOUND = 1 << 1, /* Found MSFS Navigraph installation during compilation */
  COMPILE_CANCELED = 1 << 2, /* User clicked cancel on progress */
  COMPILE_FAILED = 1 << 3, /* Caught exception */
};

ATOOLS_DECLARE_FLAGS_32(ResultFlags, atools::fs::ResultFlag)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::ResultFlags)

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASEFLAGS_H
