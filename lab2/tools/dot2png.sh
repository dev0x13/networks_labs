#!/bin/bash

if [ $# -ne 2 ]; then
  echo 1>&2 "Usage: $0 <path to input *.dot file> <path to output *.png file>"
  exit 3
fi

neato -Tpng $1 -o $2
