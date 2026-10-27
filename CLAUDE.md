# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

TrinityCore `master` (retail WoW 12.x server emulator, C++20, CMake) in a personal fork (`origin` = PSIFAK/TrinityCore, `upstream` = TrinityCore/TrinityCore). The working branch `feature/retail-systems-12.0.7` adds a fork-only **RetailSystems** layer (Chromie Time, Mythic+/Challenge Mode, BattlePay) on top of upstream and is kept current by merging/rebasing `upstream/master`. Local `master` may be stale — compare against `upstream/master` to see fork-only changes.

## Build

An existing Visual Studio 2026 build tree lives in `build/` (`SCRIPTS=static`, `TOOLS=ON`, `BUILD_TESTING=OFF`). Multi-config generator, so pass `--config`:

```powershell
cmake -S . -B build
cmake --build build --config RelWithDebInfo --target worldserver -j 6
```

Other targets: `bnetserver`, `mapextractor`/`vmap4extractor`/`vmap4assembler`/`mmaps_generator` (with `TOOLS=1`), `tests`.

Key CMake options (`cmake/options.cmake`): `SERVERS`, `TOOLS`, `SCRIPTS` (`none|static|dynamic|minimal-static|minimal-dynamic`; per-module overrides via `SCRIPTS_<MODULE>`), `USE_COREPCH`/`USE_SCRIPTPCH`, `WITH_WARNINGS`, `WITH_WARNINGS_AS_ERRORS`, `WITH_COREDEBUG`, `BUILD_TESTING`. CI (`.github/workflows/`) builds with `-DWITH_WARNINGS=1 -DWITH_WARNINGS_AS_ERRORS=1` on GCC 13 / Clang 17 / MSVC, including a no-PCH build — so code must compile without relying on includes pulled in by precompiled headers.

New source files are picked up by CMake globbing per directory; re-run the configure step after adding files.

On Windows, `worldserver.exe` needs `libmysql.dll` and OpenSSL DLLs on `PATH` or next to the executable.

## Tests

Catch2 unit tests in `tests/` (`common/`, `game/`), only built with `-DBUILD_TESTING=1` (currently off in `build/`). Coverage is small (config, string conversion, chat command parsing, hyperlinks, etc.); most gameplay changes are verified by building and running the server.

```powershell
cmake -S . -B build -DBUILD_TESTING=1
cmake --build build --config RelWithDebInfo --target tests
ctest --test-dir build -C RelWithDebInfo                 # all
ctest --test-dir build -C RelWithDebInfo -R "<test name>" # single test (catch_discover_tests registers each TEST_CASE)
```

Or run the `tests` executable directly with a Catch2 test name/tag filter.

## Architecture

- `src/common` — shared utilities (logging, config, threading, crypto, networking primitives, collision/VMAP, DataStores readers).
- `src/server/database` — async MySQL layer. `Database/Implementation/{Login,Character,World,Hotfix}Database.*` define prepared statements as enum IDs; new queries are added there, not as raw strings. `Updater/` is the DB auto-updater that applies SQL from directories registered in each DB's `updates_include` table.
- `src/server/shared` — code shared by both servers (packet framing, realm list, DB2 file loading).
- `src/server/proto` — protobuf definitions for the Battle.net protocol.
- `src/server/bnetserver` — Battle.net login server. `src/server/worldserver` — world server `main`, CLI, RA.
- `src/server/game` — the game library. Notable pieces:
  - `Server/WorldSession.*` + `Server/Protocol/Opcodes.cpp` — opcode → `WorldSession::Handle*` dispatch table (`DEFINE_HANDLER`). Handlers live in `Handlers/`; packet structs in `Server/Packets/` (`WorldPackets::*`, `Read()`/`Write()`).
  - `Entities/` — `Object` → `WorldObject` → `Unit` → `Player`/`Creature`…; update fields in `Entities/Object/Updates/UpdateFields.*` (generated from the client, don't hand-reorder).
  - `DataStores/DB2Stores.*`, `DB2Structure.h`, `DB2LoadInfo.h` — client DB2 data; `Globals/ObjectMgr` — world DB data.
  - `Spells/`, `Spells/Auras/` — spell system; `Scripting/ScriptMgr.*` — script hook interfaces.
  - `Maps/`, `Grids/` — map instances and grid-based object storage; `Instances/`, `Scenarios/` — dungeon/scenario state.
- `src/server/scripts` — content scripts grouped by expansion/zone plus `Spells/`, `Commands/`, `Custom/`. Each module has a `*_script_loader.cpp`; each script file exposes `void AddSC_<name>()` which must be declared and called from the loader.
- `src/tools` — client data extractors and mmap generator.
- `sql/base` — full schema for auth/characters (keep in sync when changing those DBs); `sql/updates/<db>/master/` — incremental upstream updates; `sql/custom/<db>/` — local-only updates (registered in `updates_include`, applied by the auto-updater); `sql/old` — archived.

## RetailSystems (fork-only layer)

See `src/server/game/Custom/RetailSystems/README.md` (Russian) for setup and scope. Targets client 12.0.7.68453.

- Core: `src/server/game/Custom/RetailSystems/{BattlePay,ChallengeMode,ChromieTime,DB2}` — each system is `*Packets`, `*Handler` (WorldSession handlers), `*Service`. `DB2/` loads extra DB2s and GameTables directly from mapextractor output (`RetailSystems.ClientDataDir`), without touching the upstream hotfix schema.
- Scripts: `src/server/scripts/Custom/RetailSystems/retail_systems.cpp`, registered via `AddRetailSystemsScripts()` in `custom_script_loader.cpp`.
- Config: `retail_systems.conf.dist` → `worldserver.conf.d/retail_systems.conf`. BattlePay is disabled by default.
- SQL: `sql/custom/{auth,characters,world}/retail_systems/`, tables prefixed `retail_`.
- Hooks into upstream files are intentionally minimal (opcode registration in `Opcodes.cpp`, `WorldSession.h/.cpp`, `DB2Stores.cpp`, `AuthHandler.cpp`, `CharacterHandler.cpp`, `HotfixHandler.cpp`, `Map.cpp`, `WorldserverServiceDispatcher.cpp`). Keep new logic inside the `Custom/` directories and keep upstream touchpoints small to limit merge conflicts with upstream.

## Conventions

- `.editorconfig`: 4-space indent, no tabs, max line 160, trailing newline; C/C++ sources are **latin1** — avoid non-ASCII characters in `.cpp/.h`.
- Commit subject style follows upstream: `Core/<Area>: ...`, `Scripts/<Zone>: ...`, `DB/<Area>: ...`.
- SQL update file naming: `YYYY_MM_DD_NN_<database>.sql` (e.g. `2026_10_19_00_world.sql`), `NN` = per-day per-database index. Changes to `auth`/`characters` schema must also be reflected in `sql/base/*`.
