#!/bin/bash -

if [[ $* == "setup" ]]
then
    cp ./regs.dat.bkp ./regs.dat
    make
fi

if [[ $* != "nolog" ]]
then
    > output.log
    LD_PRELOAD=./icshim.so icecat >> output.log
    > output.log
else
    LD_PRELOAD=./icshim.so icecat
fi
