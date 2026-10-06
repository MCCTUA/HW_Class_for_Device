#ifndef DASHBOARD_PAGE_H
#define DASHBOARD_PAGE_H

#include <Arduino.h>

// หน้า dashboard (HTML/CSS/JS ในไฟล์เดียว) เก็บใน flash
// หน้าเว็บดึง GET /api/status ทุก 2 วินาที และสั่ง relay ด้วย POST /api/relay?id=1&state=on|off|toggle
static const char DASHBOARD_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="th">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Dashboard</title>
<style>
:root{--bg:#f4f5f7;--card:#fff;--text:#1c1f24;--muted:#6b7280;--line:#e5e7eb;--on:#16a34a;--off:#9ca3af;--accent:#2563eb}
@media (prefers-color-scheme:dark){:root{--bg:#0f1115;--card:#181b21;--text:#e8eaed;--muted:#9aa0aa;--line:#2a2f38;--on:#22c55e;--off:#6b7280;--accent:#60a5fa}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
header{display:flex;justify-content:space-between;align-items:center;padding:16px;max-width:900px;margin:0 auto}
h1{font-size:18px;margin:0}
#conn{font-size:13px;color:var(--muted)}
#conn.bad{color:#dc2626}
main{display:grid;gap:16px;padding:0 16px 24px;max-width:900px;margin:0 auto;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px}
h2{font-size:13px;font-weight:600;color:var(--muted);text-transform:uppercase;letter-spacing:.04em;margin:0 0 12px}
.row{display:flex;justify-content:space-between;gap:12px;padding:6px 0;border-bottom:1px solid var(--line)}
.row:last-child{border-bottom:0}
.row span:first-child{color:var(--muted)}
.row span:last-child{font-variant-numeric:tabular-nums;text-align:right;word-break:break-all}
.big{font-size:44px;font-weight:600;line-height:1.1;font-variant-numeric:tabular-nums}
.big small{font-size:20px;color:var(--muted);font-weight:400}
.badge{display:inline-block;font-size:11px;font-weight:600;padding:2px 8px;border-radius:99px;vertical-align:middle;margin-left:8px;border:1px solid var(--on);color:var(--on)}
.badge.sim{border-color:#d97706;color:#d97706}
#spark{width:100%;height:70px;display:block;margin-top:8px}
.relay{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-bottom:1px solid var(--line)}
.relay:last-child{border-bottom:0}
.dot{display:inline-block;width:10px;height:10px;border-radius:50%;background:var(--off);margin-right:8px}
.relay.on .dot{background:var(--on)}
button{font:inherit;border:1px solid var(--line);background:var(--card);color:var(--text);border-radius:8px;padding:6px 16px;cursor:pointer;min-width:84px}
.relay.on button{background:var(--on);border-color:var(--on);color:#fff}
.topic{display:flex;gap:8px;align-items:baseline;padding:6px 0;border-bottom:1px solid var(--line)}
.topic:last-child{border-bottom:0}
.topic code{font:12px ui-monospace,Menlo,monospace;word-break:break-all;flex:1}
.topic small{color:var(--muted)}
.tag{font-size:10px;font-weight:700;padding:1px 6px;border-radius:4px;border:1px solid var(--accent);color:var(--accent);min-width:34px;text-align:center}
.tag.sub{border-color:#d97706;color:#d97706}
button:disabled{opacity:.5;cursor:wait}
</style>
</head>
<body>
<header><h1>ESP32 Dashboard</h1><span id="conn">connecting…</span></header>
<main>
<section><h2>Temperature — DS18B20<span id="tbadge" class="badge"></span></h2><div id="temp"></div></section>
<section><h2>Temp &amp; Humidity — XY-MD03<span id="xbadge" class="badge"></span></h2><div id="xymd"></div></section>
<section><h2>Relay</h2><div id="relays"></div></section>
<section><h2>Weather — <span id="city">-</span></h2><div id="weather"></div></section>
<section style="grid-column:1/-1"><h2>MQTT<span id="mbadge" class="badge"></span></h2><div id="mqtt"></div></section>
<section><h2>WiFi</h2><div id="wifi"></div></section>
</main>
<script>
const $=id=>document.getElementById(id);
const rows=list=>list.map(([k,v])=>`<div class="row"><span>${k}</span><span>${v}</span></div>`).join('');
const aqiText=['-','Good','Fair','Moderate','Poor','Very Poor'];
const fmtUp=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m'};
const sig=r=>r>=-60?'Excellent':r>=-70?'Good':r>=-80?'Fair':'Weak';

function spark(h){
  const c=$('spark');if(!c)return;
  const dpr=window.devicePixelRatio||1,W=c.clientWidth,H=c.clientHeight;
  c.width=W*dpr;c.height=H*dpr;
  const x=c.getContext('2d');x.scale(dpr,dpr);
  if(h.length<2)return;
  const lo=Math.min(...h),hi=Math.max(...h),sp=Math.max(hi-lo,1);
  const col=getComputedStyle(document.body).getPropertyValue('--accent');
  x.strokeStyle=col;x.lineWidth=2;x.lineJoin='round';x.beginPath();
  h.forEach((v,i)=>{const px=i/(h.length-1)*W,py=H-4-(v-lo)/sp*(H-8);i?x.lineTo(px,py):x.moveTo(px,py)});
  x.stroke();
}
function render(s){
  const t=s.temp;
  $('tbadge').textContent=t.sim?'SIMULATION':'LIVE';$('tbadge').className='badge'+(t.sim?' sim':'');
  $('temp').innerHTML=t.has?`<div class="big">${t.value.toFixed(1)}<small> °C</small></div>`+
    rows([['Min',t.min.toFixed(1)+' °C'],['Max',t.max.toFixed(1)+' °C'],['Source',t.sim?'Simulated (no sensor)':'DS18B20 on GPIO14']])+
    '<canvas id="spark"></canvas>':rows([['Status','Reading…']]);
  if(t.has)spark(t.history);
  const x=s.xymd;
  $('xbadge').textContent=x.sim?'SIMULATION':'LIVE';$('xbadge').className='badge'+(x.sim?' sim':'');
  $('xymd').innerHTML=x.has?`<div class="big">${x.temp.toFixed(1)}<small> °C</small></div>`+
    `<div class="big" style="margin-top:8px">${x.hum.toFixed(1)}<small> %RH</small></div>`+
    rows([['Source',x.sim?'Simulated (no sensor)':'XY-MD03 (RS485, ID '+x.id+')']]):rows([['Status','Reading…']]);
  $('relays').innerHTML=s.relays.map(r=>`<div class="relay ${r.on?'on':''}">
    <span><i class="dot"></i>Relay ${r.id}</span>
    <button data-id="${r.id}">${r.on?'ON':'OFF'}</button></div>`).join('');
  $('city').textContent=s.weather.city;
  const w=s.weather;
  $('weather').innerHTML=w.valid?rows([
    ['Temperature',w.temp.toFixed(1)+' °C'],['Humidity',w.hum+' %'],
    ['PM2.5',w.pm25.toFixed(1)+' µg/m³'],['AQI',w.aqi+' ('+aqiText[w.aqi]+')'],
    ['Rain chance',w.rain+' %'],['Updated',fmtUp(w.age)+' ago']
  ]):rows([['Status',w.fetched?'Error':'Loading…']]);
  const m=s.mqtt;
  $('mbadge').textContent=m.connected?'CONNECTED':'DISCONNECTED';$('mbadge').className='badge'+(m.connected?'':' sim');
  $('mqtt').innerHTML=rows([['Broker',m.host+':'+m.port],['Base topic',m.base],
    ['Last publish',m.last_pub<0?'-':m.last_pub+' s ago'],['Last command',m.last_cmd||'-']])+
    m.topics.map(t=>`<div class="topic"><span class="tag ${t.dir}">${t.dir.toUpperCase()}</span><code>${t.topic}</code><small>${t.desc}</small></div>`).join('');
  const f=s.wifi;
  $('wifi').innerHTML=rows([
    ['SSID',f.ssid],['Signal',f.rssi+' dBm ('+sig(f.rssi)+')'],['IP',f.ip],
    ['Gateway',f.gateway],['MAC',f.mac],['Uptime',fmtUp(s.uptime)]
  ]);
}

async function load(){
  try{
    const r=await fetch('/api/status');
    render(await r.json());
    $('conn').textContent='online';$('conn').className='';
  }catch(e){$('conn').textContent='offline';$('conn').className='bad'}
}

$('relays').addEventListener('click',async e=>{
  const b=e.target.closest('button');if(!b)return;
  b.disabled=true;
  try{await fetch('/api/relay?id='+b.dataset.id+'&state=toggle',{method:'POST'})}catch(_){}
  await load();
});

load();setInterval(load,2000);
</script>
</body>
</html>
)HTML";

#endif // DASHBOARD_PAGE_H
