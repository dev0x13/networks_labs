#!/bin/bash

echo "[CLEAR]"
./clean_channels_bin cfg/clean_channels_bin.cfg
echo "[SUN NODE]"
./sun_node_bin cfg/sun_node.cfg &
sleep 0.5
echo "[FOCUS] NODE]"
./focus_node_bin cfg/focus_node.cfg &
sleep 0.5
echo "[CONTROL NODE]"
./control_node_bin cfg/control_node.cfg &
sleep 0.5
echo "[ROUTER #1]"
./worker_node_bin cfg/r1.cfg &
sleep 0.5
echo "[ROUTER #2]"
./worker_node_bin cfg/r2.cfg &
sleep 0.5
echo "[ROUTER #3]"
./worker_node_bin cfg/r3.cfg &
sleep 0.5
echo "[ROUTER #4]"
./worker_node_bin cfg/r4.cfg &
sleep 0.5
