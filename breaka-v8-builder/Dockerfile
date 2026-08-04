# syntax=docker/dockerfile:1.7

# The compiler stays on Ubuntu 22.04 to match the GodotJS Linux workflow.
FROM node:24-bookworm-slim AS node-runtime

FROM ubuntu:22.04 AS builder

ARG TARGETARCH
ARG GODOT_ARCH=""
ARG GODOT_BUILD=template
ARG GODOT_TEMPLATE_VERSION=4.7.dev
ARG SCONS_JOBS=0

ENV DEBIAN_FRONTEND=noninteractive
ENV COREPACK_ENABLE_DOWNLOAD_PROMPT=0

COPY --from=node-runtime /usr/local/bin/node /usr/local/bin/node
COPY --from=node-runtime /usr/local/lib/node_modules /usr/local/lib/node_modules

RUN ln -s /usr/local/bin/node /usr/local/bin/nodejs \
    && ln -s /usr/local/lib/node_modules/corepack/dist/corepack.js /usr/local/bin/corepack

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        curl \
        file \
        git \
        libasound2-dev \
        libdbus-1-dev \
        libdecor-0-dev \
        libfontconfig1-dev \
        libfreetype6-dev \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        libpulse-dev \
        libudev-dev \
        libvulkan-dev \
        libwayland-bin \
        libwayland-dev \
        libx11-dev \
        libxcursor-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbcommon-dev \
        libxrandr-dev \
        pkg-config \
        python3 \
        python3-pip \
        unzip \
        zip \
    && python3 -m pip install --no-cache-dir scons==4.10.1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . /src

# Match modules/GodotJS/.github/workflows/misc_environment.yml: build the
# GodotJS runtime bundle from the module checkout, then consume scripts/out in
# the Godot SCons build below. The workflow's pnpm action resolves the pinned
# packageManager declaration in modules/GodotJS/package.json.
WORKDIR /src/modules/GodotJS
RUN corepack enable \
    && pnpm install \
    && pnpm build \
    && test -s scripts/out/jsb.runtime.bundle.js \
    && test -s scripts/out/jsb.editor.bundle.js

WORKDIR /src
RUN <<'EOF'
set -eu

case "${TARGETARCH}" in
  amd64)
    expected_arch=x86_64
    ;;
  arm64)
    expected_arch=arm64
    ;;
  *)
    echo "Unsupported Docker target architecture: ${TARGETARCH}" >&2
    exit 1
    ;;
esac

godot_arch="${GODOT_ARCH:-${expected_arch}}"
if [ "${godot_arch}" != "${expected_arch}" ]; then
  echo "GODOT_ARCH=${godot_arch} does not match --platform architecture ${TARGETARCH}; cross-compilation is intentionally not configured" >&2
  exit 1
fi

case "${GODOT_BUILD}" in
  template|editor|native-debug|native-release)
    ;;
  *)
    echo "GODOT_BUILD must be template, editor, native-debug, or native-release; received: ${GODOT_BUILD}" >&2
    exit 1
    ;;
esac

# Keep the GodotJS Linux workflow's SCons contract. OpenXR and udev are the
# two user-authorized server-template deviations: this dedicated Linux export
# has no XR/VR surface or gamepad hotplug callback requirement. `udev=no`
# excludes SDL's compiled libudev path; it is not a runtime environment hint.
scons_flags="platform=linuxbsd arch=${godot_arch} verbose=yes warnings=extra strict_checks=yes module_text_server_fb_enabled=yes skip_js_runtime=yes module_openxr_enabled=no udev=no cache_path=/tmp/breaka-scons-cache redirect_build_objects=no"
if [ "${GODOT_BUILD}" = "template" ]; then
  scons_flags="${scons_flags} debug_symbols=no"
elif [ "${GODOT_BUILD}" = "native-debug" ] || [ "${GODOT_BUILD}" = "native-release" ]; then
  scons_flags="${scons_flags} debug_symbols=yes separate_debug_symbols=yes"
fi

export BUILD_NAME=gh

run_scons() {
  target="$1"
  log="/tmp/breaka-scons-${target}.log"

  echo "Building SCons target ${target}..."
  set +e
  if [ "${SCONS_JOBS}" = "0" ]; then
    scons "target=${target}" ${scons_flags} >"${log}" 2>&1
  else
    scons -j"${SCONS_JOBS}" "target=${target}" ${scons_flags} >"${log}" 2>&1
  fi
  status=$?
  set -e

  if [ "${status}" -ne 0 ]; then
    echo "SCons target ${target} failed; concise diagnostic follows." >&2
    if ! grep -E -i 'fatal error:|error:|killed|cc1plus|collect2|undefined reference|out of memory' "${log}"; then
      tail -n 200 "${log}"
    fi
    exit "${status}"
  fi
}

if [ "${GODOT_BUILD}" = "template" ]; then
  # Match the delegated godot-build action: export templates omit editor code.
  rm -rf editor
  run_scons template_release
  run_scons template_debug

  strip "bin/godot.linuxbsd.template_release.${godot_arch}"
  strip "bin/godot.linuxbsd.template_debug.${godot_arch}"

  install -D -m 0755 "bin/godot.linuxbsd.template_release.${godot_arch}" \
    "/out/templates/${GODOT_TEMPLATE_VERSION}/linux_release.${godot_arch}"
  install -D -m 0755 "bin/godot.linuxbsd.template_debug.${godot_arch}" \
    "/out/templates/${GODOT_TEMPLATE_VERSION}/linux_debug.${godot_arch}"
elif [ "${GODOT_BUILD}" = "native-debug" ] || [ "${GODOT_BUILD}" = "native-release" ]; then
  # Native root-cause diagnostic: one selected template target with matching
  # DWARF, without the normal artifact target's final strip.
  rm -rf editor
  if [ "${GODOT_BUILD}" = "native-debug" ]; then
    symbol_target=template_debug
    output_name=linux_debug
  else
    symbol_target=template_release
    output_name=linux_release
  fi
  run_scons "${symbol_target}"

  binary="bin/godot.linuxbsd.${symbol_target}.${godot_arch}"
  symbols="${binary}.debugsymbols"
  test -s "${binary}"
  test -s "${symbols}"
  readelf -S "${binary}" | grep -q '\.gnu_debuglink'
  readelf -S "${symbols}" | grep -q '\.debug_info'

  install -D -m 0755 "${binary}" "/out/native-symbols/${output_name}.${godot_arch}"
  install -D -m 0644 "${symbols}" "/out/native-symbols/${output_name}.${godot_arch}.debugsymbols"
else
  run_scons editor
  strip "bin/godot.linuxbsd.editor.${godot_arch}"
  install -D -m 0755 "bin/godot.linuxbsd.editor.${godot_arch}" \
    "/out/editor/godot.linuxbsd.editor.${godot_arch}"
fi

grep -qx '#define JSB_WITH_QUICKJS 0' modules/GodotJS/jsb.gen.h
grep -qx '#define JSB_PREFER_QUICKJS_NG 0' modules/GodotJS/jsb.gen.h
grep -qx '#define JSB_WITH_V8 1' modules/GodotJS/jsb.gen.h
grep -qx '#define JSB_WITH_JAVASCRIPTCORE 0' modules/GodotJS/jsb.gen.h

{
  echo "godot_build=${GODOT_BUILD}"
  echo "godot_arch=${godot_arch}"
  echo "docker_target_arch=${TARGETARCH}"
  echo "godot_template_version=${GODOT_TEMPLATE_VERSION}"
  grep '^#define JSB_' modules/GodotJS/jsb.gen.h
} > /out/BUILD-INFO

find /out -type f -exec file {} \;
EOF

FROM scratch AS artifact

COPY --from=builder /out/ /
