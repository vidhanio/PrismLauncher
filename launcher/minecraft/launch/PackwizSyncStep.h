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

#include "launch/LaunchStep.h"
#include "tasks/Task.h"

// Re-syncs a packwiz-managed instance against its source pack.toml before every launch - the
// mechanism that keeps such instances "immune" to Prism's own mod update-checking, since they're
// only ever updated by re-running packwiz-installer against the original pack.toml. A sync
// failure (e.g. no network) is logged as a warning and does not block the launch, so offline play
// still works with whatever mods were last successfully synced.
class PackwizSyncStep : public LaunchStep {
    Q_OBJECT
   public:
    explicit PackwizSyncStep(LaunchTask* parent);
    virtual ~PackwizSyncStep() = default;

    virtual void executeTask();
    virtual bool canAbort() const { return true; }
    virtual bool abort();

   private:
    Task::Ptr m_syncTask;
};
