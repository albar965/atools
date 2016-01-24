/*
 *	 IlsVor.cpp
 *
 *  Created on: 25.04.2015
 *      Author: alex
 */

#include "ilsvor.h"

#include "../bglposition.h"

namespace atools {
namespace fs {
namespace bgl {

using atools::io::BinaryStream;

QString IlsVor::ilsVorTypeToStr(nav::IlsVorType type)
{
  switch(type)
  {
    case nav::TERMINAL:
      return "TERMINAL";

    case nav::LOW:
      return "LOW";

    case nav::HIGH:
      return "HIGH";

    case nav::ILS:
      return "ILS";

    case nav::VOT:
      return "VOT";
  }
  qWarning().nospace().noquote() << "Unknown ILS/VOR type " << type;
  return "";
}

IlsVor::IlsVor(BinaryStream *bs)
  : Record(bs)
{
  type = static_cast<nav::IlsVorType>(bs->readByte());
}

IlsVor::~IlsVor()
{
}

QDebug operator<<(QDebug out, const IlsVor& record)
{
  QDebugStateSaver saver(out);

  out.nospace().noquote() << static_cast<const Record&>(record)
  << " IlsVor["
  << "type " << IlsVor::ilsVorTypeToStr(record.getType())
  << "]";
  return out;
}

} // namespace bgl
} // namespace fs
} // namespace atools
