#!/bin/sh
# Copies the game's data folder into the app bundle. Run by Xcode after
# linking (see CMakeLists.txt), which provides TARGET_BUILD_DIR and
# UNLOCALIZED_RESOURCES_FOLDER_PATH.
#   copy-game-data.sh <data folder>
set -eu

source_dir="$1"
destination="${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/$(basename "$source_dir")"

# Source art and reference captures are never loaded by the game.
rsync -a --delete \
  --exclude "*.aseprite" --exclude "*.gif" --exclude "*.bak" --exclude ".DS_Store" \
  "$source_dir/" "$destination/"
