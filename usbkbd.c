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
#include <linux/input.h>
#include <locale.h>
#include <xkbcommon/xkbcommon.h>

static const char *VERSION        = "0.2.1";
static const char *DESCRIPTION    = tr("Send keypresses from an USB keyboard to VDR");

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
  void Abort(void);
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
  Abort();
}

bool cUsbkbdRemote::Connect()
{
  fd = open(usbkbd_device, O_RDONLY);
  if(fd == -1){
    if (DEBUG) printf("Cannot open %s. %s.\n", usbkbd_device, strerror(errno));
    esyslog("usbkbd: Cannot open %s. %s.\n", usbkbd_device, strerror(errno));
    return false;
  } else {
    if (DEBUG) printf("opened %s\n", usbkbd_device);
    isyslog("usbkbd: opened %s\n", usbkbd_device);
  }

  /*if(ioctl(fd, EVIOCGRAB, 1)){
    if (DEBUG) printf("Cannot grab %s. %s.\n", usbkbd_device, strerror(errno));
  } else {
    if (DEBUG) printf("Grabbed %s!\n", usbkbd_device);
  }*/

  return true;
}

bool cUsbkbdRemote::Ready(void)
{
  return fd >= 0;
}

void cUsbkbdRemote::Abort(void)
{
  Cancel();
  //ioctl(fd, EVIOCGRAB, 0);
  if (fd >= 0)
     close(fd);
  fd = -1;
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
  char str[16];
  int str_len = 0;

  struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!ctx) {
    esyslog("usbkbd: no xkb context");
    if (DEBUG) printf("usbkbd: no xkb context");
  }
  struct xkb_keymap *keymap = NULL;
  struct xkb_rule_names names = {
    .rules = getenv("XKB_DEFAULT_RULES"),
    .model = getenv("XKB_DEFAULT_MODEL"),
    .layout = getenv("XKB_DEFAULT_LAYOUT"),
    .variant = getenv("XKB_DEFAULT_VARIANT"),
    .options = getenv("XKB_DEFAULT_OPTIONS")
  };
  struct xkb_state *state = NULL;
  if (names.layout && *names.layout) {
    keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        esyslog("usbkbd: Wrong XKB_DEFAULT_* environment variables\n");
        if (DEBUG) printf("usbkbd: Wrong XKB_DEFAULT_* environment variables\n");
    }
  }
  if (!keymap) {
    names.rules = NULL;
    names.model = NULL;
    names.variant = NULL;
    const char* locale = I18nLocale(I18nCurrentLanguage());
    char vdr_lang[3] = {0};
    if (locale && strlen(locale) >= 2) {
        vdr_lang[0] = locale[0];
        vdr_lang[1] = locale[1];
        names.layout = vdr_lang;
        keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (keymap)
            isyslog("usbkbd: Fall back to vdr locale '%s' for keymap selection\n", vdr_lang);
    }
    if (!keymap) {
        names.layout = "us";
        keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (keymap)
            isyslog("usbkbd: Fall back to 'us' keymap\n");
    }
  }
  if (!keymap) {
    esyslog("usbkbd: Cannot create keymap\n");
    if (DEBUG) printf("usbkbd: Cannot create keymap\n");
    }
  else
    state = xkb_state_new(keymap);
  if (!state) {
    esyslog("usbkbd: Cannot create state\n");
    if (DEBUG) printf("usbkbd: Cannot create state\n");
    Abort();
    }

  if (DEBUG) printf("UsbkbdRemote action!\n");

  while(Running()) {
    while (access(usbkbd_device, F_OK) == -1) {
      if (connected) {
          connected = false;
          esyslog("usbkbd: no connection to %s, trying to reconnect every %.1f seconds", usbkbd_device, float(RECONNECTDELAY) / 1000);
          if (DEBUG) printf("no connection to %s, trying to reconnect every %.1f seconds\n", usbkbd_device, float(RECONNECTDELAY) / 1000);
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
            if (DEBUG) printf("reconnected to %s\n", usbkbd_device);
            //cCondWait::SleepMs(3); // wait a little after reconnect
        }
    }

    if (Ready() && read(fd, &event, sizeof(event)) != -1 && (event.type == EV_KEY)) {

        char buffer[64];
        xkb_keycode_t keycode = event.code + 8;
        if (event.value != 2)
            xkb_state_update_key(state, keycode, event.value ? XKB_KEY_DOWN : XKB_KEY_UP);
        xkb_keysym_t sym = xkb_state_key_get_one_sym(state, keycode);
        xkb_keysym_get_name(sym, buffer, sizeof(buffer));
        key = buffer;
        str_len = xkb_state_key_get_utf8(state, keycode, str, sizeof(str));

        int Delta = ThisTime.Elapsed(); // the time between two consecutive events
        if (DEBUG) printf("Delta: %d\n", Delta);
        ThisTime.Set();

        if (DEBUG) printf("key: %s, lastkey: %s  %s\n", (const char*)key, (const char*)lastkey, event.value == 0 ? "Release" : "");
        if (DEBUG) printf("utf8: %s 0x%08x length: %d sym: 0x%08x\n", (str_len > 0 && (unsigned char)str[0] >= 0x20 && (unsigned char)str[0] != 0x7F) ? str : "---", str[0], str_len, sym);

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
            if (DEBUG) printf("delta send: %ld\n", LastTime.Elapsed());
            LastTime.Set();
            if (DEBUG) printf("put %s %s\n", (const char*)key, repeat ? "Repeat" : "");

            if (!InEditMode()) {
                Put(key, repeat);
            } else {
                if (str_len > 0 && (sym & 0xFF) >= 0x20 && sym != 0x7F)
                        Put((eKeys)(kKbd|sym<<16));
                else if (sym == 0xfe52) // circumflex
                        Put((eKeys)(kKbd|0x5E<<16));
                else
                    Put(key, repeat); // control must work in edit mode, too, F1,F2,F3,F4 have str_len 0, Backspace and Return are below 0x20
            }
        }

        if (event.value == 0) { // release
            if (repeat) {
                /* send release */
                if (DEBUG) printf("release\ndelta send: %ld\nput %s Release\n", LastTime.Elapsed(), (const char *)lastkey);
                LastTime.Set();
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
