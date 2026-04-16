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

#ifndef ATOOLS_DATAEXCHANGEFLAGS_H
#define ATOOLS_DATAEXCHANGEFLAGS_H

#include <QLatin1String>

namespace atools {
namespace gui {
namespace dataexchange {

/* Common commands also used internally. Commands are set if the property exists. */
const QLatin1String STARTUP_COMMAND_QUIT("quit"); /* Terminate application */
const QLatin1String STARTUP_COMMAND_ACTIVATE("activate"); /* Show and raise application window */

} // namespace dataexchange
} // namespace gui
} // namespace atools

#endif // ATOOLS_DATAEXCHANGEFLAGS_H
