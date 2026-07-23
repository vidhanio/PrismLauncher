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

#include "InstanceCreationTask.h"

namespace Packwiz {

// Creates a brand new instance from a packwiz pack.toml URL. Nested inside InstanceImportTask
// (see InstanceImportTask::processPackwiz()), the same way FlameCreationTask/ModrinthCreationTask
// are - hence taking stagingPath/globalSettings explicitly rather than relying on the generic
// InstanceStaging wrapper, which only ever wraps the top-level InstanceImportTask.
//
// Updating an existing packwiz-managed instance does NOT go through this class - see
// createSyncTask() in PackwizInstallerTask.h, which re-runs packwiz-installer directly against the
// live instance so its own incremental sync state (packwiz.json) is preserved, rather than
// restaging into a fresh temporary directory the way Modrinth/CurseForge pack updates do.
class CreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    CreationTask(const QString& stagingPath, SettingsObject* globalSettings, QString packTomlUrl, QString packName, QString packVersion);

    bool abort() override;
    std::unique_ptr<MinecraftInstance> createInstance() override;

   private:
    QString m_packTomlUrl;
    QString m_packName;
    QString m_packVersion;

    Task::Ptr m_syncTask;
};

}  // namespace Packwiz
