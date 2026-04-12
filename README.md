This is a "plugin" for the Video Disk Recorder (VDR).

Written by:                  Joerg Riechardt <J.Riechardt@gmx.de>

Project's homepage:          https://github.com/j1rie/vdr-plugin-usbkbd

Latest version available at: https://github.com/j1rie/vdr-plugin-usbkbd

Description: The 'usbkbd' plugin sends keypresses from an USB keyboard to VDR. Even when X is active.

The easiest way for creating key mappings in remote.conf is to use VDR's key learning process.

Check the IDs of your keyboard with lsusb, adapt 70-usbkbd.rules accordingly and put it in your udev directory.  
Then you don't need to give the keyboard event device to the plugin as a parameter.
