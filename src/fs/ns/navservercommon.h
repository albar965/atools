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

#ifndef ATOOLS_NS_COMMON_H
#define ATOOLS_NS_COMMON_H

#include <QLoggingCategory>

namespace atools {
namespace fs {
namespace ns {

enum NavServerOption
{
  NONE = 0x00,
  VERBOSE = 0x01,
  HIDE_HOST = 0x02
};

Q_DECLARE_FLAGS(NavServerOptions, NavServerOption);
Q_DECLARE_OPERATORS_FOR_FLAGS(atools::fs::ns::NavServerOptions);

/* Declare a own logging category to append in the text edit */
Q_DECLARE_LOGGING_CATEGORY(gui);

} // namespace ns
} // namespace fs
} // namespace atools

#endif // ATOOLS_NS_COMMON_H
