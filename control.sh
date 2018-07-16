#!/bin/bash

if [ $1 = "s" ]; then
  echo 0 > /dev/motor-left
  echo 0 > /dev/motor-right
elif [ $1 = "f" ]; then
  echo 1 > /dev/motor-left
  echo 1 > /dev/motor-right
  sleep 1
  echo 0 > /dev/motor-left
  echo 0 > /dev/motor-right
elif [ $1 = "b" ]; then
  echo '-1' > /dev/motor-left
  echo '-1' > /dev/motor-right
  sleep 1
  echo '0' > /dev/motor-left
  echo '0' > /dev/motor-right
elif [ $1 = "r" ]; then
  echo '1' > /dev/motor-left
  echo '-1' > /dev/motor-right
elif [ $1 = "l" ]; then
  echo '1' > /dev/motor-right
  echo '-1' > /dev/motor-left
else
  exit 1
fi
