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
xterm -geometry 80x27+100+100 -hold -e "cd build/clipsserver && ./clipsserver" &
sleep 0.3
xterm -geometry 160x27+100+480 -hold -e "cd build/clipscontrol && ./clipscontrol" &
sleep 0.1

