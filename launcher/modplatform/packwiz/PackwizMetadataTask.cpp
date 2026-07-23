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

#include "PackwizMetadataTask.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include "FileSystem.h"
#include "minecraft/mod/MetadataHandler.h"
#include "modplatform/ModIndex.h"

namespace Packwiz {

namespace {
// Mirrors Resource::parseFile()'s stripping of the ".disabled" suffix and known archive
// extensions, to get a stable identifier for the file's `.pw.toml` sidecar name.
QString deriveSlug(QString fileName)
{
    if (fileName.endsWith(".disabled"))
        fileName.chop(9);

    for (const QString& ext : { QStringLiteral(".jar"), QStringLiteral(".zip"), QStringLiteral(".nilmod"), QStringLiteral(".litemod") }) {
        if (fileName.endsWith(ext)) {
            fileName.chop(ext.length());
            break;
        }
    }

    return fileName.toLower().replace(QRegularExpression("[^a-z0-9_-]"), "-");
}
}  // namespace

MetadataTask::MetadataTask(QString gameRoot, QString packTomlUrl)
    : Task(), m_gameRoot(std::move(gameRoot)), m_packTomlUrl(std::move(packTomlUrl))
{}

void MetadataTask::executeTask()
{
    setStatus(tr("Tagging synced mods as packwiz-managed..."));
    tagModsFolder();
    emitSucceeded();
}

void MetadataTask::tagModsFolder()
{
    QDir modsDir(FS::PathCombine(m_gameRoot, "mods"));
    if (!modsDir.exists())
        return;

    QDir indexDir(FS::PathCombine(modsDir.absolutePath(), ".index"));

    const auto entries = modsDir.entryInfoList(QDir::Files);
    for (const auto& entry : entries) {
        auto slug = deriveSlug(entry.fileName());
        if (slug.isEmpty())
            continue;

        auto existing = Metadata::get(indexDir, slug);
        if (existing.isValid() && existing.provider != ModPlatform::ResourceProvider::PACKWIZ) {
            // Leave mods with legitimate Modrinth/CurseForge metadata alone - e.g. dropped in
            // manually alongside a packwiz-managed pack
            continue;
        }

        Metadata::ModStruct mod;
        mod.slug = slug;
        mod.name = existing.isValid() ? existing.name : entry.fileName();
        mod.filename = entry.fileName();
        mod.mode = "packwiz";
        mod.provider = ModPlatform::ResourceProvider::PACKWIZ;
        mod.mod_id() = m_packTomlUrl;

        Metadata::update(indexDir, mod);
    }
}

}  // namespace Packwiz
