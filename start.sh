#!/bin/bash

if [ ! -f ./build/clipsserver/clipsserver ]; then
	if [ ! -d ./build ]; then
		mkdir -p build;
	fi
	cd build;
	cmake .. && make;
	cd ..
fi

#Script to start CLIPS
xterm -hold -e "cd build/clipsserver && ./clipsserver" &
# xterm -hold -e "cd build/clipscontrol && ./clipscontrol" &
xterm -hold -e "cd tests/pytest && ./test.py"
