/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "util/version.h"

#include <QDebug>
#include <QRegularExpression>
#include <QApplication>

namespace atools {
namespace util {

static const QRegularExpression VERSION_REGEXP("^(\\d+)\\.(\\d+)\\.(\\d+)"
                                               "[-\\.]?"
                                               "([a-zA-Z_]*)"
                                               "([0-9]*)$");

Version::Version(int verMajor, int verMinor, int verPatchlevel, const QString& verName, int verNameSub)
  : majorVersion(verMajor), minorVersion(verMinor), patchlevelVersion(verPatchlevel), nameSubVersion(verNameSub),
  name(verName)
{
  versionString = QString("%1.%2.%3%4").
                  arg(majorVersion).
                  arg(minorVersion).
                  arg(patchlevelVersion).
                  arg(name.isEmpty() ? QString() : "." + name).
                  arg(nameSubVersion >= 0 ? QString() : QString::number(nameSubVersion));
}

Version::Version(const QString& str)
  : versionString(str.trimmed().toLower())
{
  initFromString(str);
}

Version::Version()
{
  initFromString(QApplication::applicationVersion());
}

Version::~Version()
{

}

void Version::initFromString(const QString& str)
{
  versionString = str;
  if(!str.isEmpty())
  {
    QRegularExpressionMatch match = VERSION_REGEXP.match(str);
    bool ok;

    QStringList captured = match.capturedTexts();
    if(captured.size() < 3)
      qWarning() << "Error reading version information: size is < 3" << str;
    else
    {
      majorVersion = captured.at(1).toInt(&ok);
      if(!ok)
      {
        qWarning() << "Error reading version information: cannot read major" << str;
        majorVersion = -1;
      }

      minorVersion = captured.at(2).toInt(&ok);
      if(!ok)
      {
        qWarning() << "Error reading version information: cannot read minor" << str;
        minorVersion = -1;
      }

      patchlevelVersion = captured.at(3).toInt(&ok);
      if(!ok)
      {
        qWarning() << "Error reading version information: cannot read patchlevel" << str;
        patchlevelVersion = -1;
      }
    }
    if(captured.size() >= 4)
    {
      name = captured.at(4);
      if(namePriority() == -1)
        qWarning() << "Error reading version information: invalid name" << str;
    }

    if(captured.size() >= 5 && !captured.at(5).isEmpty())
    {
      ok = false;
      nameSubVersion = captured.at(5).toInt(&ok);

      if(!ok)
        qWarning() << "Error reading version information: invalid sub version" << str;
    }
  }
  else
    qWarning() << "Empty version";
}

int Version::namePriority() const
{
  if(isDevelop())
    return 0;
  else if(isBeta())
    return 1;
  else if(isReleaseCandidate())
    return 2;
  else if(isStable())
    return 3;

  return -1;
}

bool Version::isStable() const
{
  return name.isEmpty();
}

bool Version::isReleaseCandidate() const
{
  return name.toLower() == "rc";
}

bool Version::isBeta() const
{
  return name.toLower() == "beta";
}

bool Version::isDevelop() const
{
  return name.toLower().startsWith("dev") || name.toLower() == "alpha";
}

bool Version::operator<(const Version& other) const
{
  if(majorVersion != other.majorVersion)
    return majorVersion < other.majorVersion;
  else if(minorVersion != other.minorVersion)
    return minorVersion < other.minorVersion;
  else if(patchlevelVersion != other.patchlevelVersion)
    return patchlevelVersion < other.patchlevelVersion;
  else if(namePriority() != other.namePriority())
    return namePriority() < other.namePriority();
  else if(nameSubVersion != other.nameSubVersion)
    return nameSubVersion < other.nameSubVersion;
  else
    // equal
    return false;
}

bool Version::isValid() const
{
  return majorVersion >= 0 && minorVersion >= 0 && patchlevelVersion >= 0 && namePriority() >= 0;
}

QDebug operator<<(QDebug out, const atools::util::Version& version)
{
  QDebugStateSaver saver(out);
  out.nospace().noquote() << version.versionString;
  return out;
}

} // namespace util
} // namespace atools
