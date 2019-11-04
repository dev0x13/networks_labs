#!/bin/bash

echo "[CLEAR]"
./clean_channels cfg/cross/dr.cfg
echo "[DESIGNATED ROUTER]"
./router 1 cfg/cross/dr.cfg &
sleep 0.5
echo "[ROUTER #1]"
./router 0 cfg/cross/r1.cfg &
sleep 0.5
echo "[ROUTER #2]"
./router 0 cfg/cross/r2.cfg &
sleep 0.5
echo "[ROUTER #3]"
./router 0 cfg/cross/r3.cfg &
sleep 0.5
echo "[ROUTER #4]"
./router 0 cfg/cross/r4.cfg &
sleep 0.5
echo "[ROUTER #5]"
./router 0 cfg/cross/r5.cfg &
