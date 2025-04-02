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
 * One error message referring to a file and optional line number.
 */
class SceneryFileError
{
public:
  SceneryFileError(const QString& filepathParam, const QString& errorMessageParam, int lineNumParam = 0)
    : filepath(filepathParam), errorMessage(errorMessageParam), lineNum(lineNumParam)
  {
  }

  const QString& getFilepath() const
  {
    return filepath;
  }

  const QString& getErrorMessage() const
  {
    return errorMessage;
  }

  int getLineNum() const
  {
    return lineNum;
  }

private:
  QString filepath, errorMessage;
  int lineNum;
};

/*
 * Scenery errors and file errors for a scenery area.
 */
class SceneryErrors
{
public:
  SceneryErrors()
  {
  }

  SceneryErrors(const scenery::SceneryArea& area, const QString& message, const QList<SceneryFileError>& errors)
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

  bool hasFileOrSceneryErrors() const
  {
    return !fileErrors.isEmpty() || !sceneryErrorsMessages.isEmpty();
  }

  const atools::fs::scenery::SceneryArea& getScenery() const
  {
    return scenery;
  }

  const QStringList& getSceneryErrorsMessages() const
  {
    return sceneryErrorsMessages;
  }

  bool isWarning() const
  {
    return warning;
  }

  const QList<atools::fs::SceneryFileError>& getFileErrors() const
  {
    return fileErrors;
  }

  void appendFileError(const atools::fs::SceneryFileError& fileError)
  {
    fileErrors.append(fileError);
  }

  void appendSceneryErrorMessage(const QString& message)
  {
    sceneryErrorsMessages.append(message);
  }

  void appendSceneryErrorMessages(const QStringList& messages)
  {
    sceneryErrorsMessages.append(messages);
  }

  void setSceneryArea(const atools::fs::scenery::SceneryArea& newScenery)
  {
    scenery = newScenery;
  }

private:
  friend class NavDatabaseErrors;

  atools::fs::scenery::SceneryArea scenery;
  QStringList sceneryErrorsMessages;
  bool warning = false;
  QList<atools::fs::SceneryFileError> fileErrors;
};

/*
 * This class collects exception messages for each file and scenery database entry.
 */
class NavDatabaseErrors
{
public:
  /* Get total number of errors across all scenery areas */
  int getTotal() const;
  int getTotalErrors() const;
  int getTotalWarnings() const;

  /* Initialize with a single area */
  void init(const scenery::SceneryArea& area);

  void appendSceneryErrors(const atools::fs::SceneryErrors& errors)
  {
    sceneryErrors.append(errors);
  }

  const QList<atools::fs::SceneryErrors>& getSceneryErrors() const
  {
    return sceneryErrors;
  }

  QList<atools::fs::SceneryErrors>& getSceneryErrors()
  {
    return sceneryErrors;
  }

private:
  QList<atools::fs::SceneryErrors> sceneryErrors;
};

} // namespace fs
} // namespace atools

#endif // ATOOLS_FS_NAVDATABASEERRORS_H
