#!/bin/bash

echo "TBS drivers set for x86 Linux 3.x"

./v4l/tbs-x86_r3.sh

echo "TBS drivers building..."
make -j2

echo "TBS drivers installing..."
sudo make install

echo "TBS drivers installation done"
echo "You need to reboot..."
