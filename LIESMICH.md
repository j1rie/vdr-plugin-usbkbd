## Dies ist ein Plugin für den Video Disk Recorder (VDR).

Verfasst von:                     Joerg Riechardt <J.Riechardt@gmx.de>

Projekt-Homepage:                 https://github.com/j1rie/vdr-plugin-usbkbd

Aktuelle Version verfügbar unter: https://github.com/j1rie/vdr-plugin-usbkbd

## Beschreibung: Das Plugin usbkbd leitet Tastendrücke von einer USB-Tastatur an den VDR weiter. Auch wenn X aktiv ist.
Es funktioniert auch für alle Eingabe Geräte, die Tastendrücke senden.

Der einfachste Weg, Tastenbelegungen in remote.conf zu erstellen, ist die Verwendung des Anlernprozesses von VDR.

## USB Tastatur
Man überprüft die IDs der Tastatur mit lsusb, passt die 70-usbkbd.rules entsprechend an und legt sie in das udev-Verzeichnis.  
Dann muss man dem Plugin das Tastatur-Event-Gerät nicht als Parameter übergeben.

Die Tastatur kann beliebig an- und abgesteckt werden.

## rc-core Gerät (ungetestet, sollte auch gehen)
Die IDs werden mit udevadm info --query=all --attribute-walk --name=/dev/input/eventX gefunden.

## uinput Gerät (ungetestet, sollte auch gehen)

## aus- und anstellen
svdrpsend REMO off  
svdrpsend REMO on

## VDR's Texteingabe Modus
Man kann Buchstaben und Zahlen eingeben, die Farbtasten benutzen und wie gewohnt navigieren.  
In den Einstellungen für OSD muss "Zifferntasten für Zeichen" aus sein.

## xineliboutput
In der setup.conf sollte xineliboutput.X11.UseKeyboard = 0 sein.

