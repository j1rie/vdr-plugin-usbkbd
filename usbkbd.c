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
#include <getopt.h>
#include <xkbcommon/xkbcommon.h>

static const char *VERSION        = "0.2.0";
static const char *DESCRIPTION    = tr("Send keypresses from an USB keyboard to VDR");

#define DEBUG 0
#define RECONNECTDELAY 3000 // ms

const char* usbkbd_device = "/dev/usbkbd_event";
bool letter_detect = false;
bool keypad_numbers = false;

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
  bool letter = false;

  struct xkb_keymap *keymap = NULL;
  struct xkb_state *state = NULL;
  struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_rule_names names = {
    .rules = getenv("XKB_DEFAULT_RULES"),
    .model = getenv("XKB_DEFAULT_MODEL"),
    .layout = getenv("XKB_DEFAULT_LAYOUT"),
    .variant = getenv("XKB_DEFAULT_VARIANT"),
    .options = getenv("XKB_DEFAULT_OPTIONS")
  };
  if (names.layout && *names.layout) {
      keymap = xkb_keymap_new_from_names(ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
      if (!keymap)
        esyslog("usbkbd: Wrong XKB_DEFAULT_* environment variables\n");
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
  if (!keymap)
      esyslog("usbkbd: Cannot create keymap\n");
  else
      state = xkb_state_new(keymap);

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

        char buffer[64];
        xkb_keycode_t xkb_code = event.code + 8;
        if (state) {
          if (event.value != 2)
            xkb_state_update_key(state, xkb_code, event.value ? XKB_KEY_DOWN : XKB_KEY_UP);
          xkb_keysym_get_name(xkb_state_key_get_one_sym(state, xkb_code), buffer, sizeof(buffer));
          key = buffer;
        }

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
            char str[16];
            int str_len = xkb_state_key_get_utf8(state, xkb_code, str, sizeof(str));
            if(DEBUG) printf("delta send: %ld\n", LastTime.Elapsed());
            LastTime.Set();
            if (DEBUG) printf("put %s %s\n", (const char*)key, repeat ? "Repeat" : "");
            if (InEditMode() && state) {
                if (str_len > 0 && ((unsigned char)str[0]) >= 32 && ((unsigned char)str[0]) != 127) {
                    if (DEBUG) printf("Zeichen: %s %d, Name: %s, keypad_numbers: %d, letter_detect: %d, letter: %d\n", str, (unsigned char)str[0], (const char*)key, keypad_numbers, letter_detect, letter);
                    if (str[0] >= '0' && str[0] <= '9' &&
                        ((keypad_numbers && strncmp(key, "KP_", 3) == 0) || 
                         (!keypad_numbers && strncmp(key, "KP_", 3) != 0 && (!letter || !letter_detect))))
                        Put(str, repeat);
                    else {
                        letter = true;
                        if (str_len == 1)
                            Put((eKeys)(kKbd|str[0]<<16));
                        else if (str_len == 2) {
                            xkb_keysym_t sym = xkb_state_key_get_one_sym(state, xkb_code);
                            Put((eKeys)(kKbd|sym << 16));
                        }
                    }
                }
                else {
                    if (strcmp(key, "Left")   &&
                        strcmp(key, "Right")  &&
                        strcmp(key, "Delete") &&
                        strcmp(key, "Insert"))
                        letter = false;
                    Put(key, repeat);
                }
            } else {
                letter = false;
                if (keypad_numbers && strncmp(key, "KP_", 3) == 0 && str[0] >= '0' && str[0] <= '9')
                    Put(str, repeat);
                else
                    Put(key, repeat);
            }
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
  return "  -d DEVICE, --device=DEVICE     device to read events from (/dev/input/eventX)\n"
         "                                 default is /dev/usbkbd_event\n"
         "  -l,        --letterdetection   detect if a keyboard is used to enter texts\n"
         "  -k,        --keypadnumbers     use keypad numbers to enter letters by remote control\n";
}

bool cPluginUsbkbd::ProcessArgs(int argc, char *argv[])
{
  int c, i;
  const char *short_options = "d:lk";
  const struct option long_options[] = {
    { "device",          required_argument, NULL, 'd' },
    { "letterdetection", no_argument,       NULL, 'l' },
    { "keypadnumbers",   no_argument,       NULL, 'k' },
    { NULL,              0,                 NULL,  0  }
  };

  while ((c = getopt_long(argc, argv, short_options, long_options, &i)) != -1) {
    switch (c) {
      case 'd':
            usbkbd_device = optarg;
            break;
      case 'l':
            letter_detect = true;
            break;
      case 'k':
            keypad_numbers = true;
            break;
      default:
            return false;
    }
  }
  return true;
}

bool cPluginUsbkbd::Start(void)
{
  new cUsbkbdRemote("USBKBD");
  return true;
}


VDRPLUGINCREATOR(cPluginUsbkbd); // Don't touch this!
