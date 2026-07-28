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

#include "PackwizSyncStep.h"

#include "FileSystem.h"
#include "launch/LaunchTask.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/packwiz/PackwizInstallerTask.h"

PackwizSyncStep::PackwizSyncStep(LaunchTask* parent) : LaunchStep(parent) {}

void PackwizSyncStep::executeTask()
{
    auto instance = m_parent->instance();
    auto packTomlUrl = instance->settings()->get("ManagedPackURL").toString().trimmed();

    if (packTomlUrl.isEmpty()) {
        emit logLine(tr("No packwiz pack.toml URL configured for this instance, skipping sync."), MessageLevel::Warning);
        emitSucceeded();
        return;
    }

    emit logLine(tr("Syncing packwiz pack from %1...").arg(packTomlUrl), MessageLevel::Launcher);

    auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());
    m_syncTask = Packwiz::createSyncTask(packTomlUrl, instance->gameRoot(), instance->instanceRoot(), javaPath);
    connect(m_syncTask.get(), &Task::succeeded, this, [this] {
        emit logLine(tr("Packwiz pack synced successfully."), MessageLevel::Launcher);
        emitSucceeded();
    });
    connect(m_syncTask.get(), &Task::failed, this, [this](const QString& reason) {
        // Non-fatal: don't block offline play just because a sync couldn't complete
        emit logLine(tr("Packwiz sync failed, continuing with the existing mods: %1").arg(reason), MessageLevel::Warning);
        emitSucceeded();
    });
    connect(m_syncTask.get(), &Task::aborted, this, &PackwizSyncStep::emitAborted);
    connect(m_syncTask.get(), &Task::progress, this, &Task::setProgress);
    // The per-mod detail (e.g. "Downloaded foo.jar") only ever shows up via stepProgress -
    // createSyncTask()'s SequentialTask's own aggregate status is just "Executing task X out of Y"
    connect(m_syncTask.get(), &Task::stepProgress, this, [this](TaskStepProgress const& progress) {
        if (!progress.status.isEmpty())
            emit logLine(progress.status, MessageLevel::Launcher);
    });

    m_syncTask->start();
}

bool PackwizSyncStep::abort()
{
    if (m_syncTask) {
        return m_syncTask->abort();
    }
    return true;
}
