#!/bin/bash

echo "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
echo "generating project for server-radio"
echo "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"

cmake -G "Eclipse CDT4 - Ninja" \
	-B _build/eclipse-debug/server-radio \
	-S server-radio \
	-DCMAKE_BUILD_TYPE=Debug


echo "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"
echo "generating project for server-uslp"
echo "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-"

cmake -G "Eclipse CDT4 - Ninja" \
	-B _build/eclipse-debug/server-uslp \
	-S server-uslp \
	-DCMAKE_BUILD_TYPE=Debug
