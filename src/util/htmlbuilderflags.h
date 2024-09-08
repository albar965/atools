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

#ifndef ATOOLS_HTMLBUILDERFLAGS_H
#define ATOOLS_HTMLBUILDERFLAGS_H

#include "util/flags.h"

namespace atools {
namespace util {

namespace html {
/* HTML formatting flags for text */
enum Flag : quint32
{
  NONE = 0,

  /* HTML formatting attributes */
  BOLD = 1 << 0,
  ITALIC = 1 << 1,
  UNDERLINE = 1 << 2,
  STRIKEOUT = 1 << 3,
  SUBSCRIPT = 1 << 4,
  SUPERSCRIPT = 1 << 5,
  SMALL = 1 << 6,
  BIG = 1 << 7,
  CODE = 1 << 8,
  PRE = 1 << 9,

  NOBR = 1 << 10, /* HTML no break */

  LINK_NO_UL = 1 << 11, /* Do not underline links */
  NO_ENTITIES = 1 << 12, /* Do not convert entities */
  ALIGN_RIGHT = 1 << 13, /* Only for table data */
  ALIGN_LEFT = 1 << 14, /* Only for table header data */
  ALIGN_CENTER = 1 << 15, /* Only for table header data */
  AUTOLINK = 1 << 16, /* Automatically create links from http:// and https:// in text */
  REPLACE_CRLF = 1 << 17, /* Replace carriage return and linefeed with <br/> */

  NOBR_WHITESPACE = 1 << 18 /* HTML no break at whitespace for tooltips
                             * like: <p style='white-space:pre'>. Only for paragraphs. */
};

ATOOLS_DECLARE_FLAGS_32(Flags, atools::util::html::Flag)
ATOOLS_DECLARE_OPERATORS_FOR_FLAGS(atools::util::html::Flags)

} // namespace html
} // namespace util
} // namespace atools

#endif // ATOOLS_HTMLBUILDERFLAGS_H
