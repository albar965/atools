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

#include "fs/xp/scenerypacks.h"
#include "atools.h"
#include "exception.h"

#include <QFile>
#include <QFileInfo>

namespace atools {
namespace fs {
namespace xp {

QDebug operator<<(QDebug out, const SceneryPack& type)
{
  QDebugStateSaver saver(out);

  out.nospace() << "SceneryPack["
                << "pathstr " << type.pathstr
                << ", filepath " << type.filepath
                << ", errorText " << type.errorText
                << ", disabled " << type.disabled
                << ", valid " << type.valid
                << ", globalAirports " << type.globalAirports << "]";

  return out;
}

SceneryPacks::SceneryPacks()
{

}

SceneryPacks::~SceneryPacks()
{

}

bool SceneryPacks::exists(const QString& basePath, QStringList& errors, QString& filepath)
{
  filepath = atools::buildPathNoCase({basePath, "Custom Scenery", "scenery_packs.ini"});
  errors.append(atools::checkFileMsg(filepath));
  errors.removeAll(QString());
  return errors.isEmpty();
}

void SceneryPacks::read(const QString& basePath)
{
  entries.clear();
  index.clear();

  // X-Plane 11/Custom Scenery/scenery_packs.ini
  //
  // I
  // 1000 Version
  // SCENERY
  //
  // SCENERY_PACK Custom Scenery/X-Plane Landmarks - Chicago/
  // Listing the pack as SCENERY_PACK_DISABLED disables loading entirely.

  if(QFileInfo::exists(basePath))
  {
    QString filepath = atools::buildPathNoCase({basePath, "Custom Scenery", "scenery_packs.ini"});
    QFile file(filepath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QTextStream stream(&file);
      stream.setEncoding(QStringConverter::Utf8);
      stream.setAutoDetectUnicode(true);

      QString msg(tr("Cannot open file \"%1\". Not a scenery_packs.ini file. %2."));

      // I ================
      QString line = stream.readLine().trimmed();
      if(!(line.compare("I", Qt::CaseInsensitive) == 0 || line.compare("A", Qt::CaseInsensitive) == 0))
        throw atools::Exception(msg.arg(filepath).arg(tr("Intital \"I\" or \"A\" missing")));

      // 1000 Version ================
      line = stream.readLine().trimmed();
      if(line.section(' ', 1, 1).compare("Version", Qt::CaseInsensitive) != 0)
        throw atools::Exception(msg.arg(filepath).arg(tr("\"Version\" missing")));

      bool ok;
      fileVersion = line.section(' ', 0, 0).toInt(&ok);
      if(!ok)
        throw atools::Exception(msg.arg(filepath).arg(tr("Version number not valid")));

      // SCENERY ================
      line = stream.readLine().trimmed();
      if(line.compare("SCENERY", Qt::CaseInsensitive) != 0)
        throw atools::Exception(msg.arg(filepath).arg(tr("\"SCENERY\" missing")));

      // Empty line ================
      line = stream.readLine().trimmed();
      // if(!line.isEmpty())
      // throw atools::Exception(msg.arg(filepath).arg(tr("Empty line after \"SCENERY\" missing")));

      int lineNum = 5;
      while(!stream.atEnd())
      {
        line = stream.readLine().trimmed();
        if(!line.isEmpty())
        {
          SceneryPack pack;
          pack.pathstr = line.section(' ', 1).replace('\\', '/');

          QString key = line.section(' ', 0, 0).toUpper();
          if(key != "SCENERY_PACK" && key != "SCENERY_PACK_DISABLED")
          {
            // Add an entry for each invalid line
            pack.disabled = true;
            pack.valid = false;
            pack.errorText = tr("Invalid entry at line %1 in \"%2\".").arg(lineNum).arg(file.fileName());
            pack.errorLine = lineNum;
            entries.append(pack);
          }
          else
          {
            // Detect Global Airports and mark them
            pack.globalAirports = pack.pathstr.startsWith("Custom Scenery/Global Airports", Qt::CaseInsensitive) || // XP11
                                  pack.pathstr.startsWith("Global Scenery/Global Airports", Qt::CaseInsensitive) || // XP12
                                  pack.pathstr == "*GLOBAL_AIRPORTS*"; // XP12

            if(pack.pathstr == "*GLOBAL_AIRPORTS*")
              pack.pathstr = "Global Scenery/Global Airports";

            // SCENERY_PACK Custom Scenery/X-Plane Landmarks - Chicago/  ================
            pack.disabled = key != "SCENERY_PACK";

            QFileInfo fileinfoBase, fileinfoPath(pack.pathstr);

            if(fileinfoPath.isAbsolute())
              // Use absolute path as given
              fileinfoBase = QFileInfo(atools::buildPathNoCase({fileinfoPath.filePath()}));
            else
              // Use relative path to base directory
              fileinfoBase = QFileInfo(atools::buildPathNoCase({basePath, fileinfoPath.filePath()}));

            // Need to check the real path for existence - empty if it does not exist
            QString canonicalFilePath = atools::canonicalFilePath(fileinfoBase);

            if(!canonicalFilePath.isEmpty())
              fileinfoBase.setFile(canonicalFilePath);

            pack.valid = !canonicalFilePath.isEmpty() && fileinfoBase.exists() && fileinfoBase.isDir();

            // path to apt.dat file
            QFileInfo aptdatFileinfo(atools::buildPathNoCase({fileinfoBase.absoluteFilePath(),
                                                              "Earth nav data", "apt.dat"}));

            if(!pack.valid || (aptdatFileinfo.exists() && aptdatFileinfo.isFile()))
            {
              // If path is valid so far but apt.dat does not exist - ignore silently

              if(!pack.valid)
              {
                // Report only errors on missing base path
                // Path does not exist - use base path for reporting
                pack.filepath = fileinfoBase.filePath();

                if(!fileinfoBase.exists())
                  pack.errorText = tr("\"%1\" at line %2 in \"%3\" does not exist.").
                                   arg(fileinfoBase.filePath()).arg(lineNum).arg(file.fileName());
                else if(!fileinfoBase.isDir())
                  pack.errorText = tr("\"%1\" at line %2 in \"%3\" is not a directory.").
                                   arg(fileinfoBase.filePath()).arg(lineNum).arg(file.fileName());
                else if(!fileinfoBase.isReadable())
                  pack.errorText = tr("\"%1\" at line %2 in \"%3\" is not readable.").
                                   arg(fileinfoBase.filePath()).arg(lineNum).arg(file.fileName());
                pack.errorLine = lineNum;
              }
              else
              {
                // All valid
                pack.filepath = atools::buildPathNoCase({fileinfoBase.absoluteFilePath(), "Earth nav data", "apt.dat"});
                pack.errorLine = -1;

                // Add only to index if path exists
                QString absoluteFilePath = QFileInfo(pack.filepath).absoluteFilePath();
                if(!absoluteFilePath.isEmpty())
                  index.insert(absoluteFilePath, entries.size());
              }

              entries.append(pack);
            }
          }
        }
        lineNum++;
      }

      file.close();
    }
    else
      throw atools::Exception(tr("Cannot open file \"%1\". Reason: %2.").arg(filepath).arg(file.errorString()));
  }
  else
    qWarning() << Q_FUNC_INFO << "File scenery_packs.ini not found in " << basePath;
}

const QVector<SceneryPack>& SceneryPacks::getEntries() const
{
  return entries;
}

const SceneryPack *SceneryPacks::getEntryByPath(const QString& filepath) const
{
  QFileInfo fi(filepath);
  int idx = index.value(fi.absoluteFilePath(), -1);
  return idx >= 0 ? &entries.at(idx) : nullptr;
}

} // namespace xp
} // namespace fs
} // namespace atools
