# `tcpCLIPS60`: A ROS node binding for CLIPS 6.0
This repo allows you to connect CLIPS 6.00 (version of 05/12/93) with anything supporting sockets.
This CLIPS solution is purely written in C and includes no patches, fixes, or anything.
The Server is written in C++14.

Why did I use this old version?
Don't even dare to ask. I was required to because *bananas*.

Note: one *banana* is necessary and sufficient reason to do something. Imagine many.

# QuickStart

This guide assumes that you faithfully run Linux and have installed compilers and stuff.

<!--
## Step 0: Install dependencies
```
# apt install libncurses-dev
```
-->

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
Simply go to the root of the repo and run `test.sh`:
```
./test.sh
```

Things start to move now.
Just follow the instruction on the test node.