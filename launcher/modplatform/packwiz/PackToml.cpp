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

#include "PackToml.h"

#include <QDebug>

#include "StringUtils.h"

#include <toml++/toml.h>

namespace Packwiz {

namespace {
// Same loader uid -> display name mapping packwiz-installer's own MultiMC integration uses
// (LauncherUtils.kt: handleMultiMC), kept in display order.
const QList<QPair<QString, QString>> s_loaderDisplayNames = {
    { "forge", "Forge" },       { "neoforge", "NeoForge" },   { "fabric", "Fabric" },
    { "quilt", "Quilt" },       { "liteloader", "LiteLoader" }
};
}  // namespace

QString PackToml::versionSummary() const
{
    QStringList parts;
    if (auto mc = versions.value("minecraft"); !mc.isEmpty())
        parts << mc;

    for (const auto& [key, displayName] : s_loaderDisplayNames) {
        if (auto version = versions.value(key); !version.isEmpty())
            parts << QString("%1 %2").arg(displayName, version);
    }

    return parts.join(", ");
}

std::optional<PackToml> parsePackToml(const QByteArray& data)
{
    toml::table table;
#if TOML_EXCEPTIONS
    try {
        table = toml::parse(std::string_view(data.constData(), data.size()));
    } catch (const toml::parse_error& err) {
        qWarning() << "Could not parse pack.toml:" << QString(err.what());
        return std::nullopt;
    }
#else
    auto result = toml::parse(std::string_view(data.constData(), data.size()));
    if (!result) {
        qWarning() << "Could not parse pack.toml:" << result.error().description();
        return std::nullopt;
    }
    table = result.table();
#endif

    PackToml pack;
    pack.name = table["name"].value_or("");
    pack.packFormat = table["pack-format"].value_or("");

    if (auto index = table["index"].as_table()) {
        pack.indexFile = (*index)["file"].value_or("");
        pack.indexHashFormat = (*index)["hash-format"].value_or("");
        pack.indexHash = (*index)["hash"].value_or("");
    }

    if (auto versions = table["versions"].as_table()) {
        for (auto&& [key, value] : *versions) {
            if (auto str = value.as_string()) {
                pack.versions.insert(QString::fromStdString(std::string(key.str())), QString::fromStdString(str->get()));
            }
        }
    }

    if (!pack.isValid()) {
        qWarning() << "pack.toml is missing required fields (name/index.file)";
        return std::nullopt;
    }

    return pack;
}

}  // namespace Packwiz
