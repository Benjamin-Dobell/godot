# Breaka V8 Linux Godot Builder

This folder builds the local Godot 4.7dev + GodotJS source tree in Docker for
Breaka’s Linux V8 RPG export. It never selects GodotJSC, QuickJS, or QuickJS-NG.

The builder follows the GodotJS Linux workflow: Ubuntu 22.04, Node 24, SCons
4.10.1, a prebuilt GodotJS TypeScript runtime, and `platform=linuxbsd`.
Its runtime-bundle stage specifically mirrors
`modules/GodotJS/.github/workflows/misc_environment.yml`: from the GodotJS
checkout it runs `pnpm install`, then `pnpm build`, and requires the resulting
`scripts/out` runtime/editor bundles before SCons starts.

## Build AMD64 export templates

```sh
./breaka-v8-builder/build.sh \
  --kind template \
  --platform linux/amd64 \
  --install-template
```

The template build emits both release and debug templates, named for Godot’s
standard discovery contract:

```text
breaka-v8-builder/.output/templates/<template-resolver-version>/linux_release.x86_64
breaka-v8-builder/.output/templates/<template-resolver-version>/linux_debug.x86_64
```

`--install-template` copies them to the host’s standard Godot template
directory. On macOS that is:

```text
~/Library/Application Support/Godot/export_templates/<template-resolver-version>/
```

By default, `<template-resolver-version>` is derived from the selected source
checkout's `version.py` (for the current source, `4.7.dev`). This is the
directory name Godot's export resolver uses. It intentionally does not use the
editor's display-only custom-build suffix; pass `--template-version` only when
an export resolver explicitly requires a different directory.

The wrapper refuses to overwrite a non-empty artifact directory. Use a fresh
`--out` path for a new build.

## Clean, revision-pinned source context

For a reproducible build, point `--source` at a clean Godot checkout with its
clean GodotJS checkout at `modules/GodotJS`, rather than using a developer's
dirty working tree:

```sh
./breaka-v8-builder/build.sh \
  --source /path/to/clean-godot \
  --kind template \
  --platform linux/amd64 \
  --out /tmp/breaka-v8-templates \
  --install-template
```

The source context must contain the reviewed `.dockerignore` that preserves
`platform/android/api/**`; those platform-neutral Java API stubs are required
by the native build even when Android is not being built.

The SCons invocation follows the GodotJS Linux workflow's explicit targets,
cache, strict checks, and generated `scripts/out` artifact contract. Its
server-template deviations are `module_openxr_enabled=no` and `udev=no`:
the dedicated headless Linux V8 export has no XR/VR surface and does not need
SDL/libudev gamepad-connection callbacks. `udev=no` removes that compiled
libudev path rather than applying a runtime environment hint. Use this output
only through an isolated server export selection; preserve generic client
templates. The builder does not patch Godot, GodotJS, or OpenXR source to make
these selections.

## Build an editor

```sh
./breaka-v8-builder/build.sh \
  --kind editor \
  --platform linux/amd64 \
  --out /tmp/breaka-godot-v8-editor
```

## Architecture selection

`--platform` chooses Docker’s target architecture and the Dockerfile derives a
matching Godot architecture (`linux/amd64` -> `x86_64`, `linux/arm64` ->
`arm64`). `--arch` can state that choice explicitly. Deliberate cross-compiles
are rejected because the local Linux build configuration does not provide a
cross-toolchain; use a matching native/emulated Docker platform instead.

Each artifact directory contains `BUILD-INFO`, including the generated
GodotJS engine macros. A valid V8 build records `JSB_WITH_V8 1` and both
QuickJS/JSC selections as disabled.
