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

#ifndef ATOOLS_GPXIO_H
#define ATOOLS_GPXIO_H

#include <QCoreApplication>

class QXmlStreamReader;
class QXmlStreamWriter;
class QTextStream;

namespace atools {

namespace fs {
}
namespace util {
class XmlStream;
}
namespace geo {

class PosD;
class LineString;
class Pos;
}

namespace fs {
namespace gpx {

class GpxData;

/*
 * Collects all save and load methods for GPX files.
 * Stateless except filename for error reporting.
 */
class GpxIO
{
  Q_DECLARE_TR_FUNCTIONS(GpxIO)

public:
  GpxIO();
  ~GpxIO();

  /* true if file header looks like a GPX file */
  static bool isGpxFile(const QString& file);

  /* GPX format including track and time stamps. */
  void saveGpx(const QString& filename, const atools::fs::gpx::GpxData& gpxData);

  /* Same as above but returns the file in a string */
  QString saveGpxStr(const atools::fs::gpx::GpxData& gpxData);

  /* Same as above but returns the file in a Gzip compressed byte array */
  QByteArray saveGpxGz(const atools::fs::gpx::GpxData& gpxData);

  /* Loads GPX route coordinates and track points into LineStrings.
   * Reading is limited to files exported by this class.
   * track, route and routenames can be null and will be ignored then */
  void loadGpxStr(atools::fs::gpx::GpxData& gpxData, const QString& string);
  void loadGpxGz(atools::fs::gpx::GpxData& gpxData, const QByteArray& bytes);
  void loadGpx(atools::fs::gpx::GpxData& gpxData, const QString& filename);

private:
  void saveGpxInternal(QXmlStreamWriter& writer, const atools::fs::gpx::GpxData& gpxData);
  void loadGpxInternal(atools::fs::gpx::GpxData& gpxData, util::XmlStream& xmlStream);
  void readPosGpx(atools::geo::PosD& pos, QString& name, util::XmlStream& xmlStream, QDateTime *timestamp = nullptr);

  QString errorMsg;
};

} // namespace gpx
} // namespace fs
} // namespace atools

#endif // ATOOLS_GPXIO_H
