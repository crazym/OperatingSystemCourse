#!/bin/sh

set -e

#echo "Step1: configure ASST1 kernel"
cd kern/conf
./config ASST1

#echo "Step2: build ASST1 kernel"
cd ../compile/ASST1
bmake depend
bmake

#echo "Step 3: install kernel"
bmake install

#echo "Step4: build the user level utilities"
cd ../../..
bmake
bmake install

