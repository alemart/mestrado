#!/bin/bash
# desabilita USB autosuspend
for i in /sys/bus/usb/devices/*/power/autosuspend; do
    echo $i
    echo -1 | sudo tee $i
done
