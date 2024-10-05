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

#ifndef ATOOLS_GUI_APPLICATION_H
#define ATOOLS_GUI_APPLICATION_H

#include <QApplication>
#include <QElapsedTimer>

/* Use this macro to report an fatal error after an exception including location information */
#define ATOOLS_HANDLE_EXCEPTION(e) (atools::gui::Application::handleException(__FILE__, __LINE__, e))

/* Use this macro to report an fatal error after an unknownexception including location information */
#define ATOOLS_HANDLE_UNKNOWN_EXCEPTION (atools::gui::Application::handleException(__FILE__, __LINE__))

class QSplashScreen;
class QMainWindow;

namespace atools {

namespace util {
class Properties;
}
namespace gui {

/*
 * Adds an exception handler for the event queue and provides methods to handle fatal errors.
 * Catches any exceptions that was thrown in an event handler and shows an error dialog
 * also containing application paths etc.
 */
class Application :
  public QApplication
{
  Q_OBJECT

public:
  Application(int& argc, char **argv, int = ApplicationFlags);
  virtual ~Application() override;

  /* Instance of this or null if not applicable */
  static atools::gui::Application *applicationInstance();

  /* Creates a lock file and shows a warning dialog if this is already present from a former crash.
   * Sets safe mode if user chooses to skip file loading.
   * Always creates a crash report in case of previous unsafe exit. */
  static void recordStartAndDetectCrash(QWidget *parent, const QString& lockFileParam, const QString& crashReportFile,
                                        const QStringList& filenames, const QString& helpOnlineUrl = QString(),
                                        const QString& helpDocument = QString(), const QString& helpLanguageOnline = QString());

  /* Removes lock file. Has to be called by application. */
  static void recordExit();

  /* Record files and pack them into a zip for a crash report */
  static void buildCrashReport(const QString& crashReportFile, const QStringList& filenames);

  /* Record files and pack them into a zip for a crash report */
  static void createReport(QWidget *parent, const QString& crashReportFile, const QStringList& filenames, const QString& helpOnlineUrl,
                           const QString& helpDocument, const QString& helpLanguageOnline);

  /* Init and open with given resource */
  static void initSplashScreen(const QString& imageFile, const QString& revision);

  /* Called by main application once startup is done */
  static void finishSplashScreen(QMainWindow *mainWindow);

  /* Remove splash when showing error messages, etc. to avoid overlay */
  static void closeSplashScreen();

  /* User decided to skip loading of files due to earlier crash */
  static bool isSafeMode()
  {
    return safeMode;
  }

  /*
   * Shows an error dialog with the exception message and after that exits the application with code 1.
   *
   * Call this only on the main thread.
   */
  Q_NORETURN static void handleException(const char *file, int line, const std::exception& e);

  /*
   * Shows an error dialog and after that exits the application with code 1.
   *
   * Call this only on the main thread.
   */
  Q_NORETURN static void handleException(const char *file, int line);

  /* Add a list of paths that will be added as links in any error dialog.
   * @param header Header for the list of paths
   * @param paths A list of directory or file paths
   */
  static void addReportPath(const QString& header, const QStringList& paths);

  /*
   * Get the paths that were added using addReportPath in a HTML formatted text
   * as links
   */
  static QString getReportPathHtml();

  static QString getEmailHtml();
  static QString getContactHtml();

  static QStringList getEmailAddresses()
  {
    return emailAddresses;
  }

  static void setEmailAddresses(const QStringList& value)
  {
    emailAddresses = value;
  }

  static void setContactUrl(const QString& value)
  {
    contactUrl = value;
  }

  /* Process twice and wait 10 ms inbetween */
  static void processEventsExtended(unsigned long milliseconds = 10L);

  static QString generalErrorMessage();

  static bool isShowExceptionDialog()
  {
    return showExceptionDialog;
  }

  /* Shows a dialog for exceptions. Otherwise just logs a message and exits with return code 1. */
  static void setShowExceptionDialog(bool value)
  {
    showExceptionDialog = value;
  }

  static bool isRestartProcess()
  {
    return restartProcess;
  }

  /* Set to true to restart the same process with the same arguments after shutdown.
   *  Useful for settings reset. */
  static void setRestartProcess(bool value)
  {
    restartProcess = value;
  }

  /* true if display of tooltips is disabled for the whole application */
  static bool isTooltipsDisabled()
  {
    return tooltipsDisabled;
  }

  /* Disable display of tooltips for all widgets except the ones given in the exception list */
  static void setTooltipsDisabled(const QList<QObject *>& exceptions = {});

  /* Enable display of tooltips again */
  static void setTooltipsEnabled();

  /* Main window destructor called - appears later than isCloseCalled() */
  static bool isShuttingDown();
  static void setShuttingDown(bool value = true);

  /* Command line options. Have to be populated by the application. */
  static const atools::util::Properties& getStartupOptionsConst();
  static bool hasStartupOption(const QString& key);
  static QString getStartupOptionStr(const QString& key);
  static QStringList getStartupOptionStrList(const QString& key);
  static void addStartupOptionStr(const QString& key, const QString& value);
  static void addStartupOptionStrIf(const QString& key, const QString& value); /* Add option only if value is not empty */
  static void addStartupOptionBoolIf(const QString& key, bool value); /* Add option depending on value */
  static void addStartupOptionStrList(const QString& key, const QStringList& value);
  static void clearStartupOptions(); /* Clear for safe mode */

  /* Note on startup and print time into log */
  static void startup();
  static void startupFinished(const char *func);

signals:
  /* Called by queued connection from QGuiApplication::commitDataRequest */
  void applicationAboutToQuit();

private:
  virtual bool notify(QObject *receiver, QEvent *event) override;
  void applicationAboutToQuitInternal();

  static QHash<QString, QStringList> reportFiles;

  static QSet<QObject *> tooltipExceptions;

  static QStringList emailAddresses;
  static QString contactUrl;

  static QSplashScreen *splashScreen;
  static bool showSplash;

  static QString lockFile;
  static bool safeMode;

  static atools::util::Properties *startupOptions;

  static bool showExceptionDialog, restartProcess, tooltipsDisabled;

  static bool shuttingDown;

  static QElapsedTimer timer;

};

} // namespace gui
} // namespace atools

#endif // ATOOLS_GUI_APPLICATION_H
