/*
 * DeleteCom.h
 *
 *  Created on: 18.05.2015
 *      Author: alex
 */

#ifndef BGL_AP_DEL_DELETECOM_H_
#define BGL_AP_DEL_DELETECOM_H_

#include "../com.h"

namespace atools {
namespace io {
class BinaryStream;
}
}

namespace atools {
namespace fs {
namespace bgl {

class DeleteCom :
  public BglBase
{
public:
  DeleteCom(atools::io::BinaryStream *bs);
  virtual ~DeleteCom();

private:
  friend QDebug operator<<(QDebug out, const DeleteCom& record);

  atools::fs::bgl::com::ComType type;
  int frequency;
};

} // namespace bgl
} // namespace fs
} // namespace atools

#endif /* BGL_AP_DEL_DELETECOM_H_ */
