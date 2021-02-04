#!/bin/bash

cmake -G "Eclipse CDT4 - Ninja" \
	-B _build/eclipse-debug \
	-S server-radio \
	-DCMAKE_BUILD_TYPE=Debug
