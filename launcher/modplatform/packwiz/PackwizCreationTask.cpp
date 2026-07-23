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

#include "PackwizCreationTask.h"

#include <QByteArray>
#include <QEventLoop>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "modplatform/packwiz/PackwizInstallerTask.h"
#include "settings/INISettingsObject.h"

namespace Packwiz {

CreationTask::CreationTask(const QString& stagingPath,
                           SettingsObject* globalSettings,
                           QString packTomlUrl,
                           QString packName,
                           QString packVersion)
    : m_packTomlUrl(std::move(packTomlUrl)), m_packName(std::move(packName)), m_packVersion(std::move(packVersion))
{
    setStagingPath(stagingPath);
    setParentSettings(globalSettings);
}

bool CreationTask::abort()
{
    if (!canAbort())
        return false;
    if (m_syncTask)
        m_syncTask->abort();
    return InstanceCreationTask::abort();
}

std::unique_ptr<MinecraftInstance> CreationTask::createInstance()
{
    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_unique<INISettingsObject>(configPath);
    auto instance = std::make_unique<MinecraftInstance>(m_globalSettings, std::move(instanceSettings), m_stagingPath);

    // Seed an empty component list so packwiz-installer's own MultiMC integration has a file to
    // populate with the Minecraft/loader versions declared in the pack.toml - it silently skips
    // this entirely if the file doesn't already exist
    FS::write(FS::PathCombine(m_stagingPath, "mmc-pack.json"), QByteArray(R"({"formatVersion": 1, "components": []})"));

    instance->setIconKey(m_instIcon != "default" ? m_instIcon : "packwiz");
    instance->setManagedPack("packwiz", "", m_packName, "", m_packVersion);
    instance->settings()->set("ManagedPackURL", m_packTomlUrl);
    instance->setName(name());
    instance->saveNow();

    auto gameRoot = instance->gameRoot();
    FS::ensureFolderPathExists(gameRoot);

    QEventLoop loop;
    bool success = false;

    m_syncTask = createSyncTask(m_packTomlUrl, gameRoot, m_stagingPath);
    connect(m_syncTask.get(), &Task::succeeded, this, [&] {
        success = true;
        loop.quit();
    });
    connect(m_syncTask.get(), &Task::failed, this, [&](const QString& reason) {
        setError(reason);
        loop.quit();
    });
    connect(m_syncTask.get(), &Task::aborted, this, [&] {
        m_abort = true;
        loop.quit();
    });
    connect(m_syncTask.get(), &Task::progress, this, &Task::setProgress);
    connect(m_syncTask.get(), &Task::status, this, &Task::setStatus);

    m_syncTask->start();
    loop.exec();

    if (!success)
        return nullptr;

    return instance;
}

}  // namespace Packwiz
