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

#include "paintercontextsaver.h"

#include <QPainter>

namespace atools {
namespace util {

PainterContextSaver::PainterContextSaver(QPainter *painterToSave)
  : painter(painterToSave)
{
  if(painter != nullptr)
    painter->save();
}

PainterContextSaver::~PainterContextSaver()
{
  if(painter != nullptr)
    painter->restore();
}

} // namespace util
} // namespace atools
