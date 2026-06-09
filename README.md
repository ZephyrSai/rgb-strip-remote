# RGB Strip Remote — Web Bluetooth controller

A tiny, **install-free** web app that controls a BanlanX / SP-series Bluetooth RGB LED
strip straight from a Mac (or any computer with **Google Chrome / Edge / Brave**).
It replaces the *BanlanX* Android app — no phone needed, nothing to install.

Everything is a **single file** (`index.html`). It talks to the strip directly over
**Web Bluetooth**. No server, no account, no internet — your data never leaves the machine.

---

## ✨ What it can do

- **Control one *or many* strips at once** — add multiple devices and every command is
  broadcast to all of them **in sync** (each strip keeps its own protocol; tick/untick a
  strip to include or exclude it from the group)
- Connect / disconnect strips over Bluetooth
- Power **ON / OFF**
- **Brightness** slider
- **Color** picker + quick swatches (with a color-order fix if R/G/B look swapped)
- **Effects** (rainbow, meteor, fire, comet, stars…) — for BanlanX SP6xxE controllers
- **Speed** and **effect length** sliders
- Try any **effect/mode number** the firmware supports
- **Advanced:** send raw hex commands (for testing captured commands)
- A live **log** of every command sent

It supports **two protocols**. Pick the one for the device you're about to add in the
dropdown; after it's connected you can also change it per-device from the row's own selector:

| Protocol option | Use it if… | Devices |
|---|---|---|
| **BanlanX — SP6xxE** *(default)* | You use the app literally called **“BanlanX”** | SP611E, SP617E, SP621E, SP628E, SP630E … |
| **SP110E / SP107E** | You use the older *SP110E* / *LED Hue* style app | SP110E, SP107E, SP105E … |

Both use the same Bluetooth service, so if a strip doesn’t respond, just switch its
protocol selector and try the other — no reconnect needed. You can even mix strips of
different types in the same sync group.

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

> **Tip:** If the colors look wrong (e.g. red shows as green), change **Color order**
> from `RGB` to `GRB` (most WS2812 strips) and re-apply the color.

---

## 📦 How to send this to your friend

**Easiest:** send them the hosted link — <https://zephyrsai.github.io/rgb-strip-remote/> —
and tell them to open it in **Chrome** and click **Connect**. Nothing to download.

Prefer files? It’s just files in this folder — send the **whole folder** (or zip it):

```
banlanx/
├── index.html      ← the entire app
├── serve.command   ← optional double-click launcher (see below)
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
| Connects, but nothing happens | Wrong protocol — switch the dropdown (BanlanX ⇄ SP110E) and retry a button. |
| Colors are swapped | Change **Color order** (try `GRB`). |
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
