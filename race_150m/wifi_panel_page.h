#pragma once
static const char WIFI_PANEL_HTML[] =
R"MEDAWIFI(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><title>MEDA Wi-Fi Control</title>
<style>
:root{font-family:system-ui,sans-serif;color:#172b3a;background:#edf1f5}body{max-width:820px;margin:auto;padding:20px}h1{font-size:28px;margin-bottom:4px}p{line-height:1.5}.panel{background:white;padding:20px;margin:16px 0;border-radius:14px}button,input,select{font:inherit}button{padding:13px 18px;border:0;border-radius:8px;background:#185cc9;color:white;cursor:pointer;min-height:48px}button:disabled{opacity:.4;cursor:default}.buttons{display:flex;gap:10px;flex-wrap:wrap}#stop{background:#bd2028;font-size:22px;font-weight:bold;width:100%;margin:16px 0}#hold{width:100%;margin-top:20px;background:#146849;touch-action:none;user-select:none}#hold.active{background:#ae5510}input[type=range]{width:100%;height:40px;touch-action:none}label{display:block;margin:12px 0 4px}.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.value{font-size:22px;font-weight:650;overflow-wrap:anywhere}.grid>div{min-width:0}small{color:#4d6172}input[type=number],select{box-sizing:border-box;width:100%;padding:9px;border:1px solid #aab8c5;border-radius:5px}#message{white-space:pre-wrap;overflow-wrap:anywhere}fieldset{border:0;margin:0;padding:0}details{margin:14px 0}summary{cursor:pointer;font-weight:600}.warn{color:#92330c}output{float:right}footer{font-size:13px;margin:24px 0}
</style></head><body>
<label>Language / 语言<select id="language"><option value="en">English</option><option value="zh">中文</option></select></label>
<h1>MEDA Wi-Fi Control</h1><p>Active car 82 · Front: D12 · Side: D11 · UNO R4 WiFi</p><p id="identity">Reading board identity...</p>
<div class="panel"><div class="buttons"><button id="connect">Connect Wi-Fi</button><button id="disconnect" disabled>Disconnect</button></div><p id="message" role="status">Connect your phone to the MEDA-Race82 hotspot, then open http://192.168.4.1 in Safari.</p><p class="warn">Bench and low-speed testing only. STOP requests ESC neutral; braking must be verified on the car.</p></div>
<button id="stop">STOP — LATCH UNTIL RESET</button>
<div class="panel"><div class="grid"><div><small>Mode / state</small><div class="value" id="mode">Disconnected</div></div><div><small>Actuator outputs</small><div class="value" id="outputs">Disabled</div></div><div><small>Front distance</small><div class="value" id="front">—</div></div><div><small>Side distance</small><div class="value" id="side">—</div></div><div><small>ESC / steering</small><div id="pulses">—</div></div><div><small>Sound diagnostics</small><div id="sound">—</div></div></div></div>
<div class="panel"><label>Automatic drive preset<select id="speedPreset"><option value="0">Ultra Slow — 1430 us</option><option value="1">Slow — 1410 us (trial)</option><option value="2">Normal — 1380 us (trial)</option></select></label><button id="setSpeed" disabled>Apply Speed Preset</button><p id="speedInfo">1430 us is the recorded forward onset. Other presets need loaded testing.</p><button id="soundTest" disabled>Test Sound Detection — No Motion</button><p id="soundState">Sound test: off</p><div class="buttons"><button id="idle" disabled>Idle / Disarm</button><button id="manual" disabled>Manual Mode</button><button id="auto" disabled>Arm Sound Start</button></div><p><small>Automatic waits for the sound cue. Manual does not use obstacle protection. Switch to Idle before changing mode.</small></p>
<fieldset id="drive" disabled><label>Throttle <output id="throttleValue">0%</output></label><input id="throttle" type="range" min="-100" max="100" value="0"><small>Reverse ← 0 → Forward</small><label>Steering <output id="steerValue">0%</output></label><input id="steer" type="range" min="-100" max="100" value="0"><small>Left ← Center → Right</small><div class="grid"><label>Throttle scale (%)<input id="throttleScale" type="number" min="1" max="100" value="25"></label><label>Steering)MEDAWIFI"
R"MEDAWIFI( scale (%)<input id="steerScale" type="number" min="1" max="100" value="20"></label></div><button id="hold">HOLD TO DRIVE</button><p><small>Choose slider positions, then hold. Release to request neutral and center. Missing manual commands latch STOP after 700 ms, plus loop/radio latency.</small></p></fieldset></div>
<div class="panel"><details><summary>Parameters — session only</summary><p>Use Idle. Enter measured ESC pulse values. These are endpoints for the test range, not verified full throttle. Actuator calibration locks when PWM initializes. Reset to change it.</p><fieldset id="parameters" disabled><div class="grid" id="fields"></div><div class="buttons"><button id="read">Read Parameters</button><button id="save">Apply Parameters</button></div></fieldset><p class="warn">Standalone sound start is enabled at boot. Settings are lost on reset.</p></details></div>
<footer>Wi-Fi runs locally between this device and the car. No external JavaScript or analytics. No automatic reconnection. STOP requires a physical board reset.</footer>
<script>
'use strict';
// Translate display text only. Protocol commands and input values stay unchanged.
const translations = {
 'MEDA Wi-Fi Control':'MEDA Wi-Fi 控制',
 'Active car 82 · Front: D12 · Side: D11 · UNO R4 WiFi':'当前车辆 82 · 前方 D12 · 侧方 D11 · UNO R4 WiFi',
 'Reading board identity...':'正在读取板子身份……',
 'Connect Wi-Fi':'连接控制', 'Disconnect':'断开控制', 'Disconnected':'未连接控制',
 'Connect your phone to the MEDA-Race82 hotspot, then open http://192.168.4.1 in Safari.':'手机连接 MEDA-Race82 热点，然后在 Safari 打开 http://192.168.4.1。',
 'Bench and low-speed testing only. STOP requests ESC neutral; braking must be verified on the car.':'先架空车轮或低速测试。停止按钮请求电调中立，实际制动效果需要验证。',
 'STOP — LATCH UNTIL RESET':'停止 — 复位后才能再次运行',
 'Mode / state':'模式 / 状态', 'Actuator outputs':'执行器输出', 'Disabled':'未启用',
 'Front distance':'前方距离', 'Side distance':'侧方距离', 'ESC / steering':'电调 / 转向脉宽', 'Sound diagnostics':'声音检测数据',
 'Automatic drive preset':'自动行驶速度档', 'Apply Speed Preset':'应用速度档',
 '1430 us is the recorded forward onset. Other presets need loaded testing.':'1430 μs 是已记录的前进启转值；其他档位需要负载测试。',
 'Test Sound Detection — No Motion':'测试声音识别 — 不驱动',
 'Idle / Disarm':'待机 / 取消起跑', 'Manual Mode':'手动遥控', 'Arm Sound Start':'准备声音起跑',
 'Automatic waits for the sound cue. Manual does not use obstacle protection. Switch to Idle before changing mode.':'自动模式等待哨声。手动模式不使用障碍停车。切换模式前先进入待机。',
 'Throttle':'油门', 'Steering':'转向', 'Reverse ← 0 → Forward':'后退 ← 0 → 前进', 'Left ← Center → Right':'左转 ← 中立 → 右转',
 'Throttle scale (%)':'油门比例 (%)', 'Steering scale (%)':'转向比例 (%)', 'HOLD TO DRIVE':'按住驱动',
 'Choose slider positions, then hold. Release to request neutral and center. Missing manual commands latch STOP after 700 ms, plus loop/radio latency.':'设好滑块后按住驱动，松开请求回中。手动指令中断 700 ms 后锁定停止，另有程序与无线延迟。',
 'Parameters — session only':'参数 — 仅本次有效',
 'Use Idle. Enter measured ESC pulse values. These are endpoints for the test range, not verified full throttle. Actuator calibration locks when PWM initializes. Reset to change it.':'在待机中设置参数。电调脉宽是测试范围端点，并非已验证的全油门值。PWM 初始化后锁定执行器标定；启动默认值需修改代码。',
 'Read Parameters':'读取参数', 'Apply Parameters':'应用参数',
 'Standalone sound start is enabled at boot. Settings are lost on reset.':'通电后自动准备声音起跑。网页参数在复位后恢复默认。',
 'Wi-Fi runs locally between this device and the car. No external JavaScript or analytics. No automatic reconnection. STOP requires a physical board reset.':'Wi-Fi 仅在本设备与车辆之间通信，不依赖互联网。不会自动重连；停止锁定后需要按板子复位键。',
 'Ultra Slow pulse (us)':'超慢档脉宽 (μs)', 'Slow pulse (us)':'慢速档脉宽 (μs)', 'Normal pulse (us)':'正常档脉宽 (μs)',
 'ESC neutral (us)':'电调中立 (μs)', 'Forward test endpoint (us)':'前进测试端点 (μs)', 'Reverse test endpoint (us)':'后退测试端点 (μs)',
 'Steering center (us)':'转向中立 (μs)', 'Steering left bound (us)':'转向左限位 (μs)', 'Steering right bound (us)':'转向右限位 (μs)',
 'Stop distance (cm)':'停车距离 (cm)', 'Side target (cm)':'侧方目标距离 (cm)', 'Steering gain (0.1 us/cm)':'转向增益 (0.1 μs/cm)',
 'Correction limit (us)':'修正上限 (μs)', 'Run timeout (seconds)':'运行超时 (秒))MEDAWIFI"
R"MEDAWIFI(', 'Matching audio windows':'连续匹配声音窗口数', 'Side: 0 left / 1 right':'侧方：0 左 / 1 右',
 'Not configured':'未设置', 'No valid echo':'无有效回波', 'STOP latched':'停止已锁定', 'PWM enabled':'PWM 已启用', 'Needs calibration':'需要标定', 'Disabled in firmware':'代码未启用输出',
 'Sound test: off':'独立声音测试：关闭', 'Sound test: DETECTED — no motor command':'声音测试：已检测 — 不驱动',
 'Sound test: ARMED — play the whistle':'声音测试：已准备 — 请播放哨声', 'Sound test: WAIT CLEAR — quiet first':'声音测试：先保持安静',
 'Disconnected. Automatic driving continues; manual driving stops.':'已断开控制。自动行驶继续；手动遥控停止。',
 'Connected to car 82. Automatic sound start is unchanged. Idle disarms; STOP stops.':'已连接 82 号车。自动起跑状态不变；待机取消起跑，停止按钮停车。',
 'Automatic armed. Waiting for a fresh sound cue. Phone connection is optional.':'自动起跑已准备，等待新的哨声；不要求手机持续连接。',
 'STOP requested. Waiting for acknowledgement.':'已请求停止，等待车辆确认。', 'STOP acknowledged. Reset the board to run again.':'车辆已确认停止。按板子复位键后才能再次运行。',
 'Parameters applied for this session only.':'参数已应用，仅本次运行有效。', 'Parameters read from the board.':'已读取板子参数。',
 'Join the MEDA-Race82 hotspot and open http://192.168.4.1. This page is served by the Arduino.':'请连接 MEDA-Race82 热点并打开 http://192.168.4.1。网页由 Arduino 提供。',
 'ERR settling':'正在等待中立稳定，请 3 秒后重试', 'ERR select IDLE':'请先选择待机', 'ERR reset required':'请先按板子复位键',
 'ERR configuration':'参数配置无效', 'ERR output disabled':'输出未启用', 'ERR control busy':'已有其他控制连接', 'ERR session':'控制会话已失效，请重新连接', 'ERR parameter':'参数无效或标定已锁定',
 'No control session. Connect again.':'没有控制会话，请重新连接。', 'Command pending.':'上一条指令尚未完成。', 'Scale must be 1..100%.':'比例必须为 1～100%。'
};
const displayTerms = {'Configuration':'等待配置','Settling':'中立稳定中','Waiting clear':'等待安静','Armed':'等待哨声','Running':'行驶中','Stopped':'已停止','Idle':'待机','Manual':'手动','Automatic':'自动','Ultra Slow':'超慢','Slow':'慢速','Normal':'正常'};
let language='en';
try { language=localStorage.getItem('meda-language')==='zh'?'zh':'en'; } catch (_) {}
const originalText=new WeakMap();
function translateText(text){
 if(translations[text])return translations[text];
 if(text.includes(' / ')) { const pieces=text.split(' / ');if(pieces.every(p=>displayTerms[p]))return pieces.map(p=>displayTerms[p]).join(' / '); }
 return text.replace(/Original board car: /g,'板子原属车辆：').replace(/Unknown/g,'未知').replace(/Active vehicle: /g,'当前车辆：').replace(/Station MAC: /g,'STA MAC：').replace(/AP MAC: /g,'AP MAC：')
 .replace(/Selected: /g,'当前档位：').replace(/Forward request: /g,'前进指令：').replace(/\(not a measured speed\)/g,'（不是实测车速）')
 .replace(/Ultra Slow|Slow|Normal/g,m=>displayTerms[m]).replace(/\(trial\)/g,'（试验值）')
 .replace(/^Requesting (.+)\.\.\.$/,'正在请求 $1……').replace(/^(.+) accepted\.$/,'$1 已接受。');
}
function renderLanguage(){
 document.documentElement.lang=language==='zh'?'zh-CN':'en';
 const walker=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT);
 let node;
 while((node=walker.nextNode())){
  if(node.parentElement.closest('script,style,#language'))continue;
  const current=node.nodeValue,record=originalText.get(node);
  const source=record&&current===record.rendered?record.source:current;
  const trimmed=source.trim();
  const rendered=language==='zh'?source.replace(trimmed,translateText(trimmed)):source;
  originalText.set(node,{source,rendered});if(current!==rendered)node.nodeValue=rendered;
 }
 for(const input of document.querySelectorAll('input[placeholder]'))input.placeholder=language==='zh'?'未设置':'Not configured';
}
const languagePicker=document.getElementById('language');languagePicker.value=language;
languagePicker.onchange=()=>{language=languagePicker.value;try{localStorage.setItem('meda-language',language);}catch(_){}renderLanguage();};
new MutationObserver(()=>renderLanguage()).observe(document.body,{subtree:true,childList:true,characterData:true});
renderLanguage();

</script><script>
'use strict';
const $=id=>document.getElementById(id);
const definitions=[['s0','Ultra Slow pulse (us)',1000,1430],['s1','Slow pulse (us)',1000,1430],['s2','Normal pulse (us)',1000,1430],['n','ESC neutral (us)',1000,2000],['f','F)MEDAWIFI"
R"MEDAWIFI(orward test endpoint (us)',1000,2000],['r','Reverse test endpoint (us)',1000,2000],['c','Steering center (us)',900,2100],['l','Steering left bound (us)',900,2100],['u','Steering right bound (us)',900,2100],['stop','Stop distance (cm)',2,399],['side','Side target (cm)',2,400],['gain','Steering gain (0.1 us/cm)',1,200],['limit','Correction limit (us)',1,400],['time','Run timeout (seconds)',1,300],['win','Matching audio windows',1,20],['sideR','Side: 0 left / 1 right',0,1]];
for(const [key,label,min,max]of definitions){const l=document.createElement('label');l.textContent=label;const i=document.createElement('input');Object.assign(i,{id:`p-${key}`,type:'number',min,max,step:1,placeholder:'Not configured'});l.append(i);$('fields').append(l);}
let token=0,sequence=0,ready=false,held=false,manual=false,busy=false,stopping=false,mode=0,flags=0,lastStatus=0,pending=null;
const say=text=>{$('message').textContent=text;};
function resetControls(){held=false;manual=false;mode=0;flags=0;$('hold').classList.remove('active');$('throttle').value=$('steer').value=0;labels();$('drive').disabled=true;for(const id of ['disconnect','idle','manual','auto','parameters','setSpeed','soundTest'])$(id).disabled=true;}
function release(){held=false;$('hold').classList.remove('active');}
function labels(){$('throttleValue').textContent=$('throttle').value+'%';$('steerValue').textContent=$('steer').value+'%';}
for(const id of ['throttle','steer'])$(id).addEventListener('input',labels);
async function request(url,method='GET') {
 const abort=new AbortController(),timer=setTimeout(()=>abort.abort(),1400);
 try {const response=await fetch(url,{method,cache:'no-store',signal:abort.signal});if(!response.ok)throw new Error(await response.text());return response;}finally{clearTimeout(timer);}
}
async function send(text){
 if(!ready||!token)throw new Error('No control session. Connect again.');
 if(pending)throw new Error('Command pending.');
 pending=true;
 try {const response=await request(`/cmd?token=${token}&seq=${++sequence}&c=${text}`,'POST');const result=await response.text();if(result.startsWith('ERR'))throw new Error(result);return result;}finally{pending=null;}
}
function disconnected(){ready=false;token=0;release();resetControls();$('connect').disabled=false;$('mode').textContent='Disconnected';say('Disconnected. Automatic driving continues; manual driving stops.');}
function telemetry(event){const d=event.target.value;if(d.byteLength!==20||d.getUint8(0)!==1)return;lastStatus=Date.now();mode=d.getUint8(1);flags=d.getUint8(4);const states=['Configuration','Settling','Waiting clear','Armed','Running','Stopped'];$('mode').textContent=['Idle','Manual','Automatic'][mode]+' / '+states[d.getUint8(2)];$('outputs').textContent=(flags&2)?'STOP latched':(flags&1)?'PWM enabled':(flags&4)?'Needs calibration':'Disabled in firmware';for(const[id,offset]of [['front',6],['side',8]]){const mm=d.getUint16(offset,true);$(id).textContent=mm===65535?'No valid echo':(mm/10).toFixed(1)+' cm';}$('pulses').textContent=d.getUint16(10,true)+' / '+d.getUint16(12,true)+' us';$('sound').textContent=`RMS ${(d.getUint16(14,true)/10).toFixed(1)} · Tone ${(d.getUint16(16,true)/10).toFixed(1)}% · ${String.fromCharCode(d.getUint8(18))}`;manual=mode===1&&!(flags&2);$('drive').disabled=!manual;if(!manual)release();$('parameters').disabled=mode!==0||Boolean(flags&2);$('setSpeed').disabled=!ready||mode!==0||Boolean(flags&2);$('soundTest').disabled=!ready||mode!==0||Boolean(flags&2);$('manual').disabled=mode!==0||!(flags&4)||Boolean(flags&2);$('auto').disabled=mode!==0||!(flags&4)||!(flags&8)||Boolean(flags&2);}
$('connect').onclick=async()=>{try{$('connect').disabled=true;const response=await request('/claim','POST');const text=await response.text();if(!/^TOKEN:[0-9]+$/.test(text))throw new Error('Invalid session reply');token=Number(text.slice(6));sequence=0;ready=true;lastStatus=Date.now();for(const id of ['disconnect','idle'])$(id).disabled=false;say('Connected to car 82. Automatic sound s)MEDAWIFI"
R"MEDAWIFI(tart is unchanged. Idle disarms; STOP stops.');await pollStatus();}catch(e){say(e.message);$('connect').disabled=false;}};
$('disconnect').onclick=async()=>{release();if(pending)return;try{await send('RELEASE');}catch(e){say(e.message);}disconnected();};
async function pollInfo(){const response=await request('/info'),info=await response.json();$('identity').textContent=`Original board car: ${info.originalCar||'Unknown'} · Active vehicle: ${info.activeCar} · Station MAC: ${info.stationMac} · AP MAC: ${info.apMac}`;$('speedInfo').textContent=`Selected: ${['Ultra Slow','Slow','Normal'][info.speed]} · Forward request: ${info.forwardUs} us (not a measured speed)`;for(let i=0;i<3;i++)$('speedPreset').options[i].textContent=`${['Ultra Slow','Slow','Normal'][i]} — ${info.pulses[i]} us`; $('soundState').textContent=info.soundDetected?'Sound test: DETECTED — no motor command':info.soundArmed?'Sound test: ARMED — play the whistle':info.soundTest?'Sound test: WAIT CLEAR — quiet first':'Sound test: off';}
async function pollStatus(){const response=await request('/status');telemetry({target:{value:new DataView(await response.arrayBuffer())}});if(mode===0)await pollInfo();}
async function modeCommand(text){if(busy||pending)return;say(`Requesting ${text}...`);busy=true;release();try{await send(text);if(text==='MANUAL')await send('M,0,0');await pollStatus();say(text==='AUTO'?'Automatic armed. Waiting for a fresh sound cue. Phone connection is optional.':`${text} accepted.`);}catch(e){say(e.message);}finally{busy=false;}}
for(const[id,text]of [['idle','IDLE'],['manual','MANUAL'],['auto','AUTO']])$(id).onclick=()=>modeCommand(text);
$('setSpeed').onclick=()=>modeCommand('SPEED,'+$('speedPreset').value);
$('soundTest').onclick=()=>modeCommand('SOUND');
$('stop').onclick=()=>{release();stopping=true;say('STOP requested. Waiting for acknowledgement.');};
$('hold').addEventListener('pointerdown',e=>{if(!manual||busy||stopping)return;e.preventDefault();$('hold').setPointerCapture(e.pointerId);held=true;$('hold').classList.add('active');});
for(const name of ['pointerup','pointercancel','lostpointercapture'])$('hold').addEventListener(name,release);
window.addEventListener('blur',()=>{release();if(mode===1)stopping=true;});
document.addEventListener('visibilitychange',()=>{if(document.hidden&&mode===1){release();stopping=true;if(token)fetch(`/cmd?token=${token}&seq=${++sequence}&c=STOP`,{method:'POST',keepalive:true}).catch(()=>{});}});
window.addEventListener('pagehide',()=>{release();if(mode===1&&token)fetch(`/cmd?token=${token}&seq=${++sequence}&c=STOP`,{method:'POST',keepalive:true}).catch(()=>{});});
async function parameters(save){if(busy||pending||mode!==0)return;busy=true;try{for(const[key,,min,max]of definitions){if(stopping)break;const field=$(`p-${key}`);if(save){if(field.value==='')continue;const v=Number(field.value);if(!Number.isInteger(v)||v<min||v>max)throw new Error(`Invalid ${key}: allowed ${min}..${max}.`);await send(`P,${key},${v}`);}else{const response=await send(`G,${key}`),parts=response.split(',');if(parts[0]!=='V'||parts[1]!==key)throw new Error('Unexpected parameter reply');field.value=Number(parts[2])===0&&min>0?'':parts[2];}}await pollStatus();say(save?'Parameters applied for this session only.':'Parameters read from the board.');}catch(e){say(e.message);}finally{busy=false;}}
$('read').onclick=()=>parameters(false);$('save').onclick=()=>parameters(true);
let polling=false;
setInterval(async()=>{
 if(polling)return;
 polling=true;
 try {
  if(stopping){
   const response=await request('/stop','POST');
   const result=await response.text();
   if(result!=='OK STOP')throw new Error(result);
   stopping=false;release();await pollStatus();
   say('STOP acknowledged. Reset the board to run again.');return;
  }
  if(busy||pending)return;
  if(ready){
   if(manual){
    if(Date.now()-lastStatus>2000){release();stopping=true;return;}
    const ts=Number($('throttleScale').value),ss=Number($('steerScale').value);
    if(!Number.isFi)MEDAWIFI"
R"MEDAWIFI(nite(ts)||ts<1||ts>100||!Number.isFinite(ss)||ss<1||ss>100){release();throw new Error('Scale must be 1..100%.');}
    await send(`M,${held?Math.round(Number($('throttle').value)*ts/100):0},${held?Math.round(Number($('steer').value)*ss/100):0}`);
   }else await send('H');
  }
  if(Date.now()-lastStatus>=500)await pollStatus();
 }catch(e){release();if(manual)stopping=true;say(e.message);}
 finally{polling=false;}
},100);

pollStatus().catch(()=>say('Join the MEDA-Race82 hotspot and open http://192.168.4.1. This page is served by the Arduino.'));

</script></body></html>
)MEDAWIFI";
