#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include "RTClib.h"
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= WIFI =================
const char* ssid = "SmartAquarium";
const char* password = "12345678";

WebServer server(80);
Preferences prefs;

// ================= LOGIN =================
String loginUser = "admin";
String loginPass = "1234";
bool loggedIn = false;

// ================= RTC =================
RTC_DS3231 rtc;

// ================= SERVO =================
Servo myServo;

// ================= TEMP =================
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temperature = 0;

// ================= ULTRASONIC =================
#define TRIG 12
#define ECHO 13
float tankHeight = 16.0;
float waterPercent = 0;

// ================= BUTTON =================
#define BUTTON_PIN 27
bool lastButtonState = HIGH;
unsigned long lastDebounce = 0;
const int debounceDelay = 300;

// ================= ALERT =================
#define BUZZER 19
#define LED 25

float minTemp  = 18;
float maxTemp  = 30;
float minLevel = 50;
float maxLevel = 90;

bool alertState = false;
String alertReason = "";

// Beep pattern (non-blocking)
unsigned long lastBeep = 0;
bool beepState = false;

// ================= SCHEDULE =================
int h1,m1,s1;
int h2,m2,s2;

// ================= SYSTEM =================
int feedCount = 0;
unsigned long lastFeed = 0;
const int COOLDOWN = 20000;

// ================= TIMER =================
unsigned long lastSensor = 0;

// ================= TIME =================
String timeStr(DateTime now){
  char buf[20];
  sprintf(buf,"%02d:%02d:%02d",now.hour(),now.minute(),now.second());
  return String(buf);
}

// ================= LOG =================
void saveLog(String reason){
  DateTime now = rtc.now();
  String logs = prefs.getString("logs","");
  logs += reason + " at " + timeStr(now) + "\n";
  prefs.putString("logs",logs);
  feedCount++;
  prefs.putInt("count",feedCount);
}

// ================= FEED =================
void feedFish(String reason){
  if(millis() - lastFeed < COOLDOWN) return;
  lastFeed = millis();
  myServo.write(90);
  delay(400);
  myServo.write(40);
  saveLog(reason);
}

// ================= SENSOR =================
void readSensors(){
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);

  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  float distance = duration * 0.034 / 2;
  float waterLevel = tankHeight - distance;
  waterPercent = (waterLevel / tankHeight) * 100;
  if(waterPercent > 100) waterPercent = 100;
  if(waterPercent < 0)   waterPercent = 0;
}

// ================= ALERT =================
// Alert fires when value is ABOVE max OR BELOW min
void checkAlerts(){
  alertState  = false;
  alertReason = "";

  if(temperature < minTemp){
    alertState = true;
    alertReason += "TEMP LOW ";
  }
  if(temperature > maxTemp){
    alertState = true;
    alertReason += "TEMP HIGH ";
  }
  if(waterPercent < minLevel){
    alertState = true;
    alertReason += "LEVEL LOW ";
  }
  if(waterPercent > maxLevel){
    alertState = true;
    alertReason += "LEVEL HIGH ";
  }

  digitalWrite(LED, alertState);

  if(alertState){
    // Non-blocking 500ms beep pattern
    if(millis() - lastBeep >= 500){
      beepState = !beepState;
      digitalWrite(BUZZER, beepState);
      lastBeep = millis();
    }
  } else {
    beepState = false;
    digitalWrite(BUZZER, LOW);
  }
}

// ================= LOGIN PAGE =================
void handleLogin(){
server.send(200,"text/html",R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@300;400;600&display=swap" rel="stylesheet">
<style>
*{margin:0;padding:0;box-sizing:border-box;}
body{
  background:#020c1b;
  color:#ccd6f6;
  font-family:'Rajdhani',sans-serif;
  min-height:100vh;
  display:flex;
  align-items:center;
  justify-content:center;
  overflow:hidden;
}
.bubbles{position:fixed;width:100%;height:100%;top:0;left:0;pointer-events:none;z-index:0;}
.bubble{
  position:absolute;
  bottom:-80px;
  border-radius:50%;
  background:rgba(0,200,255,0.07);
  border:1px solid rgba(0,200,255,0.15);
  animation:rise linear infinite;
}
@keyframes rise{
  0%{transform:translateY(0) scale(1);opacity:0.6;}
  100%{transform:translateY(-110vh) scale(1.2);opacity:0;}
}
.login-box{
  position:relative;z-index:1;
  background:rgba(2,18,40,0.85);
  border:1px solid rgba(0,200,255,0.2);
  border-radius:20px;
  padding:50px 40px;
  width:360px;
  backdrop-filter:blur(12px);
  box-shadow:0 0 60px rgba(0,200,255,0.08), inset 0 0 40px rgba(0,200,255,0.03);
  text-align:center;
}
.fish-icon{font-size:52px;margin-bottom:10px;animation:swim 3s ease-in-out infinite;}
@keyframes swim{0%,100%{transform:translateX(-6px);}50%{transform:translateX(6px);}}
h1{
  font-family:'Orbitron',monospace;
  font-size:1.1rem;
  letter-spacing:3px;
  color:#64ffda;
  margin-bottom:4px;
  text-transform:uppercase;
}
.sub{font-size:0.8rem;letter-spacing:2px;color:#4a6fa5;margin-bottom:36px;text-transform:uppercase;}
.field{
  background:rgba(0,200,255,0.04);
  border:1px solid rgba(0,200,255,0.15);
  border-radius:10px;
  padding:14px 18px;
  width:100%;
  color:#ccd6f6;
  font-family:'Rajdhani',sans-serif;
  font-size:1rem;
  margin-bottom:14px;
  outline:none;
  transition:border 0.3s,box-shadow 0.3s;
}
.field:focus{border-color:#64ffda;box-shadow:0 0 12px rgba(100,255,218,0.12);}
.field::placeholder{color:#4a6fa5;letter-spacing:1px;}
.btn{
  width:100%;
  padding:14px;
  background:linear-gradient(135deg,#00c8ff22,#64ffda22);
  border:1px solid #64ffda;
  border-radius:10px;
  color:#64ffda;
  font-family:'Orbitron',monospace;
  font-size:0.85rem;
  letter-spacing:3px;
  cursor:pointer;
  transition:all 0.3s;
  text-transform:uppercase;
  margin-top:6px;
}
.btn:hover{background:linear-gradient(135deg,#00c8ff44,#64ffda33);box-shadow:0 0 20px rgba(100,255,218,0.2);}
#msg{margin-top:16px;color:#ff6b6b;font-size:0.85rem;letter-spacing:2px;min-height:20px;}
</style>
</head>
<body>
<div class="bubbles" id="bb"></div>
<div class="login-box">
  <div class="fish-icon"></div>
  <h1>Smart Aquarium</h1>
  <div class="sub">Guardian System</div>
  <input class="field" id="u" placeholder="Username" autocomplete="off">
  <input class="field" id="p" type="password" placeholder="Password">
  <button class="btn" onclick="l()">ACCESS</button>
  <p id="msg"></p>
</div>
<script>
// Generate bubbles
const bb=document.getElementById('bb');
for(let i=0;i<18;i++){
  const b=document.createElement('div');
  b.className='bubble';
  const sz=Math.random()*50+10;
  b.style.cssText=`width:${sz}px;height:${sz}px;left:${Math.random()*100}%;animation-duration:${Math.random()*10+6}s;animation-delay:${Math.random()*8}s;`;
  bb.appendChild(b);
}
function l(){
  fetch(`/doLogin?u=${u.value}&p=${p.value}`)
  .then(r=>r.text()).then(d=>{
    if(d=="OK") location.href="/";
    else{ msg.innerText="⚠ INVALID CREDENTIALS"; }
  });
}
document.addEventListener('keydown',e=>{if(e.key==='Enter')l();});
</script>
</body>
</html>
)rawliteral");
}

// ================= LOGIN CHECK =================
void handleDoLogin(){
  if(server.arg("u")==loginUser && server.arg("p")==loginPass){
    loggedIn=true;
    server.send(200,"text/plain","OK");
  } else {
    server.send(200,"text/plain","FAIL");
  }
}

// ================= DASHBOARD =================
void handleRoot(){
if(!loggedIn){
  server.sendHeader("Location","/login");
  server.send(302,"text/plain","");
  return;
}
server.send(200,"text/html",R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Rajdhani:wght@300;400;600&display=swap" rel="stylesheet">
<style>
:root{
  --bg:#020c1b;
  --panel:rgba(2,18,40,0.85);
  --border:rgba(0,200,255,0.18);
  --accent:#64ffda;
  --accent2:#00c8ff;
  --warn:#ffb347;
  --danger:#ff6b6b;
  --text:#ccd6f6;
  --muted:#4a6fa5;
}
*{margin:0;padding:0;box-sizing:border-box;}
body{
  background:var(--bg);
  color:var(--text);
  font-family:'Rajdhani',sans-serif;
  min-height:100vh;
  padding:20px;
  transition:background 0.5s;
}
body.alert-mode{background:#1a0000;}

/* Bubbles BG */
.bubbles{position:fixed;width:100%;height:100%;top:0;left:0;pointer-events:none;z-index:0;}
.bubble{position:absolute;bottom:-80px;border-radius:50%;background:rgba(0,200,255,0.05);border:1px solid rgba(0,200,255,0.1);animation:rise linear infinite;}
@keyframes rise{0%{transform:translateY(0);opacity:0.5;}100%{transform:translateY(-110vh);opacity:0;}}

.wrapper{position:relative;z-index:1;max-width:800px;margin:0 auto;}

/* Header */
.header{text-align:center;margin-bottom:28px;}
.header .fish{font-size:48px;animation:swim 3s ease-in-out infinite;}
@keyframes swim{0%,100%{transform:translateX(-8px);}50%{transform:translateX(8px);}}
.header h1{font-family:'Orbitron',monospace;font-size:1.3rem;letter-spacing:4px;color:var(--accent);text-transform:uppercase;margin-top:6px;}
.header .sub{font-size:0.75rem;letter-spacing:3px;color:var(--muted);text-transform:uppercase;}

/* Alert Banner */
#alertBanner{
  display:none;
  background:rgba(255,107,107,0.12);
  border:1px solid rgba(255,107,107,0.4);
  border-radius:12px;
  padding:12px 20px;
  text-align:center;
  color:var(--danger);
  font-family:'Orbitron',monospace;
  font-size:0.75rem;
  letter-spacing:2px;
  margin-bottom:16px;
  animation:pulse 1s ease-in-out infinite;
}
#alertBanner.show{display:block;}
@keyframes pulse{0%,100%{opacity:1;}50%{opacity:0.5;}}

/* Grid */
.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:14px;}
@media(max-width:480px){.grid{grid-template-columns:1fr;}}

/* Cards */
.card{
  background:var(--panel);
  border:1px solid var(--border);
  border-radius:16px;
  padding:20px;
  backdrop-filter:blur(10px);
  transition:border 0.3s,box-shadow 0.3s;
  position:relative;
  overflow:hidden;
}
.card::before{
  content:'';position:absolute;top:0;left:0;right:0;height:2px;
  background:linear-gradient(90deg,transparent,var(--accent2),transparent);
  opacity:0.4;
}
.card.alert-card{border-color:rgba(255,107,107,0.5);box-shadow:0 0 20px rgba(255,107,107,0.1);}
.card.full{grid-column:1/-1;}
.card-label{
  font-size:0.65rem;letter-spacing:3px;color:var(--muted);
  text-transform:uppercase;margin-bottom:8px;
}
.card-value{
  font-family:'Orbitron',monospace;
  font-size:2.2rem;
  font-weight:700;
  color:var(--accent);
  line-height:1;
}
.card-value.warn{color:var(--warn);}
.card-value.danger{color:var(--danger);}
.card-icon{font-size:24px;position:absolute;top:16px;right:18px;opacity:0.4;}

/* Progress bar */
.bar-wrap{
  background:rgba(0,200,255,0.08);
  border-radius:8px;
  height:8px;
  margin-top:12px;
  overflow:hidden;
  border:1px solid rgba(0,200,255,0.1);
}
.bar{
  height:100%;
  border-radius:8px;
  background:linear-gradient(90deg,var(--accent2),var(--accent));
  transition:width 1s ease;
}
.bar.warn{background:linear-gradient(90deg,#ff8c00,var(--warn));}
.bar.danger{background:linear-gradient(90deg,#cc0000,var(--danger));}

/* Time card */
.time-big{
  font-family:'Orbitron',monospace;
  font-size:2.6rem;
  font-weight:900;
  color:var(--accent);
  text-align:center;
  letter-spacing:4px;
}

/* Divider */
.divider{
  grid-column:1/-1;
  height:1px;
  background:linear-gradient(90deg,transparent,var(--border),transparent);
  margin:4px 0;
}

/* Section title */
.section-title{
  grid-column:1/-1;
  font-family:'Orbitron',monospace;
  font-size:0.65rem;
  letter-spacing:4px;
  color:var(--muted);
  text-transform:uppercase;
  padding:8px 0 0 2px;
}

/* Feed button */
.feed-btn{
  width:100%;
  padding:16px;
  background:linear-gradient(135deg,rgba(100,255,218,0.08),rgba(0,200,255,0.08));
  border:1px solid var(--accent);
  border-radius:12px;
  color:var(--accent);
  font-family:'Orbitron',monospace;
  font-size:0.8rem;
  letter-spacing:3px;
  cursor:pointer;
  transition:all 0.3s;
  text-transform:uppercase;
  margin-top:10px;
}
.feed-btn:hover{background:linear-gradient(135deg,rgba(100,255,218,0.2),rgba(0,200,255,0.15));box-shadow:0 0 20px rgba(100,255,218,0.15);}
.feed-btn:active{transform:scale(0.97);}

/* Count badge */
.count-badge{
  display:inline-block;
  background:rgba(100,255,218,0.1);
  border:1px solid var(--accent);
  border-radius:8px;
  padding:4px 14px;
  font-family:'Orbitron',monospace;
  font-size:1.4rem;
  color:var(--accent);
  margin-top:6px;
}

/* Inputs */
.input-row{display:flex;gap:8px;margin-top:8px;}
.field-mini{
  flex:1;
  background:rgba(0,200,255,0.04);
  border:1px solid var(--border);
  border-radius:8px;
  padding:10px;
  color:var(--text);
  font-family:'Rajdhani',sans-serif;
  font-size:0.95rem;
  text-align:center;
  outline:none;
  transition:border 0.3s;
}
.field-mini:focus{border-color:var(--accent);}
.field-mini::placeholder{color:var(--muted);font-size:0.8rem;}
.field-label{font-size:0.65rem;letter-spacing:2px;color:var(--muted);text-transform:uppercase;margin-top:12px;margin-bottom:4px;}

/* Range display */
.range-info{
  font-size:0.75rem;color:var(--muted);margin-top:6px;letter-spacing:1px;
}

/* Save button */
.save-btn{
  width:100%;padding:14px;
  background:linear-gradient(135deg,rgba(0,200,255,0.1),rgba(100,255,218,0.1));
  border:1px solid var(--accent2);
  border-radius:12px;
  color:var(--accent2);
  font-family:'Orbitron',monospace;
  font-size:0.8rem;
  letter-spacing:3px;
  cursor:pointer;
  transition:all 0.3s;
  text-transform:uppercase;
  margin-top:8px;
}
.save-btn:hover{background:linear-gradient(135deg,rgba(0,200,255,0.2),rgba(100,255,218,0.15));box-shadow:0 0 20px rgba(0,200,255,0.15);}

/* Toast */
#toast{
  position:fixed;bottom:24px;right:24px;z-index:99;
  background:rgba(2,18,40,0.95);
  border:1px solid var(--accent);
  border-radius:10px;
  padding:12px 22px;
  font-family:'Orbitron',monospace;
  font-size:0.7rem;
  color:var(--accent);
  letter-spacing:2px;
  opacity:0;
  transform:translateY(10px);
  transition:all 0.4s;
  pointer-events:none;
}
#toast.show{opacity:1;transform:translateY(0);}

/* Log */
.log-box{
  background:rgba(0,0,0,0.4);
  border:1px solid var(--border);
  border-radius:10px;
  padding:14px;
  height:160px;
  overflow-y:auto;
  font-size:0.82rem;
  line-height:1.8;
  color:#7a9fc2;
  font-family:monospace;
  margin-top:8px;
}
.log-box::-webkit-scrollbar{width:4px;}
.log-box::-webkit-scrollbar-track{background:transparent;}
.log-box::-webkit-scrollbar-thumb{background:var(--border);border-radius:4px;}
</style>
</head>
<body>

<div class="bubbles" id="bb"></div>

<div class="wrapper">

  <!-- Header -->
  <div class="header">
    <div class="fish"></div>
    <h1>Smart Aquarium Guardian</h1>
    <div class="sub">Real-time Monitoring System</div>
  </div>

  <!-- Alert Banner -->
  <div id="alertBanner">ALERT: <span id="alertMsg"></span></div>

  <div class="grid">

    <!-- Time -->
    <div class="card full">
      <div class="card-label">System Time (RTC)</div>
      <div class="time-big" id="rtc">--:--:--</div>
    </div>

    <!-- Temperature -->
    <div class="card" id="tempCard">
      <div class="card-icon"></div>
      <div class="card-label">Water Temperature</div>
      <div class="card-value" id="temp">--°C</div>
      <div class="bar-wrap"><div class="bar" id="tempBar" style="width:50%"></div></div>
      <div class="range-info" id="tempRange">Range: -- ~ --°C</div>
    </div>

    <!-- Water Level -->
    <div class="card" id="levelCard">
      <div class="card-icon"></div>
      <div class="card-label">Water Level</div>
      <div class="card-value" id="level">--%</div>
      <div class="bar-wrap"><div class="bar" id="levelBar" style="width:50%"></div></div>
      <div class="range-info" id="levelRange">Range: -- ~ --%</div>
    </div>

    <div class="divider"></div>
    <div class="section-title"> Feeding</div>

    <!-- Feed Count + Button -->
    <div class="card full">
      <div class="card-label">Total Feed Count</div>
      <div class="count-badge" id="count">0</div>
      <button class="feed-btn" onclick="feed()">FEED NOW</button>
    </div>

    <div class="divider"></div>
    <div class="section-title">Schedule</div>

    <!-- Schedule 1 -->
    <div class="card">
      <div class="card-label">Schedule 1</div>
      <div class="input-row">
        <input class="field-mini" id="h1" placeholder="HH" maxlength="2">
        <input class="field-mini" id="m1" placeholder="MM" maxlength="2">
        <input class="field-mini" id="s1" placeholder="SS" maxlength="2">
      </div>
    </div>

    <!-- Schedule 2 -->
    <div class="card">
      <div class="card-label">Schedule 2</div>
      <div class="input-row">
        <input class="field-mini" id="h2" placeholder="HH" maxlength="2">
        <input class="field-mini" id="m2" placeholder="MM" maxlength="2">
        <input class="field-mini" id="s2" placeholder="SS" maxlength="2">
      </div>
    </div>

    <div class="divider"></div>
    <div class="section-title">Alert Thresholds</div>

    <!-- Temp Range -->
    <div class="card">
      <div class="card-label">Temperature Range (°C)</div>
      <div class="field-label">Min Temp</div>
      <input class="field-mini" id="minT" placeholder="e.g. 22" style="width:100%;">
      <div class="field-label">Max Temp</div>
      <input class="field-mini" id="maxT" placeholder="e.g. 30" style="width:100%;">
    </div>

    <!-- Level Range -->
    <div class="card">
      <div class="card-label">Water Level Range (%)</div>
      <div class="field-label">Min Level</div>
      <input class="field-mini" id="minL" placeholder="e.g. 40" style="width:100%;">
      <div class="field-label">Max Level</div>
      <input class="field-mini" id="maxL" placeholder="e.g. 90" style="width:100%;">
    </div>

    <!-- Save -->
    <div class="card full">
      <button class="save-btn" onclick="save()">SAVE SETTINGS</button>
    </div>

    <div class="divider"></div>
    <div class="section-title"> Feed Log</div>

    <!-- Logs -->
    <div class="card full">
      <div class="card-label">Activity Log</div>
      <div class="log-box" id="logs">Loading...</div>
    </div>

  </div><!-- /grid -->
</div><!-- /wrapper -->

<div id="toast">✓ SAVED</div>

<script>
// Bubbles
const bb=document.getElementById('bb');
for(let i=0;i<14;i++){
  const b=document.createElement('div');
  b.className='bubble';
  const sz=Math.random()*40+8;
  b.style.cssText=`width:${sz}px;height:${sz}px;left:${Math.random()*100}%;animation-duration:${Math.random()*12+7}s;animation-delay:${Math.random()*10}s;`;
  bb.appendChild(b);
}

let cachedMinT=22,cachedMaxT=30,cachedMinL=40,cachedMaxL=90;

function feed(){
  fetch('/feed').then(r=>r.text()).then(d=>{
    if(d==="OK") showToast("FED!");
    else showToast("COOLDOWN...");
  });
}

function save(){
  const url=`/save?h1=${h1.value||0}&m1=${m1.value||0}&s1=${s1.value||0}`+
            `&h2=${h2.value||0}&m2=${m2.value||0}&s2=${s2.value||0}`+
            `&minT=${minT.value}&maxT=${maxT.value}`+
            `&minL=${minL.value}&maxL=${maxL.value}`;
  fetch(url).then(()=>{
    cachedMinT=parseFloat(minT.value);
    cachedMaxT=parseFloat(maxT.value);
    cachedMinL=parseFloat(minL.value);
    cachedMaxL=parseFloat(maxL.value);
    updateRangeDisplay();
    showToast("SETTINGS SAVED");
  });
}

function updateRangeDisplay(){
  document.getElementById('tempRange').innerText=`Alert: < ${cachedMinT}°C or > ${cachedMaxT}°C`;
  document.getElementById('levelRange').innerText=`Alert: < ${cachedMinL}% or > ${cachedMaxL}%`;
}

function showToast(msg){
  const t=document.getElementById('toast');
  t.innerText='✓ '+msg;
  t.classList.add('show');
  setTimeout(()=>t.classList.remove('show'),2500);
}

// Fetch data + logs
setInterval(()=>{
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('count').innerText=d.count;
    document.getElementById('logs').innerHTML=d.logs||'No logs yet.';
    const lb=document.getElementById('logs');
    lb.scrollTop=lb.scrollHeight;
    // Load saved range into inputs once
    if(d.minT && minT.value===''){
      minT.value=d.minT; maxT.value=d.maxT;
      minL.value=d.minL; maxL.value=d.maxL;
      cachedMinT=d.minT; cachedMaxT=d.maxT;
      cachedMinL=d.minL; cachedMaxL=d.maxL;
      updateRangeDisplay();
    }
  });
},1500);

// RTC
setInterval(()=>{
  fetch('/time').then(r=>r.json()).then(d=>{
    document.getElementById('rtc').innerText=d.time;
  });
},1000);

// Sensor + alert
setInterval(()=>{
  fetch('/sensor').then(r=>r.json()).then(d=>{
    const tVal=parseFloat(d.temp);
    const lVal=parseFloat(d.level);
    const isAlert=(d.alert==1);

    // Temp display
    const tEl=document.getElementById('temp');
    const tCard=document.getElementById('tempCard');
    const tBar=document.getElementById('tempBar');
    tEl.innerText=tVal.toFixed(1)+'°C';
    const tPct=Math.min(Math.max(((tVal-10)/(50-10))*100,0),100);
    tBar.style.width=tPct+'%';
    if(tVal<cachedMinT||tVal>cachedMaxT){
      tEl.className='card-value danger';
      tBar.className='bar danger';
      tCard.className='card alert-card';
    } else {
      tEl.className='card-value';
      tBar.className='bar';
      tCard.className='card';
    }

    // Level display
    const lEl=document.getElementById('level');
    const lCard=document.getElementById('levelCard');
    const lBar=document.getElementById('levelBar');
    lEl.innerText=lVal.toFixed(1)+'%';
    lBar.style.width=lVal+'%';
    if(lVal<cachedMinL||lVal>cachedMaxL){
      lEl.className='card-value danger';
      lBar.className='bar danger';
      lCard.className='card alert-card';
    } else {
      lEl.className='card-value';
      lBar.className='bar';
      lCard.className='card';
    }

    // Alert banner + body
    const banner=document.getElementById('alertBanner');
    if(isAlert){
      banner.classList.add('show');
      document.getElementById('alertMsg').innerText=d.reason||'CHECK SENSORS';
      document.body.classList.add('alert-mode');
    } else {
      banner.classList.remove('show');
      document.body.classList.remove('alert-mode');
    }
  });
},2000);
</script>
</body>
</html>
)rawliteral");
}

// ================= APIs =================
void handleData(){
  String logs = prefs.getString("logs","");
  logs.replace("\n","<br>");
  int count = prefs.getInt("count",0);

  // Also send saved thresholds so dashboard can pre-fill inputs
  String json = "{\"count\":"+String(count)+
                ",\"logs\":\""+logs+"\""+
                ",\"minT\":"+String(minTemp)+
                ",\"maxT\":"+String(maxTemp)+
                ",\"minL\":"+String(minLevel)+
                ",\"maxL\":"+String(maxLevel)+"}";
  server.send(200,"application/json",json);
}

void handleTime(){
  DateTime now = rtc.now();
  server.send(200,"application/json",
  "{\"time\":\""+timeStr(now)+"\"}");
}

void handleSensor(){
  // Escape alertReason for JSON
  String reason = alertReason;
  reason.trim();

  String json = "{\"temp\":"+String(temperature,1)+
                ",\"level\":"+String(waterPercent,1)+
                ",\"alert\":"+String(alertState ? 1 : 0)+
                ",\"reason\":\""+reason+"\"}";
  server.send(200,"application/json",json);
}

// ================= SAVE =================
void handleSave(){
  h1=server.arg("h1").toInt();
  m1=server.arg("m1").toInt();
  s1=server.arg("s1").toInt();
  h2=server.arg("h2").toInt();
  m2=server.arg("m2").toInt();
  s2=server.arg("s2").toInt();

  minTemp  = server.arg("minT").toFloat();
  maxTemp  = server.arg("maxT").toFloat();
  minLevel = server.arg("minL").toFloat();
  maxLevel = server.arg("maxL").toFloat();

  prefs.putInt("h1",h1); prefs.putInt("m1",m1); prefs.putInt("s1",s1);
  prefs.putInt("h2",h2); prefs.putInt("m2",m2); prefs.putInt("s2",s2);

  prefs.putFloat("minTemp",minTemp);
  prefs.putFloat("maxTemp",maxTemp);
  prefs.putFloat("minLevel",minLevel);
  prefs.putFloat("maxLevel",maxLevel);

  server.send(200,"text/plain","Saved");
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);
  WiFi.softAP(ssid,password);

  prefs.begin("data",false);

  feedCount = prefs.getInt("count",0);
  h1=prefs.getInt("h1",0);  m1=prefs.getInt("m1",0);  s1=prefs.getInt("s1",0);
  h2=prefs.getInt("h2",0);  m2=prefs.getInt("m2",0);  s2=prefs.getInt("s2",0);

  minTemp  = prefs.getFloat("minTemp",22);
  maxTemp  = prefs.getFloat("maxTemp",30);
  minLevel = prefs.getFloat("minLevel",40);
  maxLevel = prefs.getFloat("maxLevel",90);

  rtc.begin();

  myServo.attach(18);
  myServo.write(40);

  sensors.begin();

  pinMode(TRIG,OUTPUT);
  pinMode(ECHO,INPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
  pinMode(LED,OUTPUT);

  server.on("/",handleRoot);
  server.on("/login",handleLogin);
  server.on("/doLogin",handleDoLogin);
  server.on("/data",handleData);
  server.on("/time",handleTime);
  server.on("/sensor",handleSensor);
  server.on("/save",handleSave);

  server.on("/feed",[](){
    if(!loggedIn){
      server.send(401,"text/plain","LOGIN REQUIRED");
      return;
    }
    feedFish("Manual");
    server.send(200,"text/plain","OK");
  });

  server.begin();
}

// ================= LOOP =================
void loop(){
  server.handleClient();

  if(millis() - lastSensor > 2000){
    readSensors();
    checkAlerts();
    lastSensor = millis();
  }

  DateTime now = rtc.now();

  if(now.hour()==h1 && now.minute()==m1 && now.second()>=s1 && now.second()<s1+2)
    feedFish("RTC1");

  if(now.hour()==h2 && now.minute()==m2 && now.second()>=s2 && now.second()<s2+2)
    feedFish("RTC2");

  bool current = digitalRead(BUTTON_PIN);
  if(current==LOW && lastButtonState==HIGH){
    if(millis()-lastDebounce>300){
      feedFish("BUTTON");
      lastDebounce=millis();
    }
  }
  lastButtonState=current;
}
