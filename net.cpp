#include "net.h"
#include "settings.h"
#include "config.h"
#include "face.h"
#include "weather.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <time.h>

static WebServer server(80);
static bool apMode = false;

void netApplyTime(){
  configTime((long)(cfg.tzHours*3600), 0, "pool.ntp.org", "time.nist.gov");
}

bool netConnected(){ return WiFi.status() == WL_CONNECTED; }
String netGetIP(){ return apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString(); }

void startAP(){
  apMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg.apName, cfg.apPassword);
  delay(100);
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>DesktopBuddy</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:linear-gradient(135deg,#0f172a 0%,#1e293b 100%);color:#e2e8f0;min-height:100vh;padding:16px}
.card{background:rgba(30,41,59,0.8);backdrop-filter:blur(10px);border:1px solid rgba(148,163,184,0.1);border-radius:16px;padding:20px;margin-bottom:16px;box-shadow:0 4px 20px rgba(0,0,0,0.3)}
h1{font-size:24px;margin-bottom:4px;background:linear-gradient(90deg,#38bdf8,#818cf8);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.sub{color:#94a3b8;font-size:13px;margin-bottom:16px}
.tabs{display:flex;gap:4px;margin-bottom:16px;flex-wrap:wrap}
.tab{padding:8px 14px;background:rgba(51,65,85,0.6);border:none;color:#cbd5e1;border-radius:10px;cursor:pointer;font-size:13px;transition:all .2s}
.tab.active{background:linear-gradient(135deg,#0ea5e9,#6366f1);color:white}
.tab:hover{background:rgba(71,85,105,0.8)}
.field{margin-bottom:12px}
label{display:block;font-size:12px;color:#94a3b8;margin-bottom:4px;text-transform:uppercase;letter-spacing:.5px}
input,select{width:100%;padding:10px 12px;background:rgba(15,23,42,0.8);border:1px solid rgba(148,163,184,0.2);border-radius:8px;color:#e2e8f0;font-size:14px}
input:focus,select:focus{outline:none;border-color:#38bdf8}
button{padding:10px 18px;background:linear-gradient(135deg,#0ea5e9,#6366f1);color:white;border:none;border-radius:8px;cursor:pointer;font-size:14px;font-weight:500;transition:transform .1s}
button:hover{transform:translateY(-1px)}button:active{transform:translateY(0)}
.anim-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:6px;max-height:400px;overflow-y:auto;padding:4px}
.anim-btn{padding:8px;background:rgba(51,65,85,0.6);border:1px solid rgba(148,163,184,0.1);border-radius:6px;color:#cbd5e1;cursor:pointer;font-size:12px;text-align:left;transition:all .2s}
.anim-btn:hover{background:rgba(71,85,105,0.8);border-color:#38bdf8}
.status{display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:13px}
.status-item{background:rgba(15,23,42,0.5);padding:10px;border-radius:8px}
.status-label{color:#94a3b8;font-size:11px;text-transform:uppercase}
.status-value{color:#38bdf8;font-size:14px;font-weight:500;margin-top:2px}
.drop-zone{border:2px dashed rgba(148,163,184,0.3);border-radius:12px;padding:30px;text-align:center;cursor:pointer;transition:all .2s}
.drop-zone.hover{border-color:#38bdf8;background:rgba(56,189,248,0.05)}
.drop-zone p{color:#94a3b8;margin-bottom:8px}
.progress{height:6px;background:rgba(15,23,42,0.8);border-radius:3px;overflow:hidden;margin-top:12px;display:none}
.progress-bar{height:100%;background:linear-gradient(90deg,#0ea5e9,#6366f1);width:0;transition:width .3s}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:rgba(30,41,59,0.95);border:1px solid #38bdf8;padding:12px 20px;border-radius:10px;color:#e2e8f0;opacity:0;transition:opacity .3s;pointer-events:none;z-index:100}
.toast.show{opacity:1}
.panel{display:none}.panel.active{display:block}
.wifi-list{max-height:250px;overflow-y:auto;margin:8px 0}
.wifi-item{padding:10px;background:rgba(15,23,42,0.5);border-radius:6px;margin-bottom:4px;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
.wifi-item:hover{background:rgba(51,65,85,0.8)}
.signal{color:#94a3b8;font-size:12px}
</style></head>
<body>
<div class="card"><h1>DesktopBuddy</h1><div class="sub" id="statusLine">Загрузка...</div>
<div class="tabs"><button class="tab active" data-p="wifi">WiFi</button><button class="tab" data-p="settings">Настройки</button><button class="tab" data-p="anims">Анимации</button><button class="tab" data-p="ota">Прошивка</button><button class="tab" data-p="info">Статус</button></div></div>
<div class="card panel active" id="p-wifi"><h2 style="font-size:16px;margin-bottom:12px">WiFi сети</h2>
<button onclick="scanWifi()">Сканировать</button><div class="wifi-list" id="wifiList"></div>
<div class="field" style="margin-top:12px"><label>SSID</label><input id="wSsid"></div>
<div class="field"><label>Пароль</label><input id="wPass" type="password"></div>
<button onclick="saveWifi()">Подключиться</button></div>
<div class="card panel" id="p-settings">
<div class="field"><label>Город</label><input id="sCity"></div>
<div class="field"><label>Часовой пояс GMT±</label><input id="sTz" type="number" step="0.5"></div>
<div class="field"><label>Имя точки доступа</label><input id="sAp"></div>
<div class="field"><label>Пароль точки (мин. 8 симв.)</label><input id="sApPw"></div>
<button onclick="saveSettings()">Сохранить</button></div>
<div class="card panel" id="p-anims"><p style="margin-bottom:12px;color:#94a3b8;font-size:13px">Нажмите на анимацию, чтобы воспроизвести её один раз.</p>
<div class="anim-grid" id="animGrid"></div></div>
<div class="card panel" id="p-ota"><div class="drop-zone" id="dropZone">
<p>📦 Перетащите .bin файл сюда</p><p style="font-size:12px">или нажмите для выбора</p>
<input type="file" id="otaFile" accept=".bin" style="display:none"></div>
<div class="progress" id="progress"><div class="progress-bar" id="progressBar"></div></div>
<button onclick="uploadOta()" id="otaBtn" style="margin-top:12px;display:none">Прошить</button></div>
<div class="card panel" id="p-info"><div class="status" id="statusGrid"></div></div>
<div class="toast" id="toast"></div>
<script>
const $=id=>document.getElementById(id);
function toast(m){const t=$('toast');t.textContent=m;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2500)}
document.querySelectorAll('.tab').forEach(b=>b.onclick=()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));document.querySelectorAll('.panel').forEach(x=>x.classList.remove('active'));b.classList.add('active');$('p-'+b.dataset.p).classList.add('active');});
async function api(url,opt){try{const r=await fetch(url,opt);const d=await r.json();if(d.ok!==false)toast('✓ OK');else toast('✗ '+d.error);return d}catch(e){toast('✗ '+e.message);return null}}
async function loadStatus(){const d=await api('/api/status');if(!d)return;$('statusLine').textContent=d.mode+' | '+d.ip+' | '+d.uptime;$('statusGrid').innerHTML=Object.entries({'Режим':d.mode,'IP':d.ip,'WiFi':d.wifi,'Время':d.uptime,'Город':d.city,'Температура':d.temp+'°C','Погода':d.wcode,'Анимация':d.anim}).map(([k,v])=>'<div class="status-item"><div class="status-label">'+k+'</div><div class="status-value">'+v+'</div></div>').join('');}
async function scanWifi(){$('wifiList').innerHTML='<div style="padding:10px;color:#94a3b8">Сканирование...</div>';const d=await api('/api/wifi/scan');if(!d||!d.networks)return;$('wifiList').innerHTML=d.networks.map(n=>'<div class="wifi-item" onclick="$(\'wSsid\').value=\''+n.ssid+'\'"><span>'+n.ssid+'</span><span class="signal">'+n.rssi+' dBm</span></div>').join('');}
async function saveWifi(){await api('/api/wifi/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:$('wSsid').value,pass:$('wPass').value})});toast('Перезагрузка...');setTimeout(()=>location.reload(),3000);}
async function loadSettings(){const d=await api('/api/settings');if(!d)return;$('sCity').value=d.city;$('sTz').value=d.tz;$('sAp').value=d.ap;$('sApPw').value=d.appw;}
async function saveSettings(){await api('/api/settings/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({city:$('sCity').value,tz:$('sTz').value,ap:$('sAp').value,appw:$('sApPw').value})});}
async function loadAnims(){const d=await api('/api/anims');if(!d)return;$('animGrid').innerHTML=d.anims.map(a=>'<div class="anim-btn" onclick="playAnim('+a.id+')">'+a.name+'</div>').join('');}
async function playAnim(id){await api('/api/anim/play',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id})});toast('Воспроизведение...');}
const dz=$('dropZone'),fi=$('otaFile');dz.onclick=()=>fi.click();dz.ondragover=e=>{e.preventDefault();dz.classList.add('hover')};dz.ondragleave=()=>dz.classList.remove('hover');dz.ondrop=e=>{e.preventDefault();dz.classList.remove('hover');if(e.dataTransfer.files[0])selectFile(e.dataTransfer.files[0])};fi.onchange=e=>{if(e.target.files[0])selectFile(e.target.files[0])};let otaFile=null;
function selectFile(f){if(!f.name.endsWith('.bin'))return toast('Нужен .bin');otaFile=f;$('otaBtn').style.display='inline-block';toast('Файл: '+f.name)}
function uploadOta(){if(!otaFile)return;const fd=new FormData();fd.append('firmware',otaFile);const xhr=new XMLHttpRequest();$('progress').style.display='block';xhr.upload.onprogress=e=>{if(e.lengthComputable)$('progressBar').style.width=(e.loaded/e.total*100)+'%'};xhr.onload=()=>{toast(xhr.status==200?'Прошивка успешна!':'Ошибка');if(xhr.status==200)setTimeout(()=>location.reload(),5000)};xhr.open('POST','/update');xhr.send(fd);}
loadStatus();loadSettings();loadAnims();setInterval(loadStatus,5000);
</script></body></html>)rawliteral";

void sendJson(const String& j){ server.send(200,"application/json",j); }
void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }

void handleStatus(){
  String j = "{";
  j += "\"mode\":\"" + String(apMode?"AP":"STA") + "\",";
  j += "\"ip\":\"" + netGetIP() + "\",";
  j += "\"wifi\":\"" + (netConnected()?WiFi.SSID():(apMode?String(cfg.apName):String("не подключено"))) + "\",";
  uint32_t up = millis()/1000;
  j += "\"uptime\":\"" + String(up/3600) + "ч " + String((up%3600)/60) + "м\",";
  j += "\"city\":\"" + String(cfg.city) + "\",";
  j += "\"temp\":" + String(temperature) + ",";
  j += "\"wcode\":" + String(weatherCode) + ",";
  j += "\"anim\":\"" + String(animName(player.cur)) + "\"";
  j += "}";
  sendJson(j);
}

void handleSettings(){
  String j = "{";
  j += "\"city\":\"" + String(cfg.city) + "\",";
  j += "\"tz\":" + String(cfg.tzHours,1) + ",";
  j += "\"ap\":\"" + String(cfg.apName) + "\",";
  j += "\"appw\":\"" + String(cfg.apPassword) + "\"";
  j += "}";
  sendJson(j);
}

void handleSettingsSave(){
  if(!server.hasArg("plain")) return sendJson("{\"ok\":false,\"error\":\"no body\"}");
  String b = server.arg("plain");
  auto getStr = [&](const char* k, char* out, int max){
    String pat = String("\"") + k + "\":\"";
    int s = b.indexOf(pat); if(s<0) return;
    s += pat.length(); int e = b.indexOf("\"", s);
    String v = b.substring(s,e); strncpy(out, v.c_str(), max-1); out[max-1]=0;
  };
  auto getFloat = [&](const char* k){
    String pat = String("\"") + k + "\":";
    int s = b.indexOf(pat); if(s<0) return 0.0f;
    s += pat.length(); int e = b.indexOf(",", s); if(e<0) e = b.indexOf("}", s);
    return b.substring(s,e).toFloat();
  };
  char oldCity[33]; strncpy(oldCity, cfg.city, 32);
  getStr("city", cfg.city, 32);
  if(String(oldCity) != String(cfg.city)){ cfg.lat = 0.0f; cfg.lon = 0.0f; }
  cfg.tzHours = getFloat("tz");
  getStr("ap", cfg.apName, 32);
  getStr("appw", cfg.apPassword, 64);
  saveSettings();
  netApplyTime();
  weatherUpdate();
  sendJson("{\"ok\":true}");
}

void handleWifiScan(){
  int n = WiFi.scanNetworks();
  String j = "{\"networks\":[";
  for(int i=0;i<n && i<15;i++){ if(i) j += ","; j += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}"; }
  j += "]}"; WiFi.scanDelete(); sendJson(j);
}

void handleWifiSave(){
  String b = server.arg("plain");
  char ssid[33]={0}, pass[65]={0};
  auto getStr = [&](const char* k, char* out, int max){
    String pat = String("\"") + k + "\":\"";
    int s = b.indexOf(pat); if(s<0) return;
    s += pat.length(); int e = b.indexOf("\"", s);
    strncpy(out, b.substring(s,e).c_str(), max-1);
  };
  getStr("ssid", ssid, 33); getStr("pass", pass, 65);
  strncpy(cfg.wifiSsid, ssid, 32); cfg.wifiSsid[32] = 0;
  strncpy(cfg.wifiPass, pass, 64); cfg.wifiPass[64] = 0;
  saveSettings(); sendJson("{\"ok\":true}"); delay(500); ESP.restart();
}

void handleAnims(){
  String j = "{\"anims\":[";
  for(int i=0;i<A_COUNT;i++){ if(i) j += ","; j += "{\"id\":" + String(i) + ",\"name\":\"" + animName(i) + "\"}"; }
  j += "]}"; sendJson(j);
}

void handleAnimPlay(){
  String b = server.arg("plain");
  String pat = "\"id\":";
  int s = b.indexOf(pat); if(s<0) return sendJson("{\"ok\":false}");
  s += pat.length(); int e = b.indexOf(",", s); if(e<0) e = b.indexOf("}", s);
  int id = b.substring(s,e).toInt();
  if(id>=0 && id<A_COUNT) playThen(id, A_IDLE);
  sendJson("{\"ok\":true}");
}

// ★ ИСПРАВЛЕННЫЙ OTA: Разделен на загрузку и завершение
void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA Update: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void handleUpdateEnd() {
  server.sendHeader("Connection", "close");
  if (Update.hasError()) {
    server.send(502, "text/plain", "Update failed");
  } else {
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  }
}

void netBegin(){
  if(strlen(cfg.wifiSsid) > 0){
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifiSsid, cfg.wifiPass);
    uint32_t startAttempt = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) delay(100);
    if(WiFi.status() != WL_CONNECTED) startAP();
  } else {
    startAP();
  }
  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/settings", handleSettings);
  server.on("/api/settings/save", HTTP_POST, handleSettingsSave);
  server.on("/api/wifi/scan", handleWifiScan);
  server.on("/api/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/api/anims", handleAnims);
  server.on("/api/anim/play", HTTP_POST, handleAnimPlay);
  // ★ Правильная регистрация OTA
  server.on("/update", HTTP_POST, handleUpdateEnd, handleUpdateUpload);
  server.begin();
}

void netLoop(){ server.handleClient(); }