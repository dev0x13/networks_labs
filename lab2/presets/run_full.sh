#!/bin/bash

echo "[CLEAR]"
./clean_channels cfg/full/dr.cfg
echo "[DESIGNATED ROUTER]"
./router 1 cfg/full/dr.cfg &
sleep 0.5
echo "[ROUTER #1]"
./router 0 cfg/full/r1.cfg &
sleep 0.5
echo "[ROUTER #2]"
./router 0 cfg/full/r2.cfg &
sleep 0.5
echo "[ROUTER #3]"
./router 0 cfg/full/r3.cfg &
sleep 0.5
echo "[ROUTER #4]"
./router 0 cfg/full/r4.cfg &
sleep 0.5
echo "[ROUTER #5]"
./router 0 cfg/full/r5.cfg &
sleep 0.5
echo "[ROUTER #6]"
./router 0 cfg/full/r6.cfg &
