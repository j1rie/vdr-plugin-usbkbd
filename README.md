## This is a plugin for the Video Disk Recorder (VDR).

Written by:                  Joerg Riechardt <J.Riechardt@gmx.de>

Project's homepage:          https://github.com/j1rie/vdr-plugin-usbkbd

Latest version available at: https://github.com/j1rie/vdr-plugin-usbkbd

## Description: The 'usbkbd' plugin sends keypresses from an USB keyboard to VDR. Even when X is active.
It also works with any input device that sends keystrokes.

The easiest way for creating key mappings in remote.conf is to use VDR's key learning process.

## USB keyboard
Use lsusb to check the keyboard IDs, modify 70-usbkbd.rules accordingly, and place it in the udev directory.  
Then you won't need to pass the keyboard event device as a parameter to the plugin.

The keyboard can be connected or disconnected as you like.

## rc-core device (untestet, should work too)
Find the IDs with udevadm info --query=all --attribute-walk --name=/dev/input/eventX

## uinput device (untestet, should work too)

## turn it off and on
svdrpsend REMO off  
svdrpsend REMO on

## VDR's Text Input Mode
You can enter letters and numbers, use the color buttons, and navigate as usual.  

## Keyboard layout
The keyboard layout is derived from the VDR language. Alternatively, it can be controlled via environment variables.

For example, the system-wide settings can be inherited from the X server:
```
eval $(LC_ALL=C localectl status | awk -F': ' '/X11 Layout/ {print "export XKB_DEFAULT_LAYOUT=" $2} /X11 Model/ {print "export XKB_DEFAULT_MODEL=" $2} /X11 Variant/ {print "export XKB_DEFAULT_VARIANT=" $2} /X11 Options/ {print "export XKB_DEFAULT_OPTIONS=" $2}')
```

## xineliboutput
In setup.conf set xineliboutput.X11.UseKeyboard = 0  
vdr-sxfe needs to be started with -x.

## Arguments
The following arguments can be specified at startup:

- `-d` or `--device` Specifies the input device (/dev/input/eventX) to read from. The default is `/dev/usbkbd_event`.
- `-l` or `letterdetection` Enables automatic detection of text input via the keyboard. Once this is detected (by pressing a letter key), the number keys no longer produce letters (as required for text input via the remote control), but only numbers. This allows letters to be entered via the remote control using the number keys as usual, while the number keys on the keyboard can be used for entering numbers.
- `-k` or `keypadnumbers` Enables the conversion of keypad numbers into letters for text input. As a result, during text entry, the numeric keys on the keyboard are always used to enter numbers, and the numeric keys on the remote control are used to enter letters. To achieve this, the remote control must be programmed so that the numeric keys KEY_KP0 through KEY_KP9 send events, and the normal numeric keys on the keyboard send events when programming the VDR.

If neither the `-l` nor `-k` option is used, “Number keys for characters” can be disabled via the VDR OSD settings. Otherwise, the number keys on a keyboard behave like those on a remote control and, when pressed repeatedly, cycle through letters and numbers in a loop.
