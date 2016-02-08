/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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

#include "fs/writer/ap/apronwriter.h"
#include "fs/writer/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/bglreaderoptions.h"
#include "fs/writer/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace writer {

using atools::fs::bgl::Apron;
using atools::fs::bgl::Apron2;
using atools::fs::bgl::Runway;
using atools::sql::SqlQuery;

void ApronWriter::writeObject(const QPair<const bgl::Apron *, const bgl::Apron2 *> *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Apron for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":apron_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":surface", Runway::surfaceToStr(type->first->getSurface()));
  bindBool(":is_draw_surface", type->second->isDrawSurface());
  bindBool(":is_draw_detail", type->second->isDrawDetail());

  // TODO create a WKT polygon from the triangles
  QStringList list;
  for(const bgl::BglPosition& pos : type->first->getVertices())
    list.push_back(QString::number(pos.getLonX(), 'g', 8) + " " +
                   QString::number(pos.getLatY(), 'g', 8));
  bind(":vertices", list.join(", "));

  list.clear();
  for(const bgl::BglPosition& pos : type->second->getVertices())
    list.push_back(QString::number(pos.getLonX(), 'g', 8) + " " +
                   QString::number(pos.getLatY(), 'g', 8));
  bind(":vertices2", list.join(", "));

  bind(":triangles", toString(type->second->getTriangles()));

  executeStatement();
}

QString ApronWriter::toString(const QList<int>& triangles)
{
  QString retval;
  for(int i = 0; i < triangles.size(); i += 3)
  {
    if(!retval.isEmpty())
      retval += ", ";
    retval.append(QString::number(triangles.at(i))).append(" ");
    retval.append(QString::number(triangles.at(i + 1))).append(" ");
    retval.append(QString::number(triangles.at(i + 2)));
  }
  return retval;
}

} // namespace writer
} // namespace fs
} // namespace atools
