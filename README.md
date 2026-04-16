## This is a plugin for the Video Disk Recorder (VDR).

Written by:                  Joerg Riechardt <J.Riechardt@gmx.de>

Project's homepage:          https://github.com/j1rie/vdr-plugin-usbkbd

Latest version available at: https://github.com/j1rie/vdr-plugin-usbkbd

## Description: The 'usbkbd' plugin sends keypresses from an USB keyboard to VDR. Even when X is active.

The easiest way for creating key mappings in remote.conf is to use VDR's key learning process.

## USB keyboard
Use lsusb to check the keyboard IDs, modify 70-usbkbd.rules accordingly, and place it in the udev directory.  
Then you won't need to pass the keyboard event device as a parameter to the plugin.

The keyboard can be connected or disconnected as you like.

## rc-core device (untestet, should work too)
Find the IDs with udevadm info --query=all --attribute-walk --name=/dev/input/eventX

## uinput device (untestet, should work too)
