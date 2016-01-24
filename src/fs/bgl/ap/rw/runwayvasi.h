/*
 * RunwayVasi.h
 *
 *  Created on: 22.04.2015
 *      Author: alex
 */

#ifndef BGL_RUNWAYVASI_H_
#define BGL_RUNWAYVASI_H_

#include "../../record.h"

#include <QString>
#include <QList>

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

namespace rw {

enum VasiType
{
  NONE = 0x00,
  VASI21 = 0x01,
  VASI31 = 0x02,
  VASI22 = 0x03,
  VASI32 = 0x04,
  VASI23 = 0x05,
  VASI33 = 0x06,
  PAPI2 = 0x07,
  PAPI4 = 0x08,
  TRICOLOR = 0x09,
  PVASI = 0x0a,
  TVASI = 0x0b,
  BALL = 0x0c,
  APAP_PANELS = 0x0d
};

} // namespace rw

class RunwayVasi :
  public Record
{
public:
  RunwayVasi()
    : type(atools::fs::bgl::rw::NONE), pitch(0.0)
  {
  }

  RunwayVasi(atools::io::BinaryStream *bs);

  virtual ~RunwayVasi();

  float getPitch() const
  {
    return pitch;
  }

  atools::fs::bgl::rw::VasiType getType() const
  {
    return type;
  }

  static QString vasiTypeToStr(atools::fs::bgl::rw::VasiType type);

private:
  friend QDebug operator<<(QDebug out, const RunwayVasi& record);

  atools::fs::bgl::rw::VasiType type;
  float pitch;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_RUNWAYVASI_H_ */
