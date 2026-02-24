#include "berbel_component.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include <cstring>

static const char *TAG = "berbel";

static const uint8_t CMD_FAN_OFF[31] = {
    0x01, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t CMD_FAN_L1[31] = {
    0x01, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t CMD_FAN_L2[31] = {
    0x01, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t CMD_FAN_L3[31] = {
    0x01, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t CMD_LIGHT_BOTH_OFF[31] = {
    0x01, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const char *WEB_INDEX_HTML = R"rawhtml(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Berbel ESP32</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;padding:16px}
.card{background:#16213e;border-radius:12px;padding:20px;margin-bottom:16px;box-shadow:0 4px 12px rgba(0,0,0,.3)}
h1{font-size:1.4em;margin-bottom:4px;color:#e94560}
h2{font-size:1.1em;margin-bottom:12px;color:#0f3460;background:#e94560;padding:8px 12px;border-radius:8px}
.row{display:flex;align-items:center;gap:12px;margin-bottom:10px}
label{min-width:140px;font-size:.9em;color:#aaa}
input[type=range]{flex:1;accent-color:#e94560}
input[type=number]{width:60px;padding:4px;background:#0f3460;border:1px solid #e94560;border-radius:4px;color:#eee;text-align:center}
button{padding:8px 18px;background:#e94560;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:.9em;transition:opacity .2s}
button:hover{opacity:.85}
button.secondary{background:#0f3460}
.status-dot{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:6px}
.connected{background:#27ae60}
.disconnected{background:#e74c3c}
.scanning{background:#f39c12;animation:pulse 1s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
.device-list{margin-top:10px}
.device-item{display:flex;justify-content:space-between;align-items:center;padding:8px 12px;background:#0f3460;border-radius:6px;margin-bottom:6px}
.device-name{font-weight:bold}
.device-addr{font-size:.8em;color:#aaa}
.device-rssi{font-size:.8em;color:#aaa}
#log{font-size:.8em;color:#888;margin-top:8px;max-height:80px;overflow-y:auto}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:480px){.grid2{grid-template-columns:1fr}}
.toggle{display:flex;gap:8px}
.toggle button{flex:1}
.toggle button.active{background:#27ae60}
</style>
</head>
<body>
<div class="card">
<h1>&#x1F3DB; Berbel ESP32</h1>
<div id="conn-status"><span class="status-dot disconnected" id="dot"></span><span id="conn-text">Nicht verbunden</span></div>
</div>

<div class="card">
<h2>&#x1F4A8; L&uuml;fter</h2>
<div class="row">
<label>Stufe</label>
<div class="toggle">
<button id="fan0" onclick="setFan(0)">Aus</button>
<button id="fan1" onclick="setFan(1)">1</button>
<button id="fan2" onclick="setFan(2)">2</button>
<button id="fan3" onclick="setFan(3)">3</button>
</div>
</div>
<div class="row"><label>Nachlauf</label><span id="postrun">-</span></div>
</div>

<div class="card">
<h2>&#x1F4A1; Licht Oben</h2>
<div class="row">
<label>Ein/Aus</label>
<div class="toggle">
<button id="ltop_off" onclick="setLight('top','on',false)">Aus</button>
<button id="ltop_on" onclick="setLight('top','on',true)">Ein</button>
</div>
</div>
<div class="row">
<label>Helligkeit</label>
<input type="range" min="0" max="100" id="ltop_bri" oninput="document.getElementById('ltop_bri_v').value=this.value" onchange="setLight('top','brightness',this.value)">
<input type="number" min="0" max="100" id="ltop_bri_v" onchange="document.getElementById('ltop_bri').value=this.value;setLight('top','brightness',this.value)">
</div>
<div class="row">
<label>Farbtemp (K)</label>
<input type="range" min="2700" max="6500" step="100" id="ltop_ct" oninput="document.getElementById('ltop_ct_v').value=this.value" onchange="setLight('top','color_temp',this.value)">
<input type="number" min="2700" max="6500" id="ltop_ct_v" onchange="document.getElementById('ltop_ct').value=this.value;setLight('top','color_temp',this.value)">
</div>
</div>

<div class="card">
<h2>&#x1F4A1; Licht Unten</h2>
<div class="row">
<label>Ein/Aus</label>
<div class="toggle">
<button id="lbot_off" onclick="setLight('bottom','on',false)">Aus</button>
<button id="lbot_on" onclick="setLight('bottom','on',true)">Ein</button>
</div>
</div>
<div class="row">
<label>Helligkeit</label>
<input type="range" min="0" max="100" id="lbot_bri" oninput="document.getElementById('lbot_bri_v').value=this.value" onchange="setLight('bottom','brightness',this.value)">
<input type="number" min="0" max="100" id="lbot_bri_v" onchange="document.getElementById('lbot_bri').value=this.value;setLight('bottom','brightness',this.value)">
</div>
<div class="row">
<label>Farbtemp (K)</label>
<input type="range" min="2700" max="6500" step="100" id="lbot_ct" oninput="document.getElementById('lbot_ct_v').value=this.value" onchange="setLight('bottom','color_temp',this.value)">
<input type="number" min="2700" max="6500" id="lbot_ct_v" onchange="document.getElementById('lbot_ct').value=this.value;setLight('bottom','color_temp',this.value)">
</div>
</div>

<div class="card">
<h2>&#x1F4F6; Ger&auml;t koppeln</h2>
<div class="row">
<label>Aktuelle MAC</label><span id="current-mac">-</span>
</div>
<div class="row">
<button onclick="startScan()">BLE-Scan starten</button>
<span id="scan-status" style="margin-left:8px;font-size:.85em;color:#aaa"></span>
</div>
<div class="device-list" id="device-list"></div>
<div id="log"></div>
</div>

<script>
var pollTimer;
function log(m){var e=document.getElementById('log');e.innerHTML+=m+'<br>';e.scrollTop=e.scrollHeight}
function api(path,method,body){
  return fetch(path,{method:method||'GET',headers:body?{'Content-Type':'application/json'}:{},body:body?JSON.stringify(body):undefined})
    .then(r=>r.json()).catch(e=>{log('Fehler: '+e);return null});
}
function updateUI(s){
  if(!s)return;
  var dot=document.getElementById('dot');
  var ct=document.getElementById('conn-text');
  if(s.connected){dot.className='status-dot connected';ct.textContent='Verbunden: '+(s.mac||'')}
  else{dot.className='status-dot disconnected';ct.textContent='Nicht verbunden'}
  ['fan0','fan1','fan2','fan3'].forEach(function(id,i){
    document.getElementById(id).className=s.fan_level===i?'active':'';
  });
  document.getElementById('postrun').textContent=s.fan_postrun?'Aktiv':'Inaktiv';
  ['ltop_on','ltop_off'].forEach(function(id){
    document.getElementById(id).className=id==='ltop_on'&&s.light_top_on?'active':'';
  });
  ['lbot_on','lbot_off'].forEach(function(id){
    document.getElementById(id).className=id==='lbot_on'&&s.light_bottom_on?'active':'';
  });
  document.getElementById('ltop_bri').value=s.light_top_brightness;
  document.getElementById('ltop_bri_v').value=s.light_top_brightness;
  document.getElementById('ltop_ct').value=s.light_top_color_kelvin;
  document.getElementById('ltop_ct_v').value=s.light_top_color_kelvin;
  document.getElementById('lbot_bri').value=s.light_bottom_brightness;
  document.getElementById('lbot_bri_v').value=s.light_bottom_brightness;
  document.getElementById('lbot_ct').value=s.light_bottom_color_kelvin;
  document.getElementById('lbot_ct_v').value=s.light_bottom_color_kelvin;
  document.getElementById('current-mac').textContent=s.mac||'-';
}
function poll(){api('/api/state').then(updateUI)}
function setFan(l){api('/api/fan','POST',{level:l}).then(poll)}
function setLight(pos,param,val){
  var body={};body[param]=param==='on'?!!val:parseInt(val);
  api('/api/light/'+pos,'POST',body).then(poll);
}
function startScan(){
  document.getElementById('scan-status').textContent='Scannt...';
  document.getElementById('device-list').innerHTML='';
  var dot2=document.getElementById('dot');
  dot2.className='status-dot scanning';
  api('/api/scan','POST').then(function(){
    setTimeout(checkScan,2000);
  });
}
function checkScan(){
  api('/api/scan/results').then(function(r){
    if(!r)return;
    if(r.scanning){setTimeout(checkScan,1500);return}
    document.getElementById('scan-status').textContent=r.devices.length+' Ger\u00e4t(e) gefunden';
    var list=document.getElementById('device-list');
    list.innerHTML='';
    r.devices.forEach(function(d){
      var item=document.createElement('div');
      item.className='device-item';
      item.innerHTML='<div><div class="device-name">'+(d.name||'Unbekannt')+'</div><div class="device-addr">'+d.address+'</div></div><div class="device-rssi">'+d.rssi+' dBm</div>';
      var btn=document.createElement('button');
      btn.textContent='Koppeln';
      btn.onclick=(function(addr){return function(){pairDevice(addr)}})(d.address);
      item.appendChild(btn);
      list.appendChild(item);
    });
    poll();
  });
}
function pairDevice(mac){
  log('Koppeln mit '+mac+'...');
  api('/api/pair','POST',{mac:mac}).then(function(r){
    if(r&&r.ok){log('Verbunden!');poll()}
    else{log('Fehler beim Koppeln')}
  });
}
poll();
pollTimer=setInterval(poll,10000);
</script>
</body>
</html>
)rawhtml";

namespace esphome {
namespace berbel {

void BerbelScanCallbacks::onResult(NimBLEAdvertisedDevice *device) {
  component_->on_scan_result(device);
}

void BerbelClientCallbacks::onConnect(NimBLEClient *client) {
  component_->on_ble_connect(client);
}

void BerbelClientCallbacks::onDisconnect(NimBLEClient *client, int reason) {
  component_->on_ble_disconnect(client, reason);
}

void BerbelComponent::setup() {
  pref_mac_ = global_preferences->make_preference<char[18]>(fnv1_hash("berbel_mac"));
  device_mac_ = load_mac();
  if (!device_mac_.empty()) {
    ESP_LOGI(TAG, "Gespeicherte Berbel MAC: %s", device_mac_.c_str());
    state_.ble_ready = true;
  }
  setup_ble();
  setup_http();
}

void BerbelComponent::setup_ble() {
  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init("berbel-esp32");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(false, false, true);
  }
  state_.ble_ready = true;
  ESP_LOGI(TAG, "NimBLE initialisiert");
}

void BerbelComponent::loop() {
  if (!state_.ble_ready)
    return;

  if (pending_connect_) {
    pending_connect_ = false;
    std::string mac = pending_mac_;
    pending_mac_ = "";
    connect_internal(mac);
    return;
  }

  if (pending_disconnect_) {
    pending_disconnect_ = false;
    disconnect_internal();
    return;
  }

  if (ble_state_ == BerbelBleState::IDLE && !device_mac_.empty()) {
    uint32_t now = millis();
    if (now - last_update_ms_ >= update_interval_ms_) {
      last_update_ms_ = now;
      if (!connect_internal(device_mac_)) {
        ESP_LOGW(TAG, "Verbindung fehlgeschlagen, naechster Versuch in %u ms", update_interval_ms_);
      }
    }
  }
}

float BerbelComponent::get_setup_priority() const { return setup_priority::AFTER_WIFI; }

void BerbelComponent::set_update_interval(uint32_t ms) { update_interval_ms_ = ms; }

void BerbelComponent::set_device_mac(const std::string &mac) {
  device_mac_ = mac;
  save_mac(mac);
}

void BerbelComponent::start_scan() {
  if (ble_state_ == BerbelBleState::SCANNING)
    return;
  scanned_devices_.clear();
  ble_state_ = BerbelBleState::SCANNING;
  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new BerbelScanCallbacks(this), false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  scan->start(SCAN_DURATION_S, [this](NimBLEScanResults r) { this->on_scan_complete(r); }, false);
  ESP_LOGI(TAG, "BLE-Scan gestartet (%u Sekunden)", SCAN_DURATION_S);
}

void BerbelComponent::stop_scan() {
  NimBLEDevice::getScan()->stop();
  ble_state_ = BerbelBleState::IDLE;
}

void BerbelComponent::on_scan_result(NimBLEAdvertisedDevice *device) {
  ScannedDevice d;
  d.address = device->getAddress().toString();
  d.name = device->getName();
  d.rssi = device->getRSSI();
  for (auto &existing : scanned_devices_) {
    if (existing.address == d.address) {
      existing.rssi = d.rssi;
      if (!d.name.empty())
        existing.name = d.name;
      return;
    }
  }
  scanned_devices_.push_back(d);
  ESP_LOGD(TAG, "Gefunden: %s (%s) RSSI=%d", d.name.c_str(), d.address.c_str(), d.rssi);
}

void BerbelComponent::on_scan_complete(NimBLEScanResults results) {
  ble_state_ = BerbelBleState::IDLE;
  ESP_LOGI(TAG, "BLE-Scan abgeschlossen, %u Geraete gefunden", (unsigned) scanned_devices_.size());
}

void BerbelComponent::connect_to_device(const std::string &mac) {
  pending_mac_ = mac;
  pending_connect_ = true;
}

void BerbelComponent::disconnect_device() { pending_disconnect_ = true; }

void BerbelComponent::on_ble_connect(NimBLEClient *client) {
  ble_state_ = BerbelBleState::CONNECTED;
  state_.connected = true;
  ESP_LOGI(TAG, "BLE verbunden");
}

void BerbelComponent::on_ble_disconnect(NimBLEClient *client, int reason) {
  ble_state_ = BerbelBleState::IDLE;
  state_.connected = false;
  char_state_ = nullptr;
  char_brightness_ = nullptr;
  char_color_ = nullptr;
  ESP_LOGI(TAG, "BLE getrennt (reason=%d)", reason);
}

bool BerbelComponent::connect_internal(const std::string &mac) {
  if (ble_state_ == BerbelBleState::SCANNING)
    stop_scan();

  ble_state_ = BerbelBleState::CONNECTING;

  if (ble_client_ && ble_client_->isConnected()) {
    ble_client_->disconnect();
    delay(200);
  }

  if (!ble_client_) {
    ble_client_ = NimBLEDevice::createClient();
    ble_client_->setClientCallbacks(new BerbelClientCallbacks(this), false);
    ble_client_->setConnectionParams(12, 12, 0, 51);
    ble_client_->setConnectTimeout(10);
  }

  NimBLEAddress addr(mac);
  if (!ble_client_->connect(addr)) {
    ESP_LOGW(TAG, "Verbindung zu %s fehlgeschlagen", mac.c_str());
    ble_state_ = BerbelBleState::IDLE;
    state_.connected = false;
    return false;
  }

  if (!find_characteristics()) {
    ESP_LOGW(TAG, "Charakteristiken nicht gefunden");
    ble_client_->disconnect();
    ble_state_ = BerbelBleState::IDLE;
    state_.connected = false;
    return false;
  }

  ble_state_ = BerbelBleState::CONNECTED;
  state_.connected = true;
  device_mac_ = mac;
  save_mac(mac);

  read_state();

  ble_client_->disconnect();
  ble_state_ = BerbelBleState::IDLE;
  return true;
}

void BerbelComponent::disconnect_internal() {
  if (ble_client_ && ble_client_->isConnected()) {
    ble_client_->disconnect();
  }
  ble_state_ = BerbelBleState::IDLE;
  state_.connected = false;
  char_state_ = nullptr;
  char_brightness_ = nullptr;
  char_color_ = nullptr;
}

bool BerbelComponent::find_characteristics() {
  char_state_ = nullptr;
  char_brightness_ = nullptr;
  char_color_ = nullptr;

  auto services = ble_client_->getServices(true);
  if (!services) {
    ESP_LOGW(TAG, "Keine Services gefunden");
    return false;
  }

  for (auto &svc : *services) {
    auto chars = svc->getCharacteristics(true);
    if (!chars)
      continue;
    for (auto &ch : *chars) {
      std::string uuid = ch->getUUID().toString();
      std::string upper_uuid;
      upper_uuid.resize(uuid.size());
      for (size_t i = 0; i < uuid.size(); i++)
        upper_uuid[i] = toupper(uuid[i]);

      if (upper_uuid == std::string(CHAR_STATE_UUID))
        char_state_ = ch;
      else if (upper_uuid == std::string(CHAR_BRIGHTNESS_UUID))
        char_brightness_ = ch;
      else if (upper_uuid == std::string(CHAR_COLOR_UUID))
        char_color_ = ch;
    }
  }

  if (!char_state_ || !char_brightness_ || !char_color_) {
    ESP_LOGW(TAG, "Nicht alle Charakteristiken gefunden (state=%p bri=%p col=%p)", (void *) char_state_,
             (void *) char_brightness_, (void *) char_color_);
    return false;
  }
  return true;
}

bool BerbelComponent::read_state() {
  if (!char_state_ || !char_brightness_ || !char_color_) {
    return false;
  }

  std::string s_status = char_state_->readValue();
  std::string s_brightness = char_brightness_->readValue();
  std::string s_color = char_color_->readValue();

  if (s_status.empty()) {
    ESP_LOGW(TAG, "Status leer");
    return false;
  }

  std::vector<uint8_t> status(s_status.begin(), s_status.end());
  std::vector<uint8_t> brightness(s_brightness.begin(), s_brightness.end());
  std::vector<uint8_t> colors(s_color.begin(), s_color.end());

  parse_state(status, brightness, colors);
  return true;
}

void BerbelComponent::parse_state(const std::vector<uint8_t> &status, const std::vector<uint8_t> &brightness,
                                   const std::vector<uint8_t> &colors) {
  if (status.size() > STATUS_LIGHT_TOP_BYTE)
    state_.light_top_on = (status[STATUS_LIGHT_TOP_BYTE] & LIGHT_ON_MASK) != 0;
  if (status.size() > STATUS_LIGHT_BOTTOM_BYTE)
    state_.light_bottom_on = (status[STATUS_LIGHT_BOTTOM_BYTE] & LIGHT_ON_MASK) != 0;
  if (status.size() > STATUS_POSTRUN_BYTE)
    state_.fan_postrun = (status[STATUS_POSTRUN_BYTE] & POSTRUN_MASK) == POSTRUN_MASK;

  state_.fan_level = 0;
  if (status.size() > STATUS_FAN_L1_BYTE && status[STATUS_FAN_L1_BYTE] == FAN_L1_VALUE)
    state_.fan_level = 1;
  else if (status.size() > STATUS_FAN_L23_BYTE) {
    if (status[STATUS_FAN_L23_BYTE] == FAN_L2_VALUE)
      state_.fan_level = 2;
    else if (status[STATUS_FAN_L23_BYTE] == FAN_L3_VALUE)
      state_.fan_level = 3;
  }

  if (brightness.size() > BRIGHTNESS_TOP_BYTE) {
    state_.light_bottom_brightness = brightness[BRIGHTNESS_BOTTOM_BYTE] * 100 / 255;
    state_.light_top_brightness = brightness[BRIGHTNESS_TOP_BYTE] * 100 / 255;
  }

  if (colors.size() > COLOR_TOP_BYTE) {
    state_.light_bottom_color = colors[COLOR_BOTTOM_BYTE] * 100 / 255;
    state_.light_top_color = colors[COLOR_TOP_BYTE] * 100 / 255;
  }

  ESP_LOGI(TAG, "Status: Fan=%u Post=%d TopL=%d(%u%% %uK) BotL=%d(%u%% %uK)", state_.fan_level, state_.fan_postrun,
           state_.light_top_on, state_.light_top_brightness, state_.light_top_color_kelvin(), state_.light_bottom_on,
           state_.light_bottom_brightness, state_.light_bottom_color_kelvin());
}

bool BerbelComponent::execute_command(const uint8_t *cmd, size_t len) {
  if (len != 31) {
    ESP_LOGE(TAG, "Ungueltige Befehlslaenge: %u", (unsigned) len);
    return false;
  }

  if (ble_state_ == BerbelBleState::SCANNING)
    stop_scan();

  bool was_connected = (ble_state_ == BerbelBleState::CONNECTED && ble_client_ && ble_client_->isConnected());

  if (!was_connected) {
    if (!connect_internal(device_mac_)) {
      return false;
    }
  } else if (!char_brightness_) {
    if (!find_characteristics()) {
      return false;
    }
  }

  if (!char_brightness_) {
    ESP_LOGE(TAG, "Schreib-Charakteristik nicht verfuegbar");
    return false;
  }

  bool ok = char_brightness_->writeValue(cmd, len, true);

  if (!was_connected) {
    delay(100);
    read_state();
    ble_client_->disconnect();
    ble_state_ = BerbelBleState::IDLE;
  }

  return ok;
}

void BerbelComponent::set_fan_level(uint8_t level) {
  ESP_LOGI(TAG, "Luefterlevel: %u", level);
  switch (level) {
    case 0:
      execute_command(CMD_FAN_OFF, 31);
      break;
    case 1:
      execute_command(CMD_FAN_L1, 31);
      break;
    case 2:
      execute_command(CMD_FAN_L2, 31);
      break;
    case 3:
      execute_command(CMD_FAN_L3, 31);
      break;
    default:
      ESP_LOGW(TAG, "Ungueltiger Luefterlevel: %u", level);
      return;
  }
  state_.fan_level = level;
}

void BerbelComponent::set_light_top_brightness(uint8_t pct) {
  if (pct > 100)
    pct = 100;
  uint8_t cmd[31];
  memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
  cmd[5] = static_cast<uint8_t>(pct * 255 / 100);
  cmd[4] = static_cast<uint8_t>(state_.light_bottom_brightness * 255 / 100);
  execute_command(cmd, 31);
  state_.light_top_brightness = pct;
}

void BerbelComponent::set_light_bottom_brightness(uint8_t pct) {
  if (pct > 100)
    pct = 100;
  uint8_t cmd[31];
  memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
  cmd[4] = static_cast<uint8_t>(pct * 255 / 100);
  cmd[5] = static_cast<uint8_t>(state_.light_top_brightness * 255 / 100);
  execute_command(cmd, 31);
  state_.light_bottom_brightness = pct;
}

void BerbelComponent::set_light_top_on(bool on) {
  if (on) {
    uint8_t cmd[31];
    memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
    uint8_t bri = state_.light_top_brightness > 0 ? state_.light_top_brightness : 100;
    cmd[5] = static_cast<uint8_t>(bri * 255 / 100);
    cmd[4] = static_cast<uint8_t>(state_.light_bottom_brightness * 255 / 100);
    execute_command(cmd, 31);
    state_.light_top_on = true;
    if (state_.light_top_brightness == 0)
      state_.light_top_brightness = 100;
  } else {
    uint8_t cmd[31];
    memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
    cmd[4] = static_cast<uint8_t>(state_.light_bottom_brightness * 255 / 100);
    execute_command(cmd, 31);
    state_.light_top_on = false;
    state_.light_top_brightness = 0;
  }
}

void BerbelComponent::set_light_bottom_on(bool on) {
  if (on) {
    uint8_t cmd[31];
    memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
    uint8_t bri = state_.light_bottom_brightness > 0 ? state_.light_bottom_brightness : 100;
    cmd[4] = static_cast<uint8_t>(bri * 255 / 100);
    cmd[5] = static_cast<uint8_t>(state_.light_top_brightness * 255 / 100);
    execute_command(cmd, 31);
    state_.light_bottom_on = true;
    if (state_.light_bottom_brightness == 0)
      state_.light_bottom_brightness = 100;
  } else {
    uint8_t cmd[31];
    memcpy(cmd, CMD_LIGHT_BOTH_OFF, 31);
    cmd[5] = static_cast<uint8_t>(state_.light_top_brightness * 255 / 100);
    execute_command(cmd, 31);
    state_.light_bottom_on = false;
    state_.light_bottom_brightness = 0;
  }
}

uint8_t BerbelComponent::kelvin_to_pct(uint16_t kelvin) {
  if (kelvin <= KELVIN_MIN)
    return 100;
  if (kelvin >= KELVIN_MAX)
    return 0;
  return static_cast<uint8_t>((KELVIN_MAX - kelvin) * 100 / (KELVIN_MAX - KELVIN_MIN));
}

uint16_t BerbelComponent::pct_to_kelvin(uint8_t pct) {
  return KELVIN_MAX - static_cast<uint32_t>(pct) * (KELVIN_MAX - KELVIN_MIN) / 100;
}

void BerbelComponent::set_light_top_color_kelvin(uint16_t kelvin) {
  uint8_t pct = kelvin_to_pct(kelvin);
  uint8_t raw = static_cast<uint8_t>(pct * 255 / 100);

  if (!char_color_) {
    if (ble_state_ == BerbelBleState::SCANNING)
      stop_scan();
    if (!connect_internal(device_mac_))
      return;
  }

  if (char_color_) {
    std::string cur = char_color_->readValue();
    std::vector<uint8_t> colors(cur.begin(), cur.end());
    while (colors.size() <= COLOR_TOP_BYTE)
      colors.push_back(0);
    colors[COLOR_TOP_BYTE] = raw;
    char_color_->writeValue(colors.data(), colors.size(), true);
    state_.light_top_color = pct;
    ble_client_->disconnect();
    ble_state_ = BerbelBleState::IDLE;
  }
}

void BerbelComponent::set_light_bottom_color_kelvin(uint16_t kelvin) {
  uint8_t pct = kelvin_to_pct(kelvin);
  uint8_t raw = static_cast<uint8_t>(pct * 255 / 100);

  if (!char_color_) {
    if (ble_state_ == BerbelBleState::SCANNING)
      stop_scan();
    if (!connect_internal(device_mac_))
      return;
  }

  if (char_color_) {
    std::string cur = char_color_->readValue();
    std::vector<uint8_t> colors(cur.begin(), cur.end());
    while (colors.size() <= COLOR_BOTTOM_BYTE)
      colors.push_back(0);
    colors[COLOR_BOTTOM_BYTE] = raw;
    char_color_->writeValue(colors.data(), colors.size(), true);
    state_.light_bottom_color = pct;
    ble_client_->disconnect();
    ble_state_ = BerbelBleState::IDLE;
  }
}

const BerbelState &BerbelComponent::get_state() const { return state_; }

const std::vector<ScannedDevice> &BerbelComponent::get_scanned_devices() const { return scanned_devices_; }

bool BerbelComponent::is_scanning() const { return ble_state_ == BerbelBleState::SCANNING; }

BerbelBleState BerbelComponent::get_ble_state() const { return ble_state_; }

void BerbelComponent::save_mac(const std::string &mac) {
  char buf[18] = {};
  strncpy(buf, mac.c_str(), 17);
  pref_mac_.save(&buf);
  ESP_LOGI(TAG, "MAC gespeichert: %s", mac.c_str());
}

std::string BerbelComponent::load_mac() {
  char buf[18] = {};
  if (pref_mac_.load(&buf)) {
    buf[17] = '\0';
    if (strlen(buf) >= 17) {
      return std::string(buf);
    }
  }
  return "";
}

std::string BerbelComponent::state_to_json() {
  char buf[512];
  snprintf(buf, sizeof(buf),
           "{\"connected\":%s,\"mac\":\"%s\","
           "\"fan_level\":%u,\"fan_postrun\":%s,"
           "\"light_top_on\":%s,\"light_bottom_on\":%s,"
           "\"light_top_brightness\":%u,\"light_bottom_brightness\":%u,"
           "\"light_top_color_kelvin\":%u,\"light_bottom_color_kelvin\":%u}",
           state_.connected ? "true" : "false", device_mac_.c_str(), state_.fan_level,
           state_.fan_postrun ? "true" : "false", state_.light_top_on ? "true" : "false",
           state_.light_bottom_on ? "true" : "false", state_.light_top_brightness, state_.light_bottom_brightness,
           state_.light_top_color_kelvin(), state_.light_bottom_color_kelvin());
  return std::string(buf);
}

std::string BerbelComponent::scanned_to_json() {
  std::string json = "{\"scanning\":";
  json += is_scanning() ? "true" : "false";
  json += ",\"devices\":[";
  bool first = true;
  for (auto &d : scanned_devices_) {
    if (!first)
      json += ",";
    first = false;
    char entry[256];
    snprintf(entry, sizeof(entry), "{\"address\":\"%s\",\"name\":\"%s\",\"rssi\":%d}", d.address.c_str(),
             d.name.c_str(), d.rssi);
    json += entry;
  }
  json += "]}";
  return json;
}

void BerbelComponent::setup_http() {
  auto *base = web_server_base::global_web_server_base;
  if (!base) {
    ESP_LOGE(TAG, "web_server_base nicht verfuegbar");
    return;
  }
  AsyncWebServer *srv = base->get_server();
  if (!srv) {
    ESP_LOGE(TAG, "AsyncWebServer nicht verfuegbar");
    return;
  }

  srv->on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(200, "text/html", WEB_INDEX_HTML);
  });

  srv->on("/api/state", HTTP_GET, [this](AsyncWebServerRequest *req) {
    req->send(200, "application/json", state_to_json().c_str());
  });

  srv->on("/api/scan", HTTP_POST, [this](AsyncWebServerRequest *req) {
    start_scan();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  srv->on("/api/scan/results", HTTP_GET, [this](AsyncWebServerRequest *req) {
    req->send(200, "application/json", scanned_to_json().c_str());
  });

  AsyncCallbackWebHandler *fan_handler = new AsyncCallbackWebHandler();
  fan_handler->setUri("/api/fan");
  fan_handler->setMethod(HTTP_POST);
  fan_handler->onBody([this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total) {
      std::string body(reinterpret_cast<char *>(data), len);
      uint8_t level = 0;
      const char *p = strstr(body.c_str(), "\"level\"");
      if (p) {
        p += 7;
        while (*p == ':' || *p == ' ')
          p++;
        level = static_cast<uint8_t>(atoi(p));
      }
      set_fan_level(level);
      req->send(200, "application/json", "{\"ok\":true}");
    }
  });
  srv->addHandler(fan_handler);

  auto make_light_handler = [this, srv](const char *uri_suffix) {
    std::string uri = std::string("/api/light/") + uri_suffix;
    AsyncCallbackWebHandler *h = new AsyncCallbackWebHandler();
    h->setUri(uri.c_str());
    h->setMethod(HTTP_POST);
    bool is_top = (std::string(uri_suffix) == "top");
    h->onBody([this, is_top](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
      if (index + len == total) {
        std::string body(reinterpret_cast<char *>(data), len);

        const char *p_on = strstr(body.c_str(), "\"on\"");
        if (p_on) {
          p_on += 4;
          while (*p_on == ':' || *p_on == ' ')
            p_on++;
          bool on_val = (strncmp(p_on, "true", 4) == 0);
          if (is_top)
            set_light_top_on(on_val);
          else
            set_light_bottom_on(on_val);
        }

        const char *p_bri = strstr(body.c_str(), "\"brightness\"");
        if (p_bri) {
          p_bri += 12;
          while (*p_bri == ':' || *p_bri == ' ')
            p_bri++;
          uint8_t bri = static_cast<uint8_t>(atoi(p_bri));
          if (is_top)
            set_light_top_brightness(bri);
          else
            set_light_bottom_brightness(bri);
        }

        const char *p_ct = strstr(body.c_str(), "\"color_temp\"");
        if (p_ct) {
          p_ct += 12;
          while (*p_ct == ':' || *p_ct == ' ')
            p_ct++;
          uint16_t ct = static_cast<uint16_t>(atoi(p_ct));
          if (is_top)
            set_light_top_color_kelvin(ct);
          else
            set_light_bottom_color_kelvin(ct);
        }

        req->send(200, "application/json", "{\"ok\":true}");
      }
    });
    srv->addHandler(h);
  };

  make_light_handler("top");
  make_light_handler("bottom");

  AsyncCallbackWebHandler *pair_handler = new AsyncCallbackWebHandler();
  pair_handler->setUri("/api/pair");
  pair_handler->setMethod(HTTP_POST);
  pair_handler->onBody([this](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index + len == total) {
      std::string body(reinterpret_cast<char *>(data), len);
      char mac[18] = {};
      const char *p = strstr(body.c_str(), "\"mac\"");
      if (p) {
        p += 5;
        while (*p == ':' || *p == ' ' || *p == '"')
          p++;
        size_t i = 0;
        while (*p && *p != '"' && i < 17)
          mac[i++] = *p++;
        mac[i] = '\0';
      }
      if (strlen(mac) >= 17) {
        connect_to_device(std::string(mac));
        req->send(200, "application/json", "{\"ok\":true}");
      } else {
        req->send(400, "application/json", "{\"error\":\"invalid mac\"}");
      }
    }
  });
  srv->addHandler(pair_handler);

  srv->on("/api/disconnect", HTTP_POST, [this](AsyncWebServerRequest *req) {
    disconnect_device();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  srv->onNotFound([](AsyncWebServerRequest *req) { req->send(404, "text/plain", "Not Found"); });

  ESP_LOGI(TAG, "HTTP-Routen registriert");
}

}  // namespace berbel
}  // namespace esphome
