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

#include "tasks/Task.h"

namespace Packwiz {

// packwiz-installer writes mod files directly to disk - it never goes through Prism's own
// download pipeline, so nothing else would populate the per-file `.pw.toml` metadata that the
// Mods tab's Provider column (and the update-checking guards) rely on. After a successful sync,
// this walks the instance's mods folder and tags every file with `provider = PACKWIZ`, linking it
// back to the source pack.toml. Files the user drops in manually are left untouched (no metadata,
// same as today).
class MetadataTask : public Task {
    Q_OBJECT

   public:
    // gameRoot is the instance's ".minecraft" folder
    MetadataTask(QString gameRoot, QString packTomlUrl);
    ~MetadataTask() override = default;

   protected:
    void executeTask() override;

   private:
    void tagModsFolder();

   private:
    QString m_gameRoot;
    QString m_packTomlUrl;
};

}  // namespace Packwiz
