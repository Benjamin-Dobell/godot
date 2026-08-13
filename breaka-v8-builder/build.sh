#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DOCKERFILE="${ROOT_DIR}/breaka-v8-builder/Dockerfile"

build_kind="template"
docker_platform="linux/amd64"
godot_arch=""
output_dir="${ROOT_DIR}/breaka-v8-builder/.output"
template_version="${GODOT_TEMPLATE_VERSION:-}"
install_template=false
build_source_dir="${ROOT_DIR}"

usage() {
  cat <<'USAGE'
Usage: breaka-v8-builder/build.sh [options]

Builds the local Godot 4.7dev/GodotJS V8 source tree in Docker.

Options:
  --kind <template|editor>       Build Linux export templates or an editor (default: template).
  --platform <linux/amd64|...>   Docker target platform (default: linux/amd64).
  --arch <x86_64|arm64>          Godot architecture; must match --platform.
  --template-version <version>   Output template version directory (default: source version.py resolver version).
  --out <directory>              Empty local artifact directory.
  --source <directory>           Godot source checkout used as the Docker build context (default: local Godot root).
  --install-template              Copy template files into this host's Godot template directory.
  -h, --help                     Show this help.

Examples:
  ./breaka-v8-builder/build.sh --kind template --platform linux/amd64 --install-template
  ./breaka-v8-builder/build.sh --source /tmp/clean-godot --kind template --platform linux/amd64 --install-template
  ./breaka-v8-builder/build.sh --kind editor --platform linux/amd64 --out /tmp/godot-v8-editor
USAGE
}

fail() {
  echo "[breaka-v8-builder] $*" >&2
  exit 1
}

default_template_version() {
  local version_file="${build_source_dir}/version.py"
  local major minor status

  if [ -f "${version_file}" ]; then
    major="$(awk -F= '/^major[[:space:]]*=/ { gsub(/[[:space:]]/, "", $2); print $2; exit }' "${version_file}")"
    minor="$(awk -F= '/^minor[[:space:]]*=/ { gsub(/[[:space:]]/, "", $2); print $2; exit }' "${version_file}")"
    status="$(awk -F= '/^status[[:space:]]*=/ { gsub(/['\''[:space:]]/, "", $2); print $2; exit }' "${version_file}")"

    if [ -n "${major}" ] && [ -n "${minor}" ] && [ -n "${status}" ]; then
      status="${status//\"/}"
      printf '%s.%s.%s\n' "${major}" "${minor}" "${status}"
      return
    fi
  fi

  printf '%s\n' "4.7.dev"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --kind)
      [ "$#" -ge 2 ] || fail "--kind requires a value"
      build_kind="$2"
      shift 2
      ;;
    --platform)
      [ "$#" -ge 2 ] || fail "--platform requires a value"
      docker_platform="$2"
      shift 2
      ;;
    --arch)
      [ "$#" -ge 2 ] || fail "--arch requires a value"
      godot_arch="$2"
      shift 2
      ;;
    --template-version)
      [ "$#" -ge 2 ] || fail "--template-version requires a value"
      template_version="$2"
      shift 2
      ;;
    --out)
      [ "$#" -ge 2 ] || fail "--out requires a value"
      output_dir="$2"
      shift 2
      ;;
    --source)
      [ "$#" -ge 2 ] || fail "--source requires a value"
      build_source_dir="$2"
      shift 2
      ;;
    --install-template)
      install_template=true
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      fail "Unknown option: $1"
      ;;
  esac
done

if [ -z "${template_version}" ]; then
  template_version="$(default_template_version)"
fi

case "${build_kind}" in
  template|editor)
    ;;
  *)
    fail "--kind must be template or editor"
    ;;
esac

if [ "${install_template}" = true ] && [ "${build_kind}" != "template" ]; then
  fail "--install-template requires --kind template"
fi

build_source_dir="$(CDPATH= cd -- "${build_source_dir}" && pwd)"
[ -f "${build_source_dir}/SConstruct" ] || fail "--source is not a Godot source checkout: ${build_source_dir}"
[ -f "${build_source_dir}/modules/GodotJS/package.json" ] \
  || fail "--source does not contain a GodotJS checkout at modules/GodotJS: ${build_source_dir}"
[ -f "${build_source_dir}/.dockerignore" ] \
  || fail "--source requires a reviewed .dockerignore: ${build_source_dir}"

if [ -e "${output_dir}" ]; then
  [ -d "${output_dir}" ] || fail "--out exists but is not a directory: ${output_dir}"
  [ -z "$(find "${output_dir}" -mindepth 1 -maxdepth 1 -print -quit)" ] \
    || fail "--out must be empty; refusing to replace existing artifacts: ${output_dir}"
else
  mkdir -p "${output_dir}"
fi

command -v docker >/dev/null 2>&1 || fail "Docker is required"
docker buildx version >/dev/null 2>&1 || fail "Docker Buildx is required"

build_args=(
  buildx build
  --platform "${docker_platform}"
  --target artifact
  --file "${DOCKERFILE}"
  --build-arg "GODOT_BUILD=${build_kind}"
  --build-arg "GODOT_TEMPLATE_VERSION=${template_version}"
  --output "type=local,dest=${output_dir}"
)

if [ -n "${godot_arch}" ]; then
  build_args+=(--build-arg "GODOT_ARCH=${godot_arch}")
fi

echo "[breaka-v8-builder] Building ${build_kind} for ${docker_platform} from ${build_source_dir} into ${output_dir}"
docker "${build_args[@]}" "${build_source_dir}"

if [ "${install_template}" = true ]; then
  case "$(uname -s)" in
    Darwin)
      template_dir="${HOME}/Library/Application Support/Godot/export_templates/${template_version}"
      ;;
    *)
      template_dir="${XDG_DATA_HOME:-${HOME}/.local/share}/godot/export_templates/${template_version}"
      ;;
  esac

  template_source_dir="${output_dir}/templates/${template_version}"
  [ -d "${template_source_dir}" ] || fail "Template output was not produced: ${template_source_dir}"

  mkdir -p "${template_dir}"
  for template in "${template_source_dir}"/linux_debug.* "${template_source_dir}"/linux_release.*; do
    [ -f "${template}" ] || fail "Missing template artifact: ${template}"
    install -m 0755 "${template}" "${template_dir}/$(basename "${template}")"
  done

  echo "[breaka-v8-builder] Installed templates in ${template_dir}"
fi

echo "[breaka-v8-builder] Build artifacts are available in ${output_dir}"
