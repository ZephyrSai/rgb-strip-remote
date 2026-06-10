# RGB Strip Remote — Web Bluetooth controller

A tiny, **install-free** web app that controls a BanlanX / SP-series Bluetooth RGB LED
strip straight from a Mac (or any computer with **Google Chrome / Edge / Brave**).
It replaces the *BanlanX* Android app — no phone needed, nothing to install.

Everything is a **single file** (`index.html`). It talks to the strip directly over
**Web Bluetooth**. No server, no account, no internet — your data never leaves the machine.

## 🧭 Two ways to run it

| | **A. Browser (Web Bluetooth)** | **B. ESP32 hub** |
|---|---|---|
| Where the Bluetooth happens | In your browser | On an ESP32 board |
| Works on | Chrome / Edge / Brave (desktop + Android) | **Any** browser, **incl. Safari & iPhone** |
| Needs | Just the browser | A ~$5 ESP32 board (one-time flash) |
| Best for | Quick control from a laptop | An always-on hub / phones that aren't Chrome |
| Setup | Open the link below | See **[esp32/README.md](esp32/README.md)** |

Option A is documented below. Option B lives in **[`esp32/`](esp32/)** — the ESP32 connects
to the strips over Bluetooth and serves the same control page over WiFi, so any phone or
laptop on the network can drive the lights.

---

## ✨ What it can do

- **Control one *or many* strips at once** — add multiple devices and every command is
  broadcast to all of them **in sync** (each strip keeps its own protocol; tick/untick a
  strip to include or exclude it from the group)
- Connect / disconnect strips over Bluetooth
- Power **ON / OFF**
- **Brightness** slider
- **Color** picker + quick swatches, with a **per-strip color-order fix** (RGB/GRB/BGR…) so
  mismatched strips can be made to show the same color
- **Effects** (rainbow, meteor, fire, comet, stars…) — for BanlanX SP6xxE controllers
- **Speed** and **effect length** sliders
- Try any **effect/mode number** the firmware supports
- **Advanced:** send raw hex commands (for testing captured commands)
- A live **log** of every command sent

BanlanX ships **several incompatible protocols** depending on the model, so the app lets you
pick one **per strip** (in the Devices list) and switch freely until it works — no reconnect
needed. Pick the default for new devices in the dropdown:

| Protocol option | Models |
|---|---|
| **BanlanX SP61x/62x** *(default)* | SP611E, SP617E, SP620E, SP621E |
| **BanlanX SP63x/64x** | SP630E, SP631E–SP639E, SP641E–SP649E (newer) |
| **BanlanX SP613/614** | SP613E, SP614E, SP623E, SP624E |
| **SP110E / SP107E** | SP110E, SP107E, SP105E |

**If colours look wrong or speed/effects do nothing, you're probably on the wrong protocol
for that strip** — change its **Protocol** dropdown and retry. Each strip also has a
**colour-order** (RGB/GRB/BGR…) and an **RGB/RGBW** dropdown (set RGBW if your strip has a
separate white wire, then use the **White channel** slider). You can mix different strips in
one sync group.

> **Speed is 1–10** and **effect length is 1–150** — these are the only values BanlanX
> controllers accept (sending anything larger is silently ignored, which is why speed used to
> appear broken).

---

## ✅ Requirements

- **A Chromium browser**: Google Chrome, Microsoft Edge, or Brave.
  > ❌ **Safari and Firefox do not support Web Bluetooth and will not work.**
- The computer must have **Bluetooth** turned on.
- On **macOS**: the first time you connect, macOS may ask to allow the browser to use
  Bluetooth. Allow it (or enable it in **System Settings → Privacy & Security → Bluetooth**).

> ❗️ **No iPhone / iPad.** iOS has no browser that supports Web Bluetooth. Use a
> computer (Mac/Windows/Linux/Chromebook) or an **Android** phone with Chrome.

---

## ⬇️ Install & run

There's nothing to "install" — it's a web page. Pick whichever is easiest:

### Option A — Open the hosted link (easiest, nothing to download)
Open this in **Chrome / Edge / Brave** on a device that has Bluetooth:

### 👉 https://zephyrsai.github.io/rgb-strip-remote/

Then click **Connect**. That's the whole setup. Send this link to your friend and they're
done — no files, no install.

> The page is *hosted* online, but control is **100% local**: your browser talks straight
> to the strip over your device's own Bluetooth radio. The internet is only used to load
> the page once — so the device must have Bluetooth and be **near the strip** (it is not
> remote-control over the internet).

### Option B — Download the files
1. On the GitHub page click **Code ▸ Download ZIP**, then unzip it.
2. Double-click **`index.html`** to open it in Chrome.

### Option C — Clone with git
```bash
git clone https://github.com/ZephyrSai/rgb-strip-remote.git
cd rgb-strip-remote
open -a "Google Chrome" index.html      # macOS (or just double-click index.html)
```

### Option D — Run a tiny local server (only if Connect is blocked on a `file://` page)
Double-click **`serve.command`** (macOS), or run from the project folder:
```bash
python3 -m http.server 8765
# then open http://localhost:8765 in Chrome
```

---

## 🚀 How to use (the 30-second version)

1. **Close the BanlanX app on your phone** (and any other device connected to the strip).
   The strip accepts only **one** Bluetooth connection at a time.
2. Open **`index.html`** in **Chrome** (double-click it, or drag it onto the Chrome icon).
3. Make sure the **protocol dropdown** matches your controller (leave it on *BanlanX* if unsure).
4. Click **➕ Add / Connect device** → pick your strip from the list → it appears in the
   **Devices** list with a name like `SP611E`.
   - Don’t see it? Tick **“Show all BLE devices”** and try again, then pick the one that
     looks like your strip.
5. Use the controls. 🎉

### Controlling several strips together

- Click **➕ Add / Connect device** again for each additional strip — they all join the
  **sync group**, and every control (power, brightness, color, effects, sliders) is sent
  to **all ticked strips at the same time**.
- Untick a strip's **sync** box to leave it out temporarily; tick it again to rejoin.
- Each row has its own protocol selector and an **✕** to disconnect just that strip;
  **Disconnect all** drops the whole group.

> **Note:** Bluetooth fans out the commands almost simultaneously, but it isn't
> frame-accurate — strips will match within a fraction of a second, not to the millisecond.

> **Tip — colors differ between strips, or look wrong on one:** each strip has its **own
> color-order dropdown** in the **Devices** list (RGB / GRB / BGR …). Strips can be wired
> differently, so the same color command can show as different colors. Change the odd one
> out (GRB is common for WS2812) until they match — it re-applies the current color instantly.

---

## 📦 How to send this to your friend

**Easiest:** send them the hosted link — <https://zephyrsai.github.io/rgb-strip-remote/> —
and tell them to open it in **Chrome** and click **Connect**. Nothing to download.

Prefer files? It’s just files in this folder — send the **whole folder** (or zip it):

```
banlanx/
├── index.html      ← the entire browser app (Option A)
├── serve.command   ← optional double-click launcher (see below)
├── esp32/          ← Option B: ESP32 hub firmware (works on any browser, incl. iPhone)
└── README.md       ← this file
```

Easiest path: **AirDrop / email the folder**, tell them to open `index.html` in Chrome.
That’s it. There is nothing to install or build.

### If double-clicking `index.html` doesn’t connect

A few Chrome setups block Web Bluetooth when a page is opened as a `file://` URL.
If **Connect** does nothing or errors, serve it over `localhost` instead:

- **macOS:** double-click **`serve.command`** (it starts a tiny local server and opens
  the app at `http://localhost:8765`). Leave that Terminal window open while using the app.
- **Or manually**, in Terminal from this folder:
  ```bash
  python3 -m http.server 8765
  ```
  then open <http://localhost:8765> in Chrome.

`localhost` is a “secure context”, which Web Bluetooth always allows.

---

## 🛠 Troubleshooting

| Problem | Fix |
|---|---|
| “This browser doesn’t support Web Bluetooth” banner | You’re in Safari/Firefox. Use Chrome/Edge/Brave. |
| Device doesn’t appear in the picker | Close the phone app first; tick **Show all BLE devices**; make sure the strip is powered. |
| Connects, but nothing/garbled happens | Wrong **protocol** for that strip — switch its Protocol dropdown (SP61x/62x → SP63x/64x → SP613/614) and retry. |
| Colours wrong vs the official app | Try a different **colour-order** (GRB is common). If it's an RGBW strip, set **RGBW** and check the **White channel** slider isn't washing it out. |
| Colours differ *between* strips | Set each strip's **colour-order** until they match. |
| Effect **speed**/length does nothing | Fixed — speed is 1–10, length 1–150. If still nothing, the strip is on the wrong protocol (see above). |
| Sliders feel laggy / drop commands | Normal for BLE; the app queues writes. Move sliders a bit slower. |
| Disconnects randomly | Keep the Mac near the strip; some clones have weak radios. Just click **Connect** again. |
| Buttons do nothing on macOS | Allow Bluetooth for your browser in **System Settings → Privacy & Security → Bluetooth**. |

---

## 🔌 Protocol reference (for the curious / for extending)

GATT (both protocols):
- **Service:** `0000ffe0-0000-1000-8000-00805f9b34fb`
- **Write characteristic:** `0000ffe1-0000-1000-8000-00805f9b34fb`

### BanlanX SP6xxE — format `A0 <cmd> <len> <data…>`
| Action | Bytes |
|---|---|
| Power ON | `A0 62 01 01` |
| Power OFF | `A0 62 01 00` |
| Brightness (0–255) | `A0 66 01 <bri>` |
| Effect speed | `A0 67 01 <speed>` |
| Effect length | `A0 68 01 <len>` |
| Effect type | `A0 63 01 <effect>` |
| Static color mode | `A0 63 01 BE` |
| Set color | `A0 69 04 <R> <G> <B> <bri>` |

### SP110E / SP107E — format `<d1> <d2> <d3> <cmd>`
| Action | Bytes |
|---|---|
| Power ON | `00 00 00 AA` |
| Power OFF | `00 00 00 AB` |
| Brightness (0–255) | `<bri> 00 00 2A` |
| Set color | `<R> <G> <B> 1E` |
| Effect / mode | `<mode> 00 00 2C` |
| Speed | `<speed> 00 00 03` |

> **Doesn’t match your device?** Some BanlanX models have firmware quirks. You can capture
> the real commands from the official Android app via an **HCI snoop log**
> (Developer Options → *Enable Bluetooth HCI snoop log*), open the capture in **Wireshark**,
> look at writes to handle `ffe1`, and replay them using the **Advanced → send raw hex** box.

---

## 🔒 Privacy

100% local. No analytics, no network calls, no accounts. The app only does two things:
talk to your LED strip over Bluetooth, and draw the UI.

## 🙏 Credits

Protocol details reverse-engineered by the community:
- BanlanX SP6xxE: [phhusson/ha-banlanx](https://github.com/phhusson/ha-banlanx),
  [monty68/uniled](https://github.com/monty68/uniled)
- SP110E: [SP110E protocol gist](https://gist.github.com/mbullington/37957501a07ad065b67d4e8d39bfe012),
  [nguyenthuongvo/Vox](https://github.com/nguyenthuongvo/Vox)

## License

MIT — do whatever you want with it.
