#!/bin/sh

set -e

#echo "Step1: configure ASST1 kernel"
cd ~/cscc69/a2/kern/conf
./config ASST2-NORAND

#echo "Step2: build ASST1 kernel"
cd ../compile/ASST2-NORAND
bmake depend
bmake

#echo "Step 3: install kernel"
bmake install

#echo "Step4: build the user level utilities"
cd ../../..
bmake
bmake install

