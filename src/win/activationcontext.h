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

#ifndef ATOOLS_WIN_ACTIVATIONCONTEXT_H
#define ATOOLS_WIN_ACTIVATIONCONTEXT_H

#include <QString>

namespace atools {
namespace win {

class ActivationContextPrivate;

/* Wraps the Windows SxS (Side-by-Side) dynamic loading mechanism and provides an empty shell for non
 * windows systems */
class ActivationContext
{
public:
  ActivationContext();
  ~ActivationContext();

  /* true if this is Windows */
  bool isAvailable() const;

  /* Create context. true if successfull */
  bool create(const QString& manifestPath);

  /* Releases a created context */
  void release();

  /* Activates the previously created context. true if successfull */
  bool activate();

  /* Deactivates an activated context */
  bool deactivate();

  /* true if the previous operation caused an error */
  bool hasError() const;

  /* Windows error number or null */
  unsigned int getErrorNumber() const;

  /* Load a DLL by name */
  bool loadLibrary(const QString& libraryName);
  bool freeLibrary(const QString& libraryName);

  /* Retrieves the address of an exported function or variable from the specified dynamic-link library (DLL).
   *  DLL "libraryName" has to be loaded before */
  void *getProcAddress(const QString& libraryName, const QString& procName);

private:
  ActivationContextPrivate *p = nullptr;
};

} // namespace win
} // namespace atools

#endif // ATOOLS_WIN_ACTIVATIONCONTEXT_H
