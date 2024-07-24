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

#include "gui/application.h"

#include "atools.h"
#include "gui/dialog.h"
#include "gui/messagebox.h"
#include "gui/tools.h"
#include "io/fileroller.h"
#include "util/crashhandler.h"
#include "util/htmlbuilder.h"
#include "util/properties.h"
#include "zip/zipwriter.h"

#include <cstdlib>
#include <QDebug>
#include <QUrl>
#include <QThread>
#include <QTimer>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QSplashScreen>
#include <QMainWindow>

namespace atools {
namespace gui {

QHash<QString, QStringList> Application::reportFiles;
QStringList Application::emailAddresses;
QString Application::contactUrl;
QSet<QObject *> Application::tooltipExceptions;
bool Application::showSplash = true;
bool Application::shuttingDown = false;

QString Application::lockFile;
bool Application::safeMode = false;

bool Application::showExceptionDialog = true;
bool Application::restartProcess = false;
bool Application::tooltipsDisabled = false;
QSplashScreen *Application::splashScreen = nullptr;

atools::util::Properties *Application::startupOptions = nullptr;

Application::Application(int& argc, char **argv, int)
  : QApplication(argc, argv)
{
  startupOptions = new atools::util::Properties;

  // Needed to catch program stopping during Windows shutdown
  connect(this, &QCoreApplication::aboutToQuit, recordExit);

}

Application::~Application()
{
  ATOOLS_DELETE(splashScreen);

  if(restartProcess)
  {
    qDebug() << Q_FUNC_INFO << "Starting" << QCoreApplication::applicationFilePath();
    restartProcess = false;
    bool result = QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    if(result)
      qInfo() << Q_FUNC_INFO << "Success.";
    else
      qWarning() << Q_FUNC_INFO << "FAILED.";
  }
  ATOOLS_DELETE(startupOptions);
}

bool Application::isShuttingDown()
{
  return shuttingDown;
}

void Application::setShuttingDown(bool value)
{
  qDebug() << Q_FUNC_INFO << value;
  shuttingDown = value;
}

const atools::util::Properties& Application::getStartupOptionsConst()
{
  return *startupOptions;
}

bool Application::hasStartupOption(const QString& key)
{
  return startupOptions->contains(key);
}

QString Application::getStartupOptionStr(const QString& key)
{
  return startupOptions->getPropertyStr(key);
}

QStringList Application::getStartupOptionStrList(const QString& key)
{
  return startupOptions->getPropertyStrList(key);
}

void Application::addStartupOptionStr(const QString& key, const QString& value)
{
  startupOptions->setPropertyStr(key, value);
}

void Application::addStartupOptionStrIf(const QString& key, const QString& value)
{
  if(!value.isEmpty())
    startupOptions->setPropertyStr(key, value);
}

void Application::addStartupOptionBoolIf(const QString& key, bool value)
{
  if(value)
    startupOptions->setPropertyStr(key, QString());
}

void Application::addStartupOptionStrList(const QString& key, const QStringList& value)
{
  startupOptions->setPropertyStrList(key, value);
}

void Application::clearStartupOptions()
{
  startupOptions->clear();
}

Application *Application::applicationInstance()
{
  return dynamic_cast<Application *>(QCoreApplication::instance());
}

void Application::createReport(QWidget *parent, const QString& crashReportFile, const QStringList& filenames,
                               const QString& helpOnlineUrl, const QString& helpDocument, const QString& helpLanguageOnline)
{
  qDebug() << Q_FUNC_INFO << crashReportFile << filenames;

  // Build a report with all relevant files in a Zip archive
  qWarning() << Q_FUNC_INFO << "Creating crash report" << crashReportFile << filenames;
  buildCrashReport(crashReportFile, filenames);

  QString message = tr("<p>An issue report was generated and saved with all related files in a Zip archive.</p>"
                         "<p>%1&nbsp;(click to show)</p>"
                           "<p>You can send this file to the author of %2 to investigate a problem. "
                             "This is a Zip-file and you can look into the contents if needed.</p>"
                             "<p><b>Please make sure you are using the latest version of %2 before reporting a problem,<br/>"
                             "and if possible, describe all the steps to reproduce the problem.</b></p>"
                             "<p><a href=\"%3\"><b>Click here for contact information</b></a></p>").
                    arg(atools::util::HtmlBuilder::aFilePath(crashReportFile, atools::util::html::NOBR_WHITESPACE)).
                    arg(QCoreApplication::applicationName()).arg(contactUrl);

  atools::gui::MessageBox box(parent);
  box.setShowInFileManager();
  box.setHelpUrl(helpOnlineUrl + helpDocument, helpLanguageOnline);
  box.addAcceptButton(QDialogButtonBox::Ok);
  box.setMessage(message);
  box.setIcon(QMessageBox::Information);
  box.exec();
}

void Application::recordStartAndDetectCrash(QWidget *parent, const QString& lockFileParam, const QString& crashReportFile,
                                            const QStringList& filenames,
                                            const QString& helpOnlineUrl, const QString& helpDocument, const QString& helpLanguageOnline)
{
  qDebug() << Q_FUNC_INFO << "Lock file" << lockFileParam;

  // Check if lock file exists - no for last clean exit and yes for crash
  int result = QMessageBox::No;
  if(QFile::exists(lockFileParam))
  {
    qWarning() << Q_FUNC_INFO << "Found previous crash";

    // Do not open splash to avoid dialog re-appearing
    showSplash = false;

    // Build a report with all relevant files in a Zip archive
    qWarning() << Q_FUNC_INFO << "Creating crash report" << crashReportFile << filenames;
    buildCrashReport(crashReportFile, filenames);

    QString message = tr("<p><b>%1 did not exit cleanly the last time.</b></p>"
                           "<p>This was most likely caused by a crash.</p>"
                             "<p>A crash report was generated and saved with all related files in a Zip archive.</p>"
                               "<p>%2&nbsp;(click to show)</p>"
                                 "<p>You might want to send this file to the author of %1 to investigate the crash.</p>"
                                   "<p><b>Please make sure to use the latest version of %1 before reporting a crash and "
                                     "describe all steps to reproduce the problem.</b></p>"
                                     "<p><a href=\"%3\"><b>Click here for contact information</b></a></p>"
                                       "<hr/>"
                                       "<p><b>Start in safe mode now which means to skip loading of all default files like "
                                         "flight plans, window layout and other settings now which may have "
                                         "caused the previous crash?</b></p>").
                      arg(applicationName()).
                      arg(atools::util::HtmlBuilder::aFilePath(crashReportFile, atools::util::html::NOBR_WHITESPACE)).
                      arg(contactUrl);

    atools::gui::MessageBox box(parent);
    box.setShowInFileManager();

    if(!helpOnlineUrl.isEmpty() && !helpDocument.isEmpty())
      box.setHelpUrl(helpOnlineUrl + helpDocument, helpLanguageOnline);

    box.addRejectButton(QDialogButtonBox::No);
    box.addAcceptButton(QDialogButtonBox::Yes);
    box.setMessage(message);
    box.setIcon(QMessageBox::Critical);
    result = box.exec();
  }

  // Remember lock file and write PID into it (PID not used yet)
  lockFile = lockFileParam;
  atools::strToFile(lockFile, QString::number(applicationPid()));

  // Switch to safe mode to avoid loading any files if user selected this
  safeMode = result == QMessageBox::Yes;

  if(safeMode)
    qWarning() << Q_FUNC_INFO << "Starting safe mode";
}

void Application::recordExit()
{
#ifndef DEBUG_DISABLE_CRASH_REPORT
  if(lockFile.isEmpty())
    qInfo() << Q_FUNC_INFO << "No lock file found";
  else if(QFile::remove(lockFile))
    qInfo() << Q_FUNC_INFO << "Success removing lock file" << lockFile;
  else
    qWarning() << Q_FUNC_INFO << "Failed removing lock file" << lockFile;

  lockFile.clear();

#endif
}

void Application::buildCrashReport(const QString& crashReportFile, const QStringList& filenames)
{
  // Create path if missing
  QDir().mkpath(QFileInfo(crashReportFile).absolutePath());

  // Roll files over and keep three copies: little_navmap_crashreport_1.zip little_navmap_crashreport_2.zip little_navmap_crashreport.zip
  // Keep zip extension
  atools::io::FileRoller(3, "${base}_${num}.${ext}", false /* keepOriginalFile */).rollFile(crashReportFile);

  zip::ZipWriter zipWriter(crashReportFile);
  zipWriter.setCompressionPolicy(zip::ZipWriter::AlwaysCompress);

  // Add files to zip if they exist - ignore original path
  for(const QString& str : filenames)
  {
    QFile file(str);
    if(atools::checkFile(Q_FUNC_INFO, file))
    {
      // Get plain name from file - QFile returns full path
      zipWriter.addFile(QFileInfo(file.fileName()).fileName(), &file);
      if(zipWriter.status() != zip::ZipWriter::NoError)
        qWarning() << Q_FUNC_INFO << "Error adding" << file << "to" << crashReportFile << "status" << zipWriter.status();
    }
  }

  zipWriter.close();
  if(zipWriter.status() != zip::ZipWriter::NoError)
    qWarning() << Q_FUNC_INFO << "Error closing" << crashReportFile << "status" << zipWriter.status();
}

bool Application::notify(QObject *receiver, QEvent *event)
{
#ifndef DEBUG_NO_APPLICATION_EXCEPTION
  try
  {
#endif
  if(tooltipsDisabled && (event->type() == QEvent::ToolTip || event->type() == QEvent::ToolTipChange) &&
     !tooltipExceptions.contains(receiver))
    return false;
  else
    return QApplication::notify(receiver, event);

#ifndef DEBUG_NO_APPLICATION_EXCEPTION
}
catch(std::exception& e)
{
  qCritical() << "receiver" << (receiver == nullptr ? "null" : receiver->objectName());
  qCritical() << "event" << (event == nullptr ? 0 : static_cast<int>(event->type()));

  ATOOLS_PRINT_STACK_CRITICAL("Caught exception in event loop handler");
  ATOOLS_HANDLE_EXCEPTION(e);
}
catch(...)
{
  qCritical() << "receiver" << (receiver == nullptr ? "null" : receiver->objectName());
  qCritical() << "event" << (event == nullptr ? 0 : static_cast<int>(event->type()));

  ATOOLS_PRINT_STACK_CRITICAL("Caught unknown exception in event loop handler");
  ATOOLS_HANDLE_UNKNOWN_EXCEPTION;
}
#endif
}

QString Application::generalErrorMessage()
{
  return tr("<b>If the problem persists or occurs during startup "
              "delete all settings and database files of %1 and try again.</b><br/><br/>"
              "<b>If you wish to report this error attach a text copy or a screenshot of this dialog, "
                "the log file and the configuration files "
                "to your report, add all other available information and send it to "
                "the contact address below.</b><br/>").arg(QCoreApplication::applicationName());
}

void Application::setTooltipsDisabled(const QList<QObject *>& exceptions)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  tooltipExceptions = QSet<QObject *>(exceptions.begin(), exceptions.end());
#else
  tooltipExceptions = exceptions.toSet();
#endif
  tooltipsDisabled = true;
}

void Application::setTooltipsEnabled()
{
  tooltipExceptions.clear();
  tooltipsDisabled = false;
}

void Application::handleException(const char *file, int line, const std::exception& e)
{
  qCritical() << "Caught exception in file" << file << "line" << line << "what" << e.what();

  if(showExceptionDialog)
    atools::gui::Dialog::critical(nullptr,
                                  tr("<b>Caught exception in file \"%1\" line %2.</b>"
                                       "%3"
                                       "%4"
                                       "<hr/>%5"
                                         "<hr/>%6<br/>"
                                         "<b>Press OK to exit application.</b>").
                                  arg(file).
                                  arg(line).
                                  arg(atools::strJoin("<ul><li>", QString(e.what()).split("\n"), "</li><li>", "</li><li>", "</li></ul>")).
                                  arg(generalErrorMessage()).
                                  arg(getContactHtml()).
                                  arg(getReportPathHtml()));

  std::abort();
}

void Application::handleException(const char *file, int line)
{
  qCritical() << "Caught unknown exception in file" << file << "line" << line;

  if(showExceptionDialog)
    atools::gui::Dialog::critical(nullptr,
                                  tr("<b>Caught unknown exception in file %1 line %2.</b><br/><br/>"
                                     "%2"
                                     "<hr/>%4"
                                       "<hr/>%5<br/>"
                                       "<h3>Press OK to exit application.</h3>"
                                     ).
                                  arg(file).arg(line).
                                  arg(generalErrorMessage()).
                                  arg(getContactHtml()).
                                  arg(getReportPathHtml()));

  std::abort();
}

void Application::addReportPath(const QString& header, const QStringList& paths)
{
  reportFiles.insert(header, paths);
}

QString Application::getContactHtml()
{
  return tr("<b>Contact:</b><br/>"
            "<a href=\"%1\">%2 - Contact and Support</a>").arg(contactUrl).arg(applicationName());
}

QString Application::getEmailHtml()
{
  QString mailStr(tr("<b>Contact:</b><br/>"));

  QStringList emails;
  for(const QString& mail : qAsConst(emailAddresses))
    emails.append(QString("<a href=\"mailto:%1\">%1</a>").arg(mail));
  mailStr.append(emails.join(" or "));
  return mailStr;
}

void Application::processEventsExtended(unsigned long milliseconds)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QThread::msleep(milliseconds);
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

QString Application::getReportPathHtml()
{
  // Sort keys to avoid random order
  QList<QString> names = reportFiles.keys();
  std::sort(names.begin(), names.end());

  QString fileStr;
  for(const QString& header : qAsConst(names))
  {
    fileStr.append(tr("<b>%1</b><br/>").arg(header));
    const QStringList paths = reportFiles.value(header);

    for(const QString& path : paths)
      fileStr += tr("<a href=\"%1\">%2</a><br/>").arg(QUrl::fromLocalFile(path).toString()).arg(atools::elideTextShortLeft(path, 80));

    if(header != names.constLast())
      fileStr.append(tr("<br/>"));
  }

  return fileStr;
}

void Application::initSplashScreen(const QString& imageFile, const QString& revision)
{
  qDebug() << Q_FUNC_INFO;

  if(showSplash)
  {
    QPixmap pixmap(imageFile);
    splashScreen = new QSplashScreen(pixmap);
    splashScreen->show();

    processEvents();

#if defined(WINARCH64)
    QString applicationVersion = QApplication::applicationVersion() + tr(" 64-bit");
#elif defined(WINARCH32)
    QString applicationVersion = QApplication::applicationVersion() + tr(" 32-bit");
#else
    QString applicationVersion = QApplication::applicationVersion();
#endif

    splashScreen->showMessage(QObject::tr("Version %5 (revision %6)").
                              arg(applicationVersion).arg(revision),
                              Qt::AlignRight | Qt::AlignBottom, Qt::black);

    processEvents(QEventLoop::ExcludeUserInputEvents);
  }
}

void Application::finishSplashScreen(QMainWindow *mainWindow)
{
  qDebug() << Q_FUNC_INFO;

  if(splashScreen != nullptr)
    splashScreen->finish(mainWindow);
}

void Application::closeSplashScreen()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO;
#endif

  if(splashScreen != nullptr)
    splashScreen->close();
}

} // namespace gui
} // namespace atools
