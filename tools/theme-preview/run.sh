#!/bin/sh
# Build the launcher's palette code for the host and render every candidate.
set -e
cd "$(dirname "$0")"
ROOT=../..
clang++ -std=c++17 -O1 -I $ROOT/arm9/source -o dump_scheme dump_scheme.cpp \
  $ROOT/arm9/source/material/palettes/core.cpp \
  $ROOT/arm9/source/material/palettes/tones.cpp \
  $ROOT/arm9/source/material/scheme/scheme.cpp \
  $ROOT/arm9/source/material/cam/cam.cpp \
  $ROOT/arm9/source/material/cam/hct.cpp \
  $ROOT/arm9/source/material/cam/hct_solver.cpp \
  $ROOT/arm9/source/material/cam/viewing_conditions.cpp \
  $ROOT/arm9/source/material/utils/utils.cpp
python3 render_swatches.py
