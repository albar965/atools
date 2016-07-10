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

#include "logging/loggingdefs.h"
#include "fs/bgl/ap/del/deleteairport.h"
#include "fs/bgl/converter.h"
#include "io/binarystream.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString DeleteAirport::deleteAllFlagsToStr(del::DeleteAllFlags flags)
{
  QString retval;
  if((flags & del::APPROACHES) == del::APPROACHES)
    retval += "APPROACHES,";
  if((flags & del::APRONLIGHTS) == del::APRONLIGHTS)
    retval += "APRONLIGHTS,";
  if((flags & del::APRONS) == del::APRONS)
    retval += "APRONS,";
  if((flags & del::COMS) == del::COMS)
    retval += "FREQUENCIES,";
  if((flags & del::HELIPADS) == del::HELIPADS)
    retval += "HELIPADS,";
  if((flags & del::RUNWAYS) == del::RUNWAYS)
    retval += "RUNWAYS,";
  if((flags & del::STARTS) == del::STARTS)
    retval += "STARTS,";
  if((flags & del::TAXIWAYS) == del::TAXIWAYS)
    retval += "TAXIWAYS,";
  // TODO find out missing flags with bgl compiler
  // else if(flags != 0)
  // qWarning().nospace().noquote() << "Found unknown delete all flags 0x" << hex << flags;

  if(retval.endsWith(","))
    retval.chop(1);

  return retval;
}

DeleteAirport::DeleteAirport(const BglReaderOptions *options, BinaryStream *bs)
  : Record(options, bs)
{
  flags = static_cast<del::DeleteAllFlags>(bs->readShort());
  numRunways = bs->readUByte();
  numStarts = bs->readUByte();
  numFrequencies = bs->readUByte();
  bs->readUByte(); // unused

  // Print warnings for unsupported deletion cases
  if(numRunways > 0)
    qWarning().nospace().noquote() << "Found DeleteAirport with " << numRunways << " numRunways";
  if(numStarts > 0)
    qWarning().nospace().noquote() << "Found DeleteAirport with " << numStarts << " numStarts";
  if(numFrequencies > 0)
    qWarning().nospace().noquote() << "Found DeleteAirport with " << numFrequencies << " numFrequencies";

  for(int i = 0; i < numRunways; i++)
    deleteRunways.append(DeleteRunway(options, bs));

  for(int i = 0; i < numStarts; i++)
    deleteStarts.append(DeleteStart(options, bs));

  for(int i = 0; i < numFrequencies; i++)
    deleteComs.append(DeleteCom(options, bs));
}

QDebug operator<<(QDebug out, const DeleteAirport& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << "DeleteAirport[flags "
  << DeleteAirport::deleteAllFlagsToStr(record.flags);
  out << record.deleteRunways;
  out << record.deleteStarts;
  out << record.deleteComs;
  out << "]";
  return out;
}

DeleteAirport::~DeleteAirport()
{
}

} // namespace bgl
} // namespace fs
} // namespace atools
