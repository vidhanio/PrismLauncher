// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Vidhan Bhatt <me@vidhan.io>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PackwizInstallerTask.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include "Application.h"
#include "FileSystem.h"
#include "LoggedProcess.h"
#include "java/JavaUtils.h"
#include "modplatform/packwiz/PackwizMetadataTask.h"
#include "net/Download.h"
#include "tasks/SequentialTask.h"

namespace Packwiz {

namespace {
const QUrl kBootstrapJarUrl{ "https://github.com/packwiz/packwiz-installer-bootstrap/releases/latest/download/packwiz-installer-bootstrap.jar" };
}

InstallerTask::InstallerTask(QString packTomlUrl, QString gameRoot, QString instanceRoot)
    : Task(), m_packTomlUrl(std::move(packTomlUrl)), m_gameRoot(std::move(gameRoot)), m_instanceRoot(std::move(instanceRoot))
{}

QString InstallerTask::cacheDir() const
{
    return FS::PathCombine(APPLICATION->dataRoot(), "packwiz");
}

void InstallerTask::executeTask()
{
    FS::ensureFolderPathExists(cacheDir());
    auto bootstrapPath = FS::PathCombine(cacheDir(), "packwiz-installer-bootstrap.jar");

    if (QFile::exists(bootstrapPath)) {
        runInstaller();
        return;
    }

    fetchBootstrapJar();
}

void InstallerTask::fetchBootstrapJar()
{
    setStatus(tr("Fetching packwiz-installer-bootstrap..."));

    auto bootstrapPath = FS::PathCombine(cacheDir(), "packwiz-installer-bootstrap.jar");

    m_bootstrapDownloadJob = makeShared<NetJob>("Packwiz::FetchBootstrap", APPLICATION->network());
    m_bootstrapDownloadJob->addNetAction(Net::Download::makeFile(kBootstrapJarUrl, bootstrapPath));

    connect(m_bootstrapDownloadJob.get(), &Task::succeeded, this, &InstallerTask::runInstaller);
    connect(m_bootstrapDownloadJob.get(), &Task::failed, this, [this](const QString& reason) {
        emitFailed(tr("Failed to download packwiz-installer-bootstrap: %1").arg(reason));
    });
    connect(m_bootstrapDownloadJob.get(), &Task::progress, this, &Task::setProgress);

    m_bootstrapDownloadJob->start();
}

void InstallerTask::runInstaller()
{
    setStatus(tr("Syncing mods from packwiz pack..."));
    setProgress(0, 0);

    auto bootstrapPath = FS::PathCombine(cacheDir(), "packwiz-installer-bootstrap.jar");
    auto installerJarPath = FS::PathCombine(cacheDir(), "packwiz-installer.jar");

    // The bootstrap tool only needs *some* working JRE to sync files with - it never launches
    // Minecraft itself, so it doesn't need to match whatever Java the instance eventually uses
    auto javaPath = JavaUtils().GetDefaultJava()->path;

    QStringList args{
        "-jar",       bootstrapPath,        "--no-gui",   "--bootstrap-main-jar",
        installerJarPath, "--pack-folder", m_gameRoot, "--multimc-folder", m_instanceRoot,
        "--side",     "client",             m_packTomlUrl,
    };

    m_process = std::make_shared<LoggedProcess>();
    connect(m_process.get(), &LoggedProcess::log, this, [this](QStringList lines, MessageLevel) {
        static const QRegularExpression progressRe(R"(^\((\d+)/(\d+)\)\s*(.*)$)");
        for (const auto& rawLine : lines) {
            auto line = rawLine.trimmed();
            if (line.isEmpty())
                continue;

            if (auto match = progressRe.match(line); match.hasMatch()) {
                setProgress(match.captured(1).toLongLong(), match.captured(2).toLongLong());
                setStatus(match.captured(3));
            } else {
                setStatus(line);
            }
            qDebug() << "[packwiz-installer]" << line;
        }
    });
    connect(m_process.get(), &LoggedProcess::stateChanged, this, [this](LoggedProcess::State state) {
        switch (state) {
            case LoggedProcess::FailedToStart:
                emitFailed(tr("Failed to start Java to run packwiz-installer"));
                break;
            case LoggedProcess::Finished:
                if (m_process->exitCode() == 0) {
                    emitSucceeded();
                } else {
                    emitFailed(tr("packwiz-installer exited with code %1").arg(m_process->exitCode()));
                }
                break;
            case LoggedProcess::Crashed:
                emitFailed(tr("packwiz-installer crashed"));
                break;
            case LoggedProcess::Aborted:
                emitAborted();
                break;
            default:
                break;
        }
    });

    m_process->setProgram(javaPath);
    m_process->setArguments(args);
    m_process->start();
}

bool InstallerTask::abort()
{
    if (m_bootstrapDownloadJob)
        m_bootstrapDownloadJob->abort();
    if (m_process) {
        m_process->kill();
        return true;
    }
    return Task::abort();
}

Task::Ptr createSyncTask(const QString& packTomlUrl, const QString& gameRoot, const QString& instanceRoot)
{
    auto seq = makeShared<SequentialTask>(QObject::tr("Syncing packwiz pack"));
    seq->addTask(makeShared<InstallerTask>(packTomlUrl, gameRoot, instanceRoot));
    seq->addTask(makeShared<MetadataTask>(gameRoot, packTomlUrl));
    return seq;
}

}  // namespace Packwiz
