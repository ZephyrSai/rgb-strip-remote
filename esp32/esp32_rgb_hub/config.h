#pragma once
// =====================================================================
//  EDIT THIS FILE — it's the only part you normally need to change.
// =====================================================================

// ---------------------------- WiFi -----------------------------------
// USE_SOFTAP = 1 : the ESP32 creates its OWN hotspot. Join it from your
//                  phone/laptop, then open  http://192.168.4.1
//                  (Best for "send it to a friend" — no router needed.)
// USE_SOFTAP = 0 : the ESP32 joins your existing WiFi. Open the IP it
//                  prints on the Serial Monitor, or http://rgb-hub.local
#define USE_SOFTAP 1

// Hotspot created when USE_SOFTAP == 1  (password must be >= 8 chars)
#define AP_SSID "RGB-Hub"
#define AP_PASS "rgblights"

// Your home WiFi, used when USE_SOFTAP == 0
#define STA_SSID "YOUR_WIFI_NAME"
#define STA_PASS "YOUR_WIFI_PASSWORD"

// --------------------------- Strips ----------------------------------
// One entry per LED strip you want to control.
//   name    : connect to the first strip whose advertised name STARTS WITH
//             this text (e.g. "SP" matches "SP611E"). Leave "" to ignore.
//   addr    : OR match an exact MAC address "aa:bb:cc:dd:ee:ff"
//             (most reliable when you have several identical strips).
//             Leave "" to match by name instead.
//   profile : starting protocol (you can also change it live in the web page):
//             "sp61x"  -> SP611E / SP617E / SP620E / SP621E
//             "sp63x"  -> SP630E / SP63xE / SP64xE (newer)
//             "sp613"  -> SP613E / SP614E / SP623E / SP624E
//             "sp110e" -> SP110E / SP107E
//             (if unsure, start with "sp61x" and try others from the web UI)
//
// Every command from the web page is sent to ALL connected strips at once.
// Colour-order and RGB/RGBW are chosen live in the web page (default RGB).
//
// NOTE: NimBLE allows 3 simultaneous BLE connections by default. To control
//       more than 3 strips you must raise CONFIG_BT_NIMBLE_MAX_CONNECTIONS
//       in the library's nimconfig.h (see esp32/README.md).
struct StripCfg { const char* name; const char* addr; const char* profile; };

static StripCfg STRIPS[] = {
  { "SP", "", "sp61x" },            // strip #1: first device named "SP..."
  // { "SP", "", "sp63x" },         // strip #2: another "SP..." device
  // { "",   "a4:c1:38:aa:bb:cc", "sp110e" },  // by MAC
};

// How long (ms) each Bluetooth scan runs when (re)connecting.
#define SCAN_MS 5000
