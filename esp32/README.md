# ESP32 RGB Strip Hub

Run the controller **on an ESP32** instead of in the browser's Bluetooth stack.

The ESP32 connects to your BanlanX / SP-series strips over Bluetooth **and** serves the
control web page over WiFi. Anyone on the same network just opens the ESP32's address in a
browser — so unlike the browser-only version, this works on **any** browser, **including
Safari and iPhones/iPads**.

```
 Browser version:  [phone/laptop] --BLE--> [strips]              (Chrome/Edge only, no iPhone)
 ESP32 version:    [any browser]  --WiFi--> [ESP32] --BLE--> [strips]   (works on iPhone too)
```

It keeps the same features: power, brightness, color (+ color-order fix), effects, speed,
effect length, raw hex — and every command is sent to **all connected strips at once**.

---

## What you need

- An **ESP32** dev board (classic ESP32 / ESP32-WROOM — the common ~$5 boards). A board
  with Bluetooth **and** WiFi; the plain ESP32 has both. *(ESP8266 won't work — no BLE.)*
- A USB cable and the **Arduino IDE** (2.x).

---

## One-time setup

### 1. Add ESP32 board support to Arduino IDE
- **Arduino IDE ▸ Settings ▸ Additional Boards Manager URLs**, add:
  `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
- **Tools ▸ Board ▸ Boards Manager**, search **esp32**, install **“esp32 by Espressif”**
  (v3.x recommended).

### 2. Install the BLE library
- **Tools ▸ Manage Libraries**, search **NimBLE-Arduino** (by *h2zero*), install it
  (**v2.x** — this firmware targets the 2.x API).

### 3. Open the sketch
- Open **`esp32_rgb_hub/esp32_rgb_hub.ino`** in Arduino IDE. (`config.h` opens as a tab.)

---

## Configure (`config.h`)

This is the only file you normally edit.

1. **WiFi mode** — pick one:
   - `#define USE_SOFTAP 1` → the ESP32 makes its **own hotspot** (`RGB-Hub` / `rgblights`).
     Best for handing to a friend — no router needed. After connecting to that hotspot you
     open **http://192.168.4.1**.
   - `#define USE_SOFTAP 0` → the ESP32 **joins your WiFi** (set `STA_SSID` / `STA_PASS`).
     You then open the IP it prints on Serial, or **http://rgb-hub.local**.

2. **Your strips** — one line per strip in the `STRIPS[] = { ... }` list:
   ```c
   { "SP", "", "banlanx" },                  // match first device named "SP..."
   { "",   "a4:c1:38:aa:bb:cc", "sp110e" },  // OR match an exact MAC, SP110E protocol
   ```
   - `name` = advertised-name prefix, **or** `addr` = exact MAC (more reliable for multiple
     identical strips). Use one or the other.
   - `profile` = `"banlanx"` (SP6xxE / the BanlanX app) or `"sp110e"` (SP110E/SP107E).

   > **Tip — finding the MAC:** upload first with a name match, open **Serial Monitor**
   > (115200 baud); the firmware prints the name + MAC of each strip it connects to. Copy
   > the MAC into `config.h` for rock-solid matching.

---

## Upload & run

1. **Close the BanlanX phone app** (each strip allows only one Bluetooth connection).
2. Plug in the ESP32. **Tools ▸ Board** → your ESP32 model; **Tools ▸ Port** → its port.
3. Click **Upload (→)**.
4. Open **Tools ▸ Serial Monitor** at **115200** baud — it prints the WiFi address and which
   strips connected.
5. On your phone/laptop:
   - SoftAP mode: join WiFi **`RGB-Hub`** (password `rgblights`), open **http://192.168.4.1**
   - Home-WiFi mode: open the printed IP (or **http://rgb-hub.local**)
6. Control your strips. 🎉  The page auto-refreshes connection status; **↻ Rescan /
   reconnect** re-scans for any offline strip.

---

## Notes & limits

- **3 strips max by default.** NimBLE allows 3 simultaneous BLE connections out of the box.
  For more, edit the library's `nimconfig.h` and raise
  `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` (e.g. to 5–9), then re-upload. RAM permitting.
- **WiFi + BLE share one radio.** The ESP32 does both, but heavy WiFi traffic during a BLE
  scan can briefly slow things. Reconnect scans pause the web page for ~5s — normal.
- **Range:** put the ESP32 within good Bluetooth range of the strips; the WiFi side can be
  whatever your network/hotspot reaches.
- This firmware is written for **NimBLE-Arduino 2.x** + **ESP32 Arduino core 3.x**. On other
  versions a couple of BLE API calls may need minor tweaks (the compiler will point them out).
- Same protocol bytes as the browser app — see the protocol tables in the
  [main README](../README.md#-protocol-reference-for-the-curious--for-extending).

## Files
```
esp32/
├── README.md                     ← this file
└── esp32_rgb_hub/
    ├── esp32_rgb_hub.ino         ← firmware + embedded web page
    └── config.h                  ← WiFi + your strips (edit this)
```
