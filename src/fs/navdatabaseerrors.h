/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef ATOOLS_FS_NAVDATABASEERRORS_H
#define ATOOLS_FS_NAVDATABASEERRORS_H

#include "fs/scenery/sceneryarea.h"

#include <QList>

namespace atools {
namespace fs {

/*
 * This class collects exception messages for each file and scenery database entry.
 */
class NavDatabaseErrors
{
public:
  NavDatabaseErrors()
  {

  }

  /* Get total number of errors across all scenery areas */
  int getTotal() const;
  int getTotalErrors() const;
  int getTotalWarnings() const;

  /* Initialize with a single area */
  void init(const scenery::SceneryArea& area);

  struct SceneryFileError
  {
    QString filepath, errorMessage;
    int lineNum;
  };

  struct SceneryErrors
  {
    SceneryErrors()
    {
    }

    SceneryErrors(const scenery::SceneryArea& area, const QString& message, const QList<NavDatabaseErrors::SceneryFileError>& errors)
      : scenery(area), sceneryErrorsMessages({message}), fileErrors(errors)
    {
    }

    SceneryErrors(const scenery::SceneryArea& area, const QStringList& errors, bool isWarning)
      : scenery(area), sceneryErrorsMessages(errors), warning(isWarning)
    {
    }

    SceneryErrors(const scenery::SceneryArea& area, const QString& message, bool isWarning)
      : scenery(area), sceneryErrorsMessages({message}), warning(isWarning)
    {
    }

    atools::fs::scenery::SceneryArea scenery;
    QStringList sceneryErrorsMessages;
    bool warning = false;
    QList<atools::fs::NavDatabaseErrors::SceneryFileError> fileErrors;
  };

  QList<atools::fs::NavDatabaseErrors::SceneryErrors> sceneryErrors;

};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASEERRORS_H
