/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

#include "fs/db/ap/apronwriter.h"
#include "fs/db/datawriter.h"
#include "fs/bgl/util.h"
#include "fs/navdatabaseoptions.h"
#include "fs/db/ap/airportwriter.h"
#include "fs/bgl/ap/rw/runway.h"

namespace atools {
namespace fs {
namespace db {

using atools::fs::bgl::Apron;
using atools::fs::bgl::Apron2;
using atools::fs::bgl::Runway;

void ApronWriter::writeObject(const std::pair<const bgl::Apron *, const bgl::Apron2 *> *type)
{
  if(getOptions().isVerbose())
    qDebug() << "Writing Apron for airport "
             << getDataWriter().getAirportWriter()->getCurrentAirportIdent();

  bind(":apron_id", getNextId());
  bind(":airport_id", getDataWriter().getAirportWriter()->getCurrentId());
  bind(":surface", Runway::surfaceToStr(type->first->getSurface()));

  bindBool(":is_draw_surface", type->second->isDrawSurface());
  bindBool(":is_draw_detail", type->second->isDrawDetail());

  bindBglCoordinateList(":vertices", type->first->getVertices());

  if(getOptions().isIncludedBglObject(type::APRON2))
  {
    bindBglCoordinateList(":vertices2", type->second->getVertices());

    // Triangles are space and comma separated
    bind(":triangles", toBytes(type->second->getTriangleIndex()));
  }
  else
  {
    bindNullString(":vertices2");
    bindNullString(":triangles");
  }

  executeStatement();
}

QByteArray ApronWriter::toBytes(const QList<int>& triangles)
{
  QByteArray blob;
  QDataStream out(&blob, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out.setFloatingPointPrecision(QDataStream::SinglePrecision);

  out << static_cast<quint32>(triangles.size());
  for(int i = 0; i < triangles.size(); i += 3)
    out << triangles.at(i) << triangles.at(i + 1) << triangles.at(i + 2);
  return blob;
}

} // namespace writer
} // namespace fs
} // namespace atools
