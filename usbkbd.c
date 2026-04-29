/*
 * usbkbd.c: A plugin for the Video Disk Recorder
 *
 * Copyright (C) 20026-2026 Joerg Riechardt <J.Riechardt@gmx.de>
 *
 */

#include <vdr/plugin.h>
#include <vdr/i18n.h>
#include <vdr/remote.h>
#include <vdr/thread.h>
#include "input-event-codes.h"
#include <linux/input.h>
#include <locale.h>
#include <ctype.h>

static const char *VERSION        = "0.0.6";
static const char *DESCRIPTION    = tr("Send keypresses from USB keyboard to VDR");

#define DEBUG 1
#define RECONNECTDELAY 3000 // ms

const char* usbkbd_device = "/dev/usbkbd_event";

class cUsbkbdRemote : public cRemote, private cThread {
private:
  bool Connect(void);
  void Action(void);
  bool Ready();
  int fd;
  struct input_event event;
public:
  cUsbkbdRemote(const char *Name);
  ~cUsbkbdRemote();
};

cUsbkbdRemote::cUsbkbdRemote(const char *Name)
:cRemote(Name)
,cThread("USBKBD remote control")
{
  Connect();
  Start();
}

cUsbkbdRemote::~cUsbkbdRemote()
{
  Cancel();
  //ioctl(fd, EVIOCGRAB, 0);
  if (fd >= 0)
     close(fd);
  fd = -1;
}

bool cUsbkbdRemote::Connect()
{
  fd = open(usbkbd_device, O_RDONLY);
  if(fd == -1){
    if(DEBUG) printf("Cannot open %s. %s.\n", usbkbd_device, strerror(errno));
    esyslog("usbkbd: Cannot open %s. %s.\n", usbkbd_device, strerror(errno));
    return false;
  } else {
    if(DEBUG) printf("opened %s\n", usbkbd_device);
    isyslog("usbkbd: opened %s\n", usbkbd_device);
  }

  /*if(ioctl(fd, EVIOCGRAB, 1)){
    if(DEBUG) printf("Cannot grab %s. %s.\n", usbkbd_device, strerror(errno));
  } else {
    if(DEBUG) printf("Grabbed %s!\n", usbkbd_device);
  }*/

  return true;
}

bool cUsbkbdRemote::Ready(void)
{
  return fd >= 0;
}

void cUsbkbdRemote::Action(void)
{
  cTimeMs FirstTime;
  cTimeMs LastTime;
  cTimeMs ThisTime;
  bool repeat = false;
  cString key = "";
  cString lastkey = "";
  bool connected = true;

  if(DEBUG) printf("UsbkbdRemote action!\n");

  while(Running()) {
    while (access(usbkbd_device, F_OK) == -1) {
      if (connected) {
          connected = false;
          esyslog("usbkbd: no connection to %s, trying to reconnect every %.1f seconds", usbkbd_device, float(RECONNECTDELAY) / 1000);
          if(DEBUG) printf("no connection to %s, trying to reconnect every %.1f seconds\n", usbkbd_device, float(RECONNECTDELAY) / 1000);
      }
      //ioctl(fd, EVIOCGRAB, 0);
      if (fd >= 0) {
        close(fd);
        fd = -1;
      }
      cCondWait::SleepMs(RECONNECTDELAY);
    }

    if (fd == -1) {
        if (Connect()) {
            if (!connected)
                connected = true;
            isyslog("usbkbd: reconnected to %s", usbkbd_device);
            if(DEBUG) printf("reconnected to %s\n", usbkbd_device);
            //cCondWait::SleepMs(3); // wait a little after reconnect
        }
    }

    if (Ready() && read(fd, &event, sizeof(event)) != -1 && (event.type == EV_KEY)) {

        key = cString::sprintf("%s", evkeys[event.code]);

        int Delta = ThisTime.Elapsed(); // the time between two consecutive events
        if (DEBUG) printf("Delta: %d\n", Delta);
        ThisTime.Set();

        if (DEBUG) printf("key: %s, lastkey: %s  %s\n", (const char*)key, (const char*)lastkey, event.value == 0 ? "Release" : "");

        if (event.value == 1) { // new key
            if (DEBUG) printf("new key\n");
            if (repeat) {
                if (DEBUG) printf("put %s Release\n", (const char*)lastkey);
                Put(lastkey, false, true); // generated release for previous repeated key
            }
            lastkey = key;
            repeat = false;
            FirstTime.Set();
        } else if (event.value == 2) { // repeat
            if (DEBUG) printf("repeat\n");
            if (FirstTime.Elapsed() < (uint)Setup.RcRepeatDelay) {
                if (DEBUG) printf("continue Delay\n\n");
                continue; // repeat function kicks in after a short delay
            }
            if (LastTime.Elapsed() < (uint)Setup.RcRepeatDelta) {
                if (DEBUG)  printf("continue Delta\n\n");
                continue; // skip same keys coming in too fast
            }
            repeat = true;
        }

        /* send key */
        if (event.value == 1 || event.value == 2) {
            if(DEBUG) printf("delta send: %ld\n", LastTime.Elapsed());
            LastTime.Set();
            if (DEBUG) printf("put %s %s\n", (const char*)key, repeat ? "Repeat" : "");
            Put(key, repeat);
            char insert_char = tolower(key[4]); // remove "KEY_"
            if (DEBUG) printf("insert_char: %c\n", insert_char);
            Put((eKeys)(kKbd|insert_char<<16));
        }

        if (event.value == 0) { // release
            if (repeat) {
                /* send release */
                if (DEBUG) printf("release\n");
                if (DEBUG) printf("delta send: %ld\n", LastTime.Elapsed());
                LastTime.Set();
                if (DEBUG) printf("put %s Release\n", (const char *)lastkey);
                Put(lastkey, false, true);
                repeat = false;
            }
            lastkey = "";
        }
        if (DEBUG) printf("\n");
    }
  }
}

class cPluginUsbkbd : public cPlugin {
public:
  cPluginUsbkbd(void);
  virtual ~cPluginUsbkbd() override;
  virtual const char *Version(void) override { return VERSION; }
  virtual const char *Description(void) override { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void) override;
  virtual bool ProcessArgs(int argc, char *argv[]) override;
  virtual bool Start(void) override;
  };

cPluginUsbkbd::cPluginUsbkbd(void)
{
}

cPluginUsbkbd::~cPluginUsbkbd()
{
}

const char *cPluginUsbkbd::CommandLineHelp(void)
{
  return "  usbkbd event (/dev/input/eventX)\n"
         "  default /dev/usbkbd_event\n";
}

bool cPluginUsbkbd::ProcessArgs(int argc, char *argv[])
{
  if(argc > 1) usbkbd_device = argv[1];

  return true;
}

bool cPluginUsbkbd::Start(void)
{
  new cUsbkbdRemote("USBKBD");
  return true;
}


VDRPLUGINCREATOR(cPluginUsbkbd); // Don't touch this!
