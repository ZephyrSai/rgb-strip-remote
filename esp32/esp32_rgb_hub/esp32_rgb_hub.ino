// =====================================================================
//  ESP32 RGB Strip Hub
//  WiFi  -> serves a control web page (works on ANY browser, incl. iPhone)
//  BLE   -> connects to BanlanX / SP-series strips and relays commands
//
//  Supports the same selectable protocols as the browser app:
//    sp61x  (SP611E/617E/620E/621E)   sp63x (SP630E/SP63x/SP64x)
//    sp613  (SP613E/614E/623E/624E)   sp110e (SP110E/SP107E)
//  Each strip's protocol / colour-order / RGB-RGBW can be changed live
//  from the web page, so you can test until colours + speed work.
//
//  Libraries required (Arduino IDE Library Manager):
//    - NimBLE-Arduino  by h2zero   (v2.x)
//  Board: any ESP32 dev board. Core v3.x recommended.
//  Edit config.h for WiFi + your strips, then Upload. See esp32/README.md.
// =====================================================================
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NimBLEDevice.h>
#include "config.h"

static const char* SVC_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb";
static const char* CHR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb";

enum Profile { P_SP61X, P_SP63X, P_SP613, P_SP110E };

static Profile parseProfile(const char* s) {
  if (!strcmp(s, "sp63x"))  return P_SP63X;
  if (!strcmp(s, "sp613"))  return P_SP613;
  if (!strcmp(s, "sp110e")) return P_SP110E;
  return P_SP61X;                                 // "sp61x" / "banlanx" / default
}
static const char* profileName(Profile p) {
  switch (p) { case P_SP63X: return "sp63x"; case P_SP613: return "sp613";
               case P_SP110E: return "sp110e"; default: return "sp61x"; }
}

static const char* ORDER_NAMES[6] = { "rgb","grb","bgr","rbg","gbr","brg" };
static uint8_t orderIdx(const String& s) {
  for (uint8_t i = 0; i < 6; i++) if (s == ORDER_NAMES[i]) return i;
  return 0;
}
// reorder r,g,b into out[3] per order index (matches the browser app)
static void applyOrder(uint8_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t* out) {
  static const uint8_t MAP[6][3] = {{0,1,2},{1,0,2},{2,1,0},{0,2,1},{1,2,0},{2,0,1}};
  uint8_t v[3] = { r, g, b };
  if (idx > 5) idx = 0;
  out[0] = v[MAP[idx][0]]; out[1] = v[MAP[idx][1]]; out[2] = v[MAP[idx][2]];
}
static uint8_t clampU(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct Strip {
  StripCfg*                   cfg      = nullptr;
  Profile                     profile  = P_SP61X;
  uint8_t                     order    = 0;       // colour-order index
  bool                        rgbw     = false;
  NimBLEClient*               client   = nullptr;
  NimBLERemoteCharacteristic* chr      = nullptr;
  bool                        connected = false;
  std::string                 addr;
  std::string                 devName;
};

static const size_t NUM_STRIPS = sizeof(STRIPS) / sizeof(STRIPS[0]);
Strip strips[sizeof(STRIPS) / sizeof(STRIPS[0])];
WebServer server(80);

// =====================================================================
//  Low-level writes
// =====================================================================
static void writeStrip(Strip& s, const uint8_t* data, size_t len) {
  if (s.connected && s.chr) s.chr->writeValue((uint8_t*)data, len, false);
}
// SP63x frame:  53 <cmd> 00 01 00 <len> <data...>
static void f6write(Strip& s, uint8_t cmd, const uint8_t* data, uint8_t len) {
  uint8_t buf[16] = { 0x53, cmd, 0x00, 0x01, 0x00, len };
  for (uint8_t i = 0; i < len && i < 10; i++) buf[6 + i] = data[i];
  writeStrip(s, buf, 6 + len);
}

// =====================================================================
//  Logical commands -> per-strip protocol encoding (broadcast to all)
// =====================================================================
static void doPower(bool on) {
  for (auto& s : strips) { if (!s.connected) continue;
    switch (s.profile) {
      case P_SP61X:  { uint8_t d[]={0xA0,0x62,0x01,(uint8_t)(on?1:0)}; writeStrip(s,d,4); } break;
      case P_SP63X:  { uint8_t d[]={(uint8_t)(on?1:0)}; f6write(s,0x50,d,1); } break;
      case P_SP613:  { uint8_t d[]={0x0F,0x01,(uint8_t)(on?1:0)}; writeStrip(s,d,3); } break;
      case P_SP110E: { uint8_t d[]={0,0,0,(uint8_t)(on?0xAA:0xAB)}; writeStrip(s,d,4); } break;
    }
  }
}
static void doBrightness(uint8_t v) {
  for (auto& s : strips) { if (!s.connected) continue;
    switch (s.profile) {
      case P_SP61X:  { uint8_t d[]={0xA0,0x66,0x01,v}; writeStrip(s,d,4); } break;
      case P_SP63X:  { uint8_t d[]={0x00,v}; f6write(s,0x51,d,2); } break;
      case P_SP613:  { uint8_t d[]={0x12,0x01,v}; writeStrip(s,d,3); } break;
      case P_SP110E: { uint8_t d[]={v,0,0,0x2A}; writeStrip(s,d,4); } break;
    }
  }
}
static void doSpeed(uint8_t v) {            // v is 1..10 from the slider
  for (auto& s : strips) { if (!s.connected) continue;
    switch (s.profile) {
      case P_SP61X:  { uint8_t d[]={0xA0,0x67,0x01,clampU(v,1,10)}; writeStrip(s,d,4); } break;
      case P_SP63X:  { uint8_t d[]={clampU(v,1,10)}; f6write(s,0x54,d,1); } break;
      case P_SP613:  { uint8_t d[]={0x14,0x01,clampU(v,1,10)}; writeStrip(s,d,3); } break;
      case P_SP110E: { uint8_t d[]={clampU(v*255/10,1,255),0,0,0x03}; writeStrip(s,d,4); } break;
    }
  }
}
static void doLength(uint8_t v) {           // v is 1..150
  for (auto& s : strips) { if (!s.connected) continue;
    switch (s.profile) {
      case P_SP61X: { uint8_t d[]={0xA0,0x68,0x01,clampU(v,1,150)}; writeStrip(s,d,4); } break;
      case P_SP63X: { uint8_t d[]={clampU(v,1,150)}; f6write(s,0x55,d,1); } break;
      default: break;                       // SP613 / SP110E: no length
    }
  }
}
static void doEffect(uint8_t code) {
  for (auto& s : strips) { if (!s.connected) continue;
    switch (s.profile) {
      case P_SP61X:  { uint8_t d[]={0xA0,0x63,0x01,code}; writeStrip(s,d,4); } break;
      case P_SP63X:  { uint8_t d[]={0x03,code}; f6write(s,0x53,d,2); } break;   // mode 3 = dynamic
      case P_SP613:  { uint8_t d[]={0x15,0x01,code}; writeStrip(s,d,3); } break;
      case P_SP110E: { uint8_t d[]={code,0,0,0x2C}; writeStrip(s,d,4); } break;
    }
  }
}
static void doColor(uint8_t r, uint8_t g, uint8_t b, uint8_t level) {
  for (auto& s : strips) { if (!s.connected) continue;
    uint8_t o[3]; applyOrder(s.order, r, g, b, o);
    switch (s.profile) {
      case P_SP61X:  { uint8_t m[]={0xA0,0x63,0x01,0xBE}; writeStrip(s,m,4);
                       uint8_t d[]={0xA0,0x69,0x04,o[0],o[1],o[2],level}; writeStrip(s,d,7); } break;
      case P_SP63X:  { uint8_t m[]={0x01,0x01}; f6write(s,0x53,m,2);
                       uint8_t d[]={o[0],o[1],o[2],level}; f6write(s,0x52,d,4); } break;
      case P_SP613:  { uint8_t m[]={0x15,0x01,0x63}; writeStrip(s,m,3);
                       uint8_t d[]={0x13,0x04,o[0],o[1],o[2],level}; writeStrip(s,d,6); } break;
      case P_SP110E: { uint8_t d[]={o[0],o[1],o[2],0x1E}; writeStrip(s,d,4); } break;
    }
  }
}
static void doWhite(uint8_t w) {            // only RGBW-flagged strips
  for (auto& s : strips) { if (!s.connected || !s.rgbw) continue;
    switch (s.profile) {
      case P_SP61X:  { uint8_t d[]={0xA0,0x76,0x02,w,0x00}; writeStrip(s,d,5); } break;
      case P_SP63X:  { uint8_t d[]={0x01,w}; f6write(s,0x51,d,2); } break;
      case P_SP613:  { uint8_t d[]={0x21,0x02,w,0xFF}; writeStrip(s,d,4); } break;
      default: break;
    }
  }
}
static void doRaw(const uint8_t* data, size_t len) { for (auto& s : strips) writeStrip(s, data, len); }

// =====================================================================
//  BLE connect / reconnect
// =====================================================================
static bool ieq(const std::string& a, const char* b) {
  std::string s(b);
  if (a.size() != s.size()) return false;
  for (size_t i = 0; i < a.size(); i++) if (tolower(a[i]) != tolower(s[i])) return false;
  return true;
}
static bool addrTaken(const std::string& a) {
  for (auto& s : strips) if (s.connected && s.addr == a) return true;
  return false;
}
static bool matchCfg(StripCfg* c, const NimBLEAdvertisedDevice* d) {
  if (c->addr && strlen(c->addr) > 0) return ieq(d->getAddress().toString(), c->addr);
  if (c->name && strlen(c->name) > 0) {
    std::string n = d->getName();
    return !n.empty() && n.rfind(c->name, 0) == 0;
  }
  return false;
}
static bool tryConnect(int i, const NimBLEAdvertisedDevice* d) {
  if (!strips[i].client) strips[i].client = NimBLEDevice::createClient();
  NimBLEClient* c = strips[i].client;
  if (!c->isConnected() && !c->connect(d)) return false;
  NimBLERemoteService* svc = c->getService(SVC_UUID);
  if (!svc) { c->disconnect(); return false; }
  NimBLERemoteCharacteristic* ch = svc->getCharacteristic(CHR_UUID);
  if (!ch) { c->disconnect(); return false; }
  strips[i].chr = ch; strips[i].connected = true;
  strips[i].addr = d->getAddress().toString();
  strips[i].devName = d->getName();
  return true;
}
static void connectStrips() {
  Serial.println("[BLE] scanning...");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  NimBLEScanResults res = scan->getResults(SCAN_MS, false);
  int count = res.getCount();
  for (size_t i = 0; i < NUM_STRIPS; i++) {
    if (strips[i].connected) continue;
    for (int j = 0; j < count; j++) {
      const NimBLEAdvertisedDevice* d = res.getDevice(j);
      if (addrTaken(d->getAddress().toString())) continue;
      if (matchCfg(strips[i].cfg, d)) {
        Serial.printf("[BLE] strip %d -> %s (%s) ... ", (int)i,
                      d->getName().c_str(), d->getAddress().toString().c_str());
        Serial.println(tryConnect((int)i, d) ? "connected" : "FAILED");
        break;
      }
    }
  }
  scan->clearResults();
}

// =====================================================================
//  Web server
// =====================================================================
extern const char INDEX_HTML[] PROGMEM;
static uint8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = tolower(c);
  return (c >= 'a' && c <= 'f') ? c - 'a' + 10 : 0;
}

static void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

static void handleStatus() {
  String j = "[";
  for (size_t i = 0; i < NUM_STRIPS; i++) {
    if (i) j += ",";
    const char* match = (STRIPS[i].name && strlen(STRIPS[i].name)) ? STRIPS[i].name : STRIPS[i].addr;
    j += "{\"match\":\"";     j += match ? match : "";
    j += "\",\"name\":\"";    j += strips[i].devName.c_str();
    j += "\",\"profile\":\""; j += profileName(strips[i].profile);
    j += "\",\"order\":\"";   j += ORDER_NAMES[strips[i].order];
    j += "\",\"rgbw\":";      j += (strips[i].rgbw ? "true" : "false");
    j += ",\"connected\":";   j += (strips[i].connected ? "true" : "false");
    j += "}";
  }
  j += "]";
  server.send(200, "application/json", j);
}

static void handlePower()  { doPower(server.arg("on") == "1"); server.send(200,"text/plain","ok"); }
static void handleBri()    { doBrightness((uint8_t)server.arg("v").toInt()); server.send(200,"text/plain","ok"); }
static void handleWhite()  { doWhite((uint8_t)server.arg("v").toInt()); server.send(200,"text/plain","ok"); }
static void handleSpeed()  { doSpeed((uint8_t)server.arg("v").toInt()); server.send(200,"text/plain","ok"); }
static void handleLength() { doLength((uint8_t)server.arg("v").toInt()); server.send(200,"text/plain","ok"); }
static void handleEffect() { doEffect((uint8_t)server.arg("code").toInt()); server.send(200,"text/plain","ok"); }

static void handleColor() {
  long v = strtol(server.arg("hex").c_str(), nullptr, 16);   // "rrggbb"
  uint8_t bri = server.arg("bri").length() ? (uint8_t)server.arg("bri").toInt() : 0xFF;
  doColor((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff, bri);
  server.send(200, "text/plain", "ok");
}
static void handleSetcfg() {
  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= (int)NUM_STRIPS) { server.send(400, "text/plain", "bad idx"); return; }
  if (server.hasArg("profile")) strips[idx].profile = parseProfile(server.arg("profile").c_str());
  if (server.hasArg("order"))   strips[idx].order   = orderIdx(server.arg("order"));
  if (server.hasArg("rgbw"))    strips[idx].rgbw    = (server.arg("rgbw") == "1");
  server.send(200, "text/plain", "ok");
}
static void handleRaw() {
  String h = server.arg("hex");
  uint8_t buf[40]; size_t n = 0;
  for (size_t i = 0; i + 1 < (size_t)h.length() && n < sizeof(buf); ) {
    if (!isxdigit(h[i])) { i++; continue; }
    buf[n++] = (hexNibble(h[i]) << 4) | hexNibble(h[i + 1]); i += 2;
  }
  if (n) doRaw(buf, n);
  server.send(200, "text/plain", n ? "ok" : "no bytes");
}
static void handleReconnect() { connectStrips(); server.send(200, "text/plain", "ok"); }

static void startServer() {
  server.on("/",               handleRoot);
  server.on("/api/status",     handleStatus);
  server.on("/api/power",      handlePower);
  server.on("/api/brightness", handleBri);
  server.on("/api/white",      handleWhite);
  server.on("/api/speed",      handleSpeed);
  server.on("/api/length",     handleLength);
  server.on("/api/effect",     handleEffect);
  server.on("/api/color",      handleColor);
  server.on("/api/setcfg",     handleSetcfg);
  server.on("/api/raw",        handleRaw);
  server.on("/api/reconnect",  handleReconnect);
  server.begin();
  Serial.println("[WEB] server started on port 80");
}

// =====================================================================
//  WiFi
// =====================================================================
static void startWifi() {
#if USE_SOFTAP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WIFI] hotspot '%s'  ->  http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
#else
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASS);
  Serial.printf("[WIFI] joining '%s'", STA_SSID);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI] connected  ->  http://%s   (or http://rgb-hub.local)\n",
                  WiFi.localIP().toString().c_str());
    if (MDNS.begin("rgb-hub")) MDNS.addService("http", "tcp", 80);
  } else {
    Serial.println("[WIFI] FAILED — check STA_SSID/STA_PASS in config.h");
  }
#endif
}

// =====================================================================
//  Arduino entry points
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ESP32 RGB Strip Hub ===");
  for (size_t i = 0; i < NUM_STRIPS; i++) {
    strips[i].cfg     = &STRIPS[i];
    strips[i].profile = parseProfile(STRIPS[i].profile);
  }
  startWifi();
  NimBLEDevice::init("rgb-hub");
  connectStrips();
  startServer();
}

void loop() {
  server.handleClient();
  for (auto& s : strips) s.connected = (s.client && s.client->isConnected());
  static uint32_t last = 0;
  bool anyDown = false;
  for (auto& s : strips) if (!s.connected) anyDown = true;
  if (anyDown && millis() - last > 15000) { last = millis(); connectStrips(); }
  delay(5);
}

// =====================================================================
//  Embedded control page (served at  /  )
// =====================================================================
const char INDEX_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#0b0d12"><title>RGB Strip Hub (ESP32)</title>
<style>
:root{--bg:#0b0d12;--card:#161a23;--card2:#1d2230;--line:#2a3142;--text:#e7ebf3;--muted:#97a0b5;--accent:#6ea8fe;--good:#38d39f;--bad:#ff6b6b}
*{box-sizing:border-box}body{margin:0;padding:18px;background:radial-gradient(1200px 600px at 50% -10%,#1a2030,var(--bg));color:var(--text);font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;min-height:100vh}
.wrap{max-width:720px;margin:0 auto}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:16px}
h1{font-size:20px;margin:0}h1 small{color:var(--muted);font-weight:400;font-size:12px;display:block;margin-top:2px}
.status{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--muted)}
.dot{width:10px;height:10px;border-radius:50%;background:var(--bad);box-shadow:0 0 10px var(--bad);flex:none}
.dot.on{background:var(--good);box-shadow:0 0 10px var(--good)}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:16px;margin-bottom:14px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);margin:0 0 12px}
.row{display:flex;gap:10px;flex-wrap:wrap;align-items:center}.row+.row{margin-top:12px}
label.field{display:block}label.field span{display:block;font-size:12px;color:var(--muted);margin-bottom:6px}
button{appearance:none;border:1px solid var(--line);background:var(--card2);color:var(--text);padding:10px 14px;border-radius:10px;font-size:14px;cursor:pointer}
button:hover{border-color:var(--accent)}button.good{color:var(--good)}button.danger{color:var(--bad)}
select,input[type=number],input[type=text]{background:var(--card2);color:var(--text);border:1px solid var(--line);border-radius:10px;padding:9px 10px;font-size:14px}
select.ds{padding:5px 7px;font-size:12px;border-radius:8px}
input[type=range]{width:100%;accent-color:var(--accent)}
input[type=color]{width:64px;height:44px;padding:0;border:1px solid var(--line);border-radius:10px;background:var(--card2)}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:8px}
.swatches{display:flex;gap:8px;flex-wrap:wrap}.swatch{width:34px;height:34px;border-radius:8px;border:1px solid var(--line);cursor:pointer;padding:0}
.devrow{display:flex;align-items:center;gap:8px;flex-wrap:wrap;padding:8px 10px;border:1px solid var(--line);border-radius:10px;background:var(--card2);margin-top:8px}
.devrow .devname{flex:1;min-width:110px;font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.muted{color:var(--muted);font-size:12px}.val{min-width:42px;text-align:right;color:var(--muted)}
#log{background:#0a0c11;border:1px solid var(--line);border-radius:10px;font:12px/1.5 ui-monospace,Menlo,monospace;color:#9fb0c9;padding:10px;height:110px;overflow:auto;white-space:pre-wrap}
</style></head><body><div class="wrap">
<header><h1>RGB Strip Hub <small>served by ESP32 · works on any browser</small></h1>
<div class="status"><span id="dot" class="dot"></span><span id="statusText">…</span></div></header>

<div class="card"><h2>Strips</h2><div id="devs"></div>
<div class="row" style="margin-top:12px"><button id="reconnect">↻ Rescan / reconnect</button></div>
<p class="muted" style="margin:12px 0 0">Strips are matched in <code>config.h</code>. Set each strip's <b>protocol</b>, <b>colour-order</b> and <b>RGB/RGBW</b> here until colours + speed work. Every control below is sent to all connected strips at once.</p></div>

<div class="card"><h2>Controls</h2>
<div class="row"><button id="on" class="good">⏻ ON</button><button id="off" class="danger">⏻ OFF</button></div>
<div class="row"><label class="field" style="flex:1"><span>Brightness <span id="briVal" class="val"></span></span><input type="range" id="bri" min="0" max="255" value="200"></label></div>
<div class="row" style="align-items:flex-end"><label class="field"><span>Color</span><input type="color" id="color" value="#ff0040"></label>
<button id="apply">Apply color</button><div class="swatches" id="sw" style="flex:1"></div></div>
<div class="row"><label class="field" style="flex:1"><span>White channel <span class="muted">(RGBW strips only)</span> <span id="whiteVal" class="val"></span></span><input type="range" id="white" min="0" max="255" value="0"></label></div>
<div class="row" style="margin-top:16px"><label class="field" style="flex:1;min-width:150px"><span>Speed <span class="muted">(1–10)</span> <span id="spVal" class="val"></span></span><input type="range" id="speed" min="1" max="10" value="5"></label>
<label class="field" style="flex:1;min-width:150px"><span>Effect length <span class="muted">(1–150)</span> <span id="lenVal" class="val"></span></span><input type="range" id="length" min="1" max="150" value="75"></label></div>
<h2 style="margin-top:18px">Effects <span class="muted" style="text-transform:none;letter-spacing:0">— names calibrated for SP61x</span></h2><div class="grid" id="fx"></div>
<div class="row" style="margin-top:10px"><label class="field"><span>Effect / mode # (0–255)</span><input type="number" id="fxnum" min="0" max="255" value="1" style="width:120px"></label><button id="fxgo" style="align-self:flex-end">Set effect</button></div>
<details style="margin-top:18px"><summary class="muted">Advanced — send raw hex</summary>
<div class="row"><input type="text" id="raw" placeholder="A0 62 01 01" style="flex:1;font-family:ui-monospace,monospace"><button id="rawgo">Send</button></div></details>
</div>

<div class="card"><h2>Log</h2><div id="log"></div></div>
</div>
<script>
const $=i=>document.getElementById(i);
const log=m=>{const l=$("log");l.textContent+="["+new Date().toTimeString().slice(0,8)+"] "+m+"\n";l.scrollTop=l.scrollHeight};
const api=async(p,label)=>{try{const r=await fetch(p);if(label)log((r.ok?"→ ":"✗ ")+label);return r}catch(e){log("✗ "+(label||p)+": "+e.message)}};
const FX=[["Rainbow",1],["Rainbow meteor",2],["Rainbow stars",3],["Rainbow spin",4],["Red/Yellow fire",5],["Red/Purple fire",6],["Green/Yellow fire",7],["Green/Cyan fire",8],["Red comet",11],["Blue meteor",20],["Red/Blue snake",26],["Cyan wave",50],["Red/Yellow wave",55],["White stars",80],["Blue bg stars",83]];
const SW=["#ff0000","#ff7a00","#ffd400","#34ff00","#00ffd0","#0066ff","#7a00ff","#ff00c8","#ffffff"];
const PROTOS=[["sp61x","SP61x/62x"],["sp63x","SP63x/64x"],["sp613","SP613/614"],["sp110e","SP110E"]];
const ORDERS=["rgb","grb","bgr","rbg","gbr","brg"];
function throttle(fn,ms){let p=null,t=null;return v=>{p=v;if(t)return;fn(p);p=null;t=setTimeout(()=>{t=null;if(p!=null){const q=p;p=null;fn(q)}},ms)}}
function mkSel(opts,val,on,title){const s=document.createElement("select");s.className="ds";if(title)s.title=title;opts.forEach(o=>{const v=Array.isArray(o)?o[0]:o,t=Array.isArray(o)?o[1]:o.toUpperCase();const e=document.createElement("option");e.value=v;e.textContent=t;if(v===val)e.selected=true;s.appendChild(e)});s.onchange=()=>on(s.value);return s}
function setcfg(i,o){const q=Object.keys(o).map(k=>k+"="+encodeURIComponent(o[k])).join("&");api("/api/setcfg?idx="+i+"&"+q,"strip "+i+" "+q)}
async function refresh(){try{const j=await(await fetch("/api/status")).json();const box=$("devs");box.innerHTML="";let up=0;
 j.forEach((s,i)=>{const row=document.createElement("div");row.className="devrow";
  const d=document.createElement("span");d.className="dot"+(s.connected?" on":"");
  const n=document.createElement("span");n.className="devname";n.textContent=(s.name||s.match||"strip");
  const pp=mkSel(PROTOS,s.profile,v=>setcfg(i,{profile:v}),"Protocol");
  const oo=mkSel(ORDERS,s.order,v=>setcfg(i,{order:v}),"Colour order");
  const cc=mkSel([["rgb","RGB"],["rgbw","RGBW"]],s.rgbw?"rgbw":"rgb",v=>setcfg(i,{rgbw:v==="rgbw"?1:0}),"Channels");
  const st=document.createElement("span");st.className="muted";st.textContent=s.connected?"":"offline";
  row.append(d,n,pp,oo,cc,st);box.appendChild(row);if(s.connected)up++});
 if(!j.length)box.innerHTML='<p class="muted">No strips configured in config.h.</p>';
 $("dot").classList.toggle("on",up>0);$("statusText").textContent=up>0?("Connected: "+up):"No strips connected";
}catch(e){$("statusText").textContent="hub unreachable"}}
window.addEventListener("DOMContentLoaded",()=>{
 SW.forEach(c=>{const b=document.createElement("button");b.className="swatch";b.style.background=c;b.onclick=()=>{$("color").value=c;applyColor()};$("sw").appendChild(b)});
 FX.forEach(([n,code])=>{const b=document.createElement("button");b.textContent=n;b.onclick=()=>api("/api/effect?code="+code,"Effect: "+n);$("fx").appendChild(b)});
 $("briVal").textContent=$("bri").value;$("spVal").textContent=$("speed").value;$("lenVal").textContent=$("length").value;$("whiteVal").textContent=$("white").value;
 function applyColor(){api("/api/color?hex="+$("color").value.slice(1)+"&bri="+$("bri").value,"Color "+$("color").value)}
 $("on").onclick=()=>api("/api/power?on=1","Power ON");
 $("off").onclick=()=>api("/api/power?on=0","Power OFF");
 const bs=throttle(v=>fetch("/api/brightness?v="+v),80);
 $("bri").oninput=e=>{$("briVal").textContent=e.target.value;bs(e.target.value)};
 $("bri").onchange=e=>api("/api/brightness?v="+e.target.value,"Brightness "+e.target.value);
 const ws=throttle(v=>fetch("/api/white?v="+v),80);
 $("white").oninput=e=>{$("whiteVal").textContent=e.target.value;ws(e.target.value)};
 $("white").onchange=e=>api("/api/white?v="+e.target.value,"White "+e.target.value);
 const ss=throttle(v=>fetch("/api/speed?v="+v),80);
 $("speed").oninput=e=>{$("spVal").textContent=e.target.value;ss(e.target.value)};
 $("speed").onchange=e=>api("/api/speed?v="+e.target.value,"Speed "+e.target.value);
 const ls=throttle(v=>fetch("/api/length?v="+v),80);
 $("length").oninput=e=>{$("lenVal").textContent=e.target.value;ls(e.target.value)};
 $("length").onchange=e=>api("/api/length?v="+e.target.value,"Length "+e.target.value);
 $("apply").onclick=applyColor;
 $("fxgo").onclick=()=>api("/api/effect?code="+$("fxnum").value,"Effect #"+$("fxnum").value);
 $("rawgo").onclick=()=>{const h=($("raw").value.match(/[0-9a-f]{2}/gi)||[]).join("");if(h)api("/api/raw?hex="+h,"raw "+$("raw").value);else log("no hex")};
 $("reconnect").onclick=()=>{log("rescanning…");api("/api/reconnect","reconnect").then(refresh)};
 refresh();setInterval(refresh,5000);
});
</script></body></html>
)HTMLPAGE";
