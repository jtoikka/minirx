#!/bin/bash
if [ ! -d build ]; then
  meson . build
fi
cd build
rm tests
ninja
if [ -f tests ]; then
  ./tests
fi
