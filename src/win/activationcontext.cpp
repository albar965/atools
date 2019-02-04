/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "win/activationcontext.h"

#include <QDebug>

#if defined(Q_OS_WIN32)
extern "C" {
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
}
#endif

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace atools {
namespace win {

class ActivationContextPrivate
{
public:
#if defined(Q_OS_WIN32)
  DWORD_PTR cookie = 0;
  HANDLE activationContext = 0;
  DWORD lastError = ERROR_SUCCESS;

  QHash<QString, HMODULE> loadedLibraries;

#endif
};

/* Returned value has to be deleted by caller */
wchar_t *asWChar(const QString& str)
{
  wchar_t *wcharb = new wchar_t[str.size() + 1];
  str.toWCharArray(wcharb);
  wcharb[str.size()] = 0;
  return wcharb;
}

ActivationContext::ActivationContext()
  : p(new ActivationContextPrivate())
{

}

ActivationContext::~ActivationContext()
{
#if defined(Q_OS_WIN32)
  for(const QString& key : p->loadedLibraries.keys())
    freeLibrary(key);

  deactivate();
  release();
#endif

  delete p;
}

bool ActivationContext::isAvailable() const
{
#if defined(Q_OS_WIN32)
  return true;

#else
  return false;

#endif
}

bool ActivationContext::create(const QString& manifestPath)
{
  qDebug() << Q_FUNC_INFO << manifestPath;
#if defined(Q_OS_WIN32)
  ACTCTX ctx;
  memset(&ctx, 0, sizeof(ctx));

  wchar_t *manifestWPath = asWChar(manifestPath);

  ctx.cbSize = sizeof(ctx);
  ctx.lpSource = manifestWPath;
  ctx.dwFlags = 0;

  p->activationContext = CreateActCtx(&ctx);
  p->lastError = GetLastError();

  delete[] manifestWPath;

  if(p->activationContext == INVALID_HANDLE_VALUE)
    qWarning() << Q_FUNC_INFO << "Manifest" << manifestPath << "Error" << p->lastError;

  return p->activationContext != INVALID_HANDLE_VALUE;

#else
  return false;

#endif
}

bool ActivationContext::activate()
{
  qDebug() << Q_FUNC_INFO;
#if defined(Q_OS_WIN32)
  bool retval = ActivateActCtx(p->activationContext, &p->cookie);
  p->lastError = GetLastError();

  if(!retval)
    qWarning() << Q_FUNC_INFO << "Error" << p->lastError;

  return retval;

#else
  return false;

#endif
}

bool ActivationContext::deactivate()
{
#if defined(Q_OS_WIN32)
  bool retval = true;
  if(p->cookie != 0)
    retval = DeactivateActCtx(0, p->cookie);
  p->lastError = GetLastError();
  p->cookie = 0;

  if(!retval)
    qWarning() << Q_FUNC_INFO << "Error" << p->lastError;

  return retval;

#else
  return false;

#endif
}

void ActivationContext::release()
{
#if defined(Q_OS_WIN32)
  if(p->activationContext != 0)
    ReleaseActCtx(p->activationContext);
  p->activationContext = 0;
#endif
}

bool ActivationContext::hasError() const
{
#if defined(Q_OS_WIN32)

  return p->lastError != ERROR_SUCCESS;

#else
  return false;

#endif
}

unsigned int ActivationContext::getErrorNumber() const
{
#if defined(Q_OS_WIN32)
  return p->lastError;

#else
  return 0;

#endif
}

bool ActivationContext::loadLibrary(const QString& libraryName)
{
  qDebug() << Q_FUNC_INFO << libraryName;

#if defined(Q_OS_WIN32)
  wchar_t *libraryWPath = asWChar(libraryName);
  HMODULE hLibrary = LoadLibrary(libraryWPath);
  p->lastError = GetLastError();

  if(hLibrary != NULL)
    p->loadedLibraries.insert(libraryName, hLibrary);

  delete[] libraryWPath;

  if(hLibrary == NULL)
    qWarning() << Q_FUNC_INFO << "Library" << libraryName << "Error" << p->lastError;

  return hLibrary != NULL;

#else
  return false;

#endif
}

bool ActivationContext::freeLibrary(const QString& libraryName)
{
#if defined(Q_OS_WIN32)
  bool retval = true;
  if(p->loadedLibraries.contains(libraryName))
  {
    retval = FreeLibrary(p->loadedLibraries.value(libraryName));
    p->loadedLibraries.remove(libraryName);
    p->lastError = GetLastError();

    if(!retval)
      qWarning() << Q_FUNC_INFO << "Library" << libraryName << "Error" << p->lastError;
  }
  else
    qWarning() << Q_FUNC_INFO << "Library" << libraryName << "not found";

  return retval;

#else
  return false;

#endif
}

void *ActivationContext::getProcAddress(const QString& libraryName, const QString& procName)
{
#if defined(Q_OS_WIN32)
  FARPROC ptr = GetProcAddress(p->loadedLibraries.value(libraryName), procName.toLocal8Bit().data());
  p->lastError = GetLastError();

  if(ptr == nullptr)
    qWarning() << Q_FUNC_INFO << "Library" << libraryName << "procedure" << procName << "Error" << p->lastError;

  return (void *)ptr;

#else
  return nullptr;

#endif
}

} // namespace win
} // namespace atools
