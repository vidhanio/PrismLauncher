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

#pragma once

#include <QString>
#include <memory>

#include "MessageLevel.h"
#include "QObjectPtr.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class LoggedProcess;

namespace Packwiz {

// Runs packwiz-installer-bootstrap.jar (downloaded/cached on first use) against a pack.toml URL,
// syncing an instance's mods/resourcepacks/etc. and, since Prism is a MultiMC-format launcher,
// letting packwiz-installer's own MultiMC integration update mmc-pack.json's Minecraft/loader
// components directly. Prism does not re-implement any of packwiz's sync/diffing logic itself.
class InstallerTask : public Task {
    Q_OBJECT

   public:
    // gameRoot is the instance's ".minecraft" folder (packwiz's --pack-folder);
    // instanceRoot is the instance's root folder containing mmc-pack.json (packwiz's --multimc-folder)
    InstallerTask(QString packTomlUrl, QString gameRoot, QString instanceRoot);
    ~InstallerTask() override = default;

    bool canAbort() const override { return true; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void fetchBootstrapJar();
    void runInstaller();
    QString cacheDir() const;

   private:
    QString m_packTomlUrl;
    QString m_gameRoot;
    QString m_instanceRoot;

    NetJob::Ptr m_bootstrapDownloadJob;
    std::shared_ptr<LoggedProcess> m_process;
};

// Builds the run-the-installer + tag-the-results task pair shared by instance creation, the
// automatic pre-launch sync step, and the manual "Sync Now" button - all three just point it at
// different (gameRoot, instanceRoot) directories.
Task::Ptr createSyncTask(const QString& packTomlUrl, const QString& gameRoot, const QString& instanceRoot);

}  // namespace Packwiz
