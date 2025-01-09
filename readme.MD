# `tcpCLIPS60`: A ROS node binding for CLIPS 6.0
This repo allows you to connect CLIPS 6.00 (version of 05/12/93) with anything supporting sockets.
This CLIPS solution is purely written in C and includes no patches, fixes, or anything.
The Server is written in C++14.

Why did I use this old version? Beause: _why not!?_
CLIPS6.0 is a cool version and should suffice all our needs.
`tcpCLIPS64` is comming [SOON™](https://wowwiki-archive.fandom.com/wiki/Soon)

# QuickStart

This guide assumes that you faithfully run Linux and have installed compilers and stuff.


## Step 0: Install dependencies
```
# apt install cmake libboost-all-dev libncurses-dev
```


## Step 1: Download and build
1. Clone this repo somewhere
```
mkdir -p ~/develop
cd develop
git clone https://github.com/kyordhel/tcpclips60.git
```

2. Build the source (it may take a while)
```
cd tcpclips60/build
cmake .. && make
cd ..
```


## Step 2: Test it
Simply go to the root of the repo and run `start.sh`:
```
./start.sh
```

Things start to move now.
Just follow the instruction on the test node.
