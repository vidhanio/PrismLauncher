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

#include <QMap>
#include <QString>
#include <optional>

namespace Packwiz {

// Read-only view of the top-level fields of a packwiz `pack.toml` manifest, used purely for display
// (e.g. the New Instance preview and the instance's Packwiz settings tab). The actual mod
// install/sync/diffing logic is handled entirely by packwiz-installer itself; Prism never needs to
// walk the index.toml or per-file entries.
struct PackToml {
    QString name;
    QString packFormat;

    QString indexFile;
    QString indexHashFormat;
    QString indexHash;

    // e.g. { "minecraft": "1.20.1", "fabric": "0.15.7" }
    QMap<QString, QString> versions;

    bool isValid() const { return !name.isEmpty() && !indexFile.isEmpty(); }

    // A short human-readable summary of the versions map, e.g. "1.20.1 with Fabric 0.15.7",
    // suitable for display where a single "pack version" string is expected.
    QString versionSummary() const;
};

// Parses the raw contents of a pack.toml file. Returns std::nullopt on a parse error or if
// required fields are missing.
std::optional<PackToml> parsePackToml(const QByteArray& data);

}  // namespace Packwiz
