#!/bin/bash
# Pause at reset (-S) and open GDB stub on :1234 (-s).
# Consider -no-reboot -no-shutdown so the window stays after a crash.
qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 512 -S -s -no-reboot -no-shutdown
