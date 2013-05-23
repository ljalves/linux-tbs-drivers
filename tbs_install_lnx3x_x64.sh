#!/bin/bash

echo "TBS drivers set for x64 Linux 3.x"

./v4l/tbs-x86_64.sh

echo "TBS drivers building..."
make

echo "TBS drivers installing..."
sudo make install

echo "TBS drivers installation done"
echo "You need to reboot..."
