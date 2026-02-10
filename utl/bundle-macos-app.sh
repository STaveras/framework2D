#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BIN_DIR="${REPO_ROOT}/bin"
OUTPUT_APP=""
APP_NAME=""
BUNDLE_ID=""
BINARY_NAME=""
RUN_COMMAND_NAME="run.command"
ICON_SOURCE="${REPO_ROOT}/icon.png"
OVERWRITE=0

usage() {
  cat <<'EOF'
Usage:
  ./utl/bundle-macos-app.sh [options]

Options:
  --bin-dir <path>      Path to folder to bundle (default: ./bin)
  --binary <name>       Executable inside --bin-dir to launch
  --run-command <name>  Launcher script inside --bin-dir (default: run.command)
  --app-name <name>     App display name (default: derived from binary)
  --bundle-id <id>      CFBundleIdentifier (default: com.framework2d.<app-name>)
  --icon <path>         Source PNG icon (default: ./icon.png)
  --output <path>       Output .app path (default: ./<app-name>.app)
  --overwrite           Replace existing output bundle
  -h, --help            Show this help
EOF
}

die() {
  echo "Error: $*" >&2
  exit 1
}

make_absolute_path() {
  local input_path="$1"
  if [[ "${input_path}" = /* ]]; then
    printf '%s\n' "${input_path}"
  else
    printf '%s/%s\n' "${PWD}" "${input_path}"
  fi
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --bin-dir)
      [[ $# -ge 2 ]] || die "Missing value for --bin-dir"
      BIN_DIR="$2"
      shift 2
      ;;
    --binary)
      [[ $# -ge 2 ]] || die "Missing value for --binary"
      BINARY_NAME="$2"
      shift 2
      ;;
    --run-command)
      [[ $# -ge 2 ]] || die "Missing value for --run-command"
      RUN_COMMAND_NAME="$2"
      shift 2
      ;;
    --app-name)
      [[ $# -ge 2 ]] || die "Missing value for --app-name"
      APP_NAME="$2"
      shift 2
      ;;
    --bundle-id)
      [[ $# -ge 2 ]] || die "Missing value for --bundle-id"
      BUNDLE_ID="$2"
      shift 2
      ;;
    --icon)
      [[ $# -ge 2 ]] || die "Missing value for --icon"
      ICON_SOURCE="$2"
      shift 2
      ;;
    --output)
      [[ $# -ge 2 ]] || die "Missing value for --output"
      OUTPUT_APP="$2"
      shift 2
      ;;
    --overwrite)
      OVERWRITE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown argument: $1"
      ;;
  esac
done

BIN_DIR="$(make_absolute_path "${BIN_DIR}")"
ICON_SOURCE="$(make_absolute_path "${ICON_SOURCE}")"
[[ -d "${BIN_DIR}" ]] || die "Bin directory does not exist: ${BIN_DIR}"

RUN_COMMAND_PATH="${BIN_DIR}/${RUN_COMMAND_NAME}"
USE_RUN_COMMAND=0
if [[ -f "${RUN_COMMAND_PATH}" ]]; then
  USE_RUN_COMMAND=1
fi

if [[ ${USE_RUN_COMMAND} -eq 1 ]]; then
  chmod +x "${RUN_COMMAND_PATH}"

  if grep -Eq '\./framework2D_d(\s|$|")' "${RUN_COMMAND_PATH}"; then
    die "${RUN_COMMAND_NAME} is configured for debug binary. Update it to use ./framework2D."
  fi

  if [[ -z "${BINARY_NAME}" ]]; then
    BINARY_NAME="framework2D"
  fi
else
  if [[ -z "${BINARY_NAME}" ]]; then
    exec_candidates=()
    while IFS= read -r file_path; do
      exec_candidates+=("${file_path}")
    done < <(find "${BIN_DIR}" -maxdepth 1 -type f -perm -u+x ! -name "*.command" -print | sort)

    if [[ ${#exec_candidates[@]} -eq 0 ]]; then
      die "No executable found in ${BIN_DIR}. Use --binary to specify one."
    fi

    if [[ ${#exec_candidates[@]} -gt 1 ]]; then
      echo "Multiple executables found in ${BIN_DIR}:" >&2
      for candidate in "${exec_candidates[@]}"; do
        echo "  - $(basename "${candidate}")" >&2
      done
      die "Please choose one with --binary <name>."
    fi

    BINARY_NAME="$(basename "${exec_candidates[0]}")"
  fi
fi

BINARY_PATH="${BIN_DIR}/${BINARY_NAME}"
[[ -f "${BINARY_PATH}" ]] || die "Executable not found: ${BINARY_PATH}"
[[ -x "${BINARY_PATH}" ]] || die "File is not executable: ${BINARY_PATH}"

if [[ -z "${APP_NAME}" ]]; then
  TITLE_FILE_PATH="$(find "${BIN_DIR}" -mindepth 2 -type f -name title -print | sort | head -n 1 || true)"
  if [[ -n "${TITLE_FILE_PATH}" ]]; then
    APP_NAME="$(sed -n '1{s/\r$//;p;}' "${TITLE_FILE_PATH}" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
  fi

  if [[ -z "${APP_NAME}" ]]; then
    APP_NAME="${BINARY_NAME%_d}"
    [[ -n "${APP_NAME}" ]] || APP_NAME="${BINARY_NAME}"
  fi
fi

if [[ -z "${BUNDLE_ID}" ]]; then
  bundle_suffix="$(printf '%s' "${APP_NAME}" | tr '[:upper:]' '[:lower:]' | tr -cs 'a-z0-9' '.' | sed 's/^\.*//;s/\.*$//')"
  [[ -n "${bundle_suffix}" ]] || bundle_suffix="app"
  BUNDLE_ID="com.framework2d.${bundle_suffix}"
fi

if [[ -z "${OUTPUT_APP}" ]]; then
  OUTPUT_APP="${REPO_ROOT}/${APP_NAME}.app"
fi

OUTPUT_APP="$(make_absolute_path "${OUTPUT_APP}")"
OUTPUT_PARENT="$(dirname "${OUTPUT_APP}")"
mkdir -p "${OUTPUT_PARENT}"

if [[ -e "${OUTPUT_APP}" ]]; then
  if [[ ${OVERWRITE} -eq 1 ]]; then
    rm -rf "${OUTPUT_APP}"
  else
    die "Output exists: ${OUTPUT_APP} (use --overwrite to replace it)."
  fi
fi

CONTENTS_DIR="${OUTPUT_APP}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

mkdir -p "${MACOS_DIR}" "${RESOURCES_DIR}"

cp -R "${BIN_DIR}/." "${RESOURCES_DIR}/"
chmod +x "${RESOURCES_DIR}/${BINARY_NAME}"

LAUNCHER_NAME="$(printf '%s' "${APP_NAME}" | tr -cs 'A-Za-z0-9_-' '_')"
[[ -n "${LAUNCHER_NAME}" ]] || LAUNCHER_NAME="launcher"
LAUNCHER_PATH="${MACOS_DIR}/${LAUNCHER_NAME}"

cat > "${LAUNCHER_PATH}" <<EOF
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="\$(cd "\$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
RESOURCES_DIR="\${SCRIPT_DIR}/../Resources"

cd "\${RESOURCES_DIR}"
EOF

if [[ ${USE_RUN_COMMAND} -eq 1 ]]; then
  cat >> "${LAUNCHER_PATH}" <<EOF
exec "./${RUN_COMMAND_NAME}" "\$@"
EOF
else
  cat >> "${LAUNCHER_PATH}" <<EOF
exec "./${BINARY_NAME}" "\$@"
EOF
fi

chmod +x "${LAUNCHER_PATH}"

ICON_NAME="AppIcon"
ICON_CREATED=0

if [[ -f "${ICON_SOURCE}" ]]; then
  if command -v sips >/dev/null 2>&1; then
    ICON_TMP_DIR="$(mktemp -d -t appicon)"
    ICONSET_DIR="${ICON_TMP_DIR}/AppIcon.iconset"
    mkdir -p "${ICONSET_DIR}"

    create_iconset_image() {
      local size="$1"
      local output_name="$2"
      sips -s format png -z "${size}" "${size}" "${ICON_SOURCE}" --out "${ICONSET_DIR}/${output_name}" >/dev/null
    }

    create_iconset_image 16 "icon_16x16.png"
    create_iconset_image 32 "icon_16x16@2x.png"
    create_iconset_image 32 "icon_32x32.png"
    create_iconset_image 64 "icon_32x32@2x.png"
    create_iconset_image 128 "icon_128x128.png"
    create_iconset_image 256 "icon_128x128@2x.png"
    create_iconset_image 256 "icon_256x256.png"
    create_iconset_image 512 "icon_256x256@2x.png"
    create_iconset_image 512 "icon_512x512.png"
    create_iconset_image 1024 "icon_512x512@2x.png"

    if command -v iconutil >/dev/null 2>&1; then
      if iconutil -c icns "${ICONSET_DIR}" -o "${RESOURCES_DIR}/${ICON_NAME}.icns" >/dev/null 2>&1; then
        ICON_CREATED=1
      else
        echo "Info: iconutil conversion failed; falling back to tiff2icns." >&2
      fi
    fi

    # Fallback: on some systems iconutil rejects otherwise valid iconsets.
    if [[ ${ICON_CREATED} -eq 0 ]] && command -v tiffutil >/dev/null 2>&1 && command -v tiff2icns >/dev/null 2>&1; then
      TIFF_PATHS=()
      for size in 16 32 48 128 256 512 1024; do
        TIFF_PATH="${ICON_TMP_DIR}/icon_${size}.tiff"
        sips -s format tiff -z "${size}" "${size}" "${ICON_SOURCE}" --out "${TIFF_PATH}" >/dev/null
        TIFF_PATHS+=("${TIFF_PATH}")
      done

      MULTI_TIFF_PATH="${ICON_TMP_DIR}/AppIcon.tiff"
      if tiffutil -cat "${TIFF_PATHS[@]}" -out "${MULTI_TIFF_PATH}" >/dev/null 2>&1; then
        if tiff2icns "${MULTI_TIFF_PATH}" "${RESOURCES_DIR}/${ICON_NAME}.icns" >/dev/null 2>&1; then
          if [[ -s "${RESOURCES_DIR}/${ICON_NAME}.icns" ]]; then
            ICON_CREATED=1
          fi
        fi
      fi
    fi

    if [[ ${ICON_CREATED} -eq 0 ]]; then
      echo "Warning: icon conversion failed for ${ICON_SOURCE}; app bundle will be created without custom icon." >&2
    fi

    rm -rf "${ICON_TMP_DIR}"
  else
    echo "Warning: icon conversion skipped (sips not available)." >&2
  fi
else
  echo "Warning: icon source not found, skipping icon: ${ICON_SOURCE}" >&2
fi

ICON_PLIST_ENTRY=""
if [[ ${ICON_CREATED} -eq 1 ]]; then
  ICON_PLIST_ENTRY=$'  <key>CFBundleIconFile</key>\n  <string>AppIcon</string>'
fi

PLIST_PATH="${CONTENTS_DIR}/Info.plist"
cat > "${PLIST_PATH}" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>
  <string>en</string>
  <key>CFBundleExecutable</key>
  <string>${LAUNCHER_NAME}</string>
  <key>CFBundleIdentifier</key>
  <string>${BUNDLE_ID}</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
${ICON_PLIST_ENTRY}
  <key>CFBundleName</key>
  <string>${APP_NAME}</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundleVersion</key>
  <string>1</string>
  <key>LSMinimumSystemVersion</key>
  <string>11.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
EOF

if command -v plutil >/dev/null 2>&1; then
  plutil -lint "${PLIST_PATH}" >/dev/null
fi

echo "Created app bundle: ${OUTPUT_APP}"
echo "Bundle id: ${BUNDLE_ID}"
echo "Launcher executable: ${LAUNCHER_NAME}"
if [[ ${USE_RUN_COMMAND} -eq 1 ]]; then
  echo "Launch target: ${RUN_COMMAND_NAME}"
else
  echo "Launch target: ${BINARY_NAME}"
fi
echo "Game binary: ${BINARY_NAME}"
echo "Open it with: open \"${OUTPUT_APP}\""
