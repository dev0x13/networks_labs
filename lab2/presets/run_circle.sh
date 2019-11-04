#!/bin/bash

echo "[CLEAR]"
./clean_channels cfg/circle/dr.cfg
echo "[DESIGNATED ROUTER]"
./router 1 cfg/circle/dr.cfg &
sleep 0.5
echo "[ROUTER #1]"
./router 0 cfg/circle/r1.cfg &
sleep 0.5
echo "[ROUTER #2]"
./router 0 cfg/circle/r2.cfg &
sleep 0.5
echo "[ROUTER #3]"
./router 0 cfg/circle/r3.cfg &
sleep 0.5
echo "[ROUTER #4]"
./router 0 cfg/circle/r4.cfg &
sleep 0.5
echo "[ROUTER #5]"
./router 0 cfg/circle/r5.cfg &
