Dies ist ein Plugin für den Video Disk Recorder (VDR).

Verfasst von:                     Joerg Riechardt <J.Riechardt@gmx.de>

Projekt-Homepage:                 https://github.com/j1rie/vdr-plugin-usbkbd

Aktuelle Version verfügbar unter: https://github.com/j1rie/vdr-plugin-usbkbd

Beschreibung: Das Plugin usbkbd leitet Tastatureingaben von einer USB-Tastatur an den VDR weiter. Auch wenn X aktiv ist.

Der einfachste Weg, Tastenbelegungen in remote.conf zu erstellen, ist die Verwendung des Anlernprozesses von VDR.

Man überprüft die IDs der Tastatur mit lsusb, passt die 70-usbkbd.rules entsprechend an und legt sie in das udev-Verzeichnis.  
Dann muss man dem Plugin das Tastatur-Event-Gerät nicht als Parameter übergeben.

Die Tastatur kann beliebig an- und abgesteckt werden.
