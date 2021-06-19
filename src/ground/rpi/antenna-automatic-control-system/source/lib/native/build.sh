#!/usr/bin/env bash
set catalog_path=pwd
cmake -G "Unix Makefiles" -B build/ -S src/lib-strela-ms-rpi/ -DCMAKE_INSTALL_PREFIX=${catalog_path}/stage
cmake --build build/ --target install -- -j4
