/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "desktopservices.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DesktopServices::DesktopServices(const WorkspaceSettings& settings,
                                 QWidget* parent) noexcept
  : mSettings(settings), mParent(parent) {
}

DesktopServices::~DesktopServices() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool DesktopServices::openUrl(const QUrl& url) const noexcept {
  if (url.isLocalFile()) {
    return openLocalPath(FilePath(url.toLocalFile()));
  } else {
    return openWebUrl(url);
  }
}

bool DesktopServices::openWebUrl(const QUrl& url) const noexcept {
  showWaitCursor();
  foreach (QString cmd, mSettings.externalWebBrowserCommands.get()) {
    cmd.replace("{{URL}}", url.toString());
    if (QProcess::startDetached(cmd)) {
      qDebug() << "Successfully opened URL with command:" << cmd;
      return true;
    } else {
      qWarning() << "Failed to open URL with command:" << cmd;
    }
  }
  return openUrlFallback(url);
}

bool DesktopServices::openLocalPath(const FilePath& filePath) const noexcept {
  showWaitCursor();
  const QString ext = filePath.getSuffix().toLower();
  if (filePath.isExistingDir()) {
    return openDirectory(filePath);
  } else if (ext == "pdf") {
    return openLocalPathWithCommand(filePath,
                                    mSettings.externalPdfReaderCommands.get());
  } else {
    return openUrlFallback(QUrl::fromLocalFile(filePath.toNative()));
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool DesktopServices::openDirectory(const FilePath& filePath) const noexcept {
  return openLocalPathWithCommand(filePath,
                                  mSettings.externalFileManagerCommands.get());
}

bool DesktopServices::openLocalPathWithCommand(
    const FilePath& filePath, const QStringList& commands) const noexcept {
  const QUrl url = QUrl::fromLocalFile(filePath.toNative());
  foreach (QString cmd, commands) {
    cmd.replace("{{FILEPATH}}", filePath.toNative());
    cmd.replace("{{URL}}", url.toString());
    if (QProcess::startDetached(cmd)) {
      qDebug() << "Successfully opened file or directory with command:" << cmd;
      return true;
    } else {
      qWarning() << "Failed to open file or directory with command:" << cmd;
    }
  }
  return openUrlFallback(url);
}

bool DesktopServices::openUrlFallback(const QUrl& url) const noexcept {
  // Support specifying a custom URL handler application (such as `xdg-open`)
  // since QDesktopServices::openUrl() does not work in any case (observed
  // with Snap packages). See https://bugreports.qt.io/browse/QTBUG-83939.
  static const QString envHandler =
      QString(qgetenv("LIBREPCB_OPEN_URL_HANDLER")).trimmed();

  QString handlerName;
  bool success = false;
  if (!envHandler.isEmpty()) {
    handlerName = envHandler;
    success = QProcess::startDetached(envHandler, {url.toString()});
  } else {
    handlerName = "QDesktopServices";
    success = QDesktopServices::openUrl(url);
  }

  if (success) {
    qInfo().noquote() << QString("Successfully opened URL with %1: \"%2\"")
                             .arg(handlerName)
                             .arg(url.toString());
  } else {
    qCritical().noquote() << QString("Failed to open URL with %1: \"%2\"")
                                 .arg(handlerName)
                                 .arg(url.toString());
  }
  return success;
}

void DesktopServices::showWaitCursor() noexcept {
  // While waiting for an external application to appear, change the cursor
  // to a waiting spinner for a moment to give immediate feedback about the
  // ongoing operation. Since we don't know how long the operation takes,
  // we just use a fixed delay before restoring the normal cursor.
  qApp->setOverrideCursor(Qt::WaitCursor);
  QTimer::singleShot(2000, qApp, []() { qApp->restoreOverrideCursor(); });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
