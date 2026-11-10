'use strict';
const $=id=>document.getElementById(id);
const definitions=[['s0','Custom pulse (us)',1000,1430],['s1','Slow pulse (us)',1000,1430],['s2','Normal pulse (us)',1000,1430],['n','ESC neutral (us)',1000,2000],['f','Forward test endpoint (us)',1000,2000],['r','Reverse test endpoint (us)',1000,2000],['c','Steering center (us)',900,2100],['l','Steering left bound (us)',900,2100],['u','Steering right bound (us)',900,2100],['stop','Stop distance (cm)',2,399],['side','Side target (cm)',2,400],['gain','Steering gain (0.1 us/cm)',1,200],['limit','Correction limit (us)',1,400],['time','Run timeout (seconds)',1,300],['win','Matching audio windows',1,20],['sideR','Side: 0 left / 1 right',0,1]];
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
$('connect').onclick=async()=>{try{$('connect').disabled=true;const response=await request('/claim','POST');const text=await response.text();if(!/^TOKEN:[0-9]+$/.test(text))throw new Error('Invalid session reply');token=Number(text.slice(6));sequence=0;ready=true;lastStatus=Date.now();for(const id of ['disconnect','idle'])$(id).disabled=false;say('Connected to car 82. Automatic sound start is unchanged. Idle disarms; STOP stops.');await pollStatus();}catch(e){say(e.message);$('connect').disabled=false;}};
$('disconnect').onclick=async()=>{release();if(pending)return;try{await send('RELEASE');}catch(e){say(e.message);}disconnected();};
async function pollInfo(){const response=await request('/info'),info=await response.json();$('identity').textContent=`Original board car: ${info.originalCar||'Unknown'} · Active vehicle: ${info.activeCar} · Station MAC: ${info.stationMac} · AP MAC: ${info.apMac}`;$('speedInfo').textContent=`Selected: ${['Custom','Slow','Normal'][info.speed]} · Forward request: ${info.forwardUs} us (not a measured speed)`;for(let i=0;i<3;i++)$('speedPreset').options[i].textContent=`${['Custom','Slow','Normal'][i]} — ${info.pulses[i]} us`; $('soundState').textContent=info.soundDetected?'Sound test: DETECTED — no motor command':info.soundArmed?'Sound test: ARMED — play the whistle':info.soundTest?'Sound test: WAIT CLEAR — quiet first':'Sound test: off';}
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
    if(!Number.isFinite(ts)||ts<1||ts>100||!Number.isFinite(ss)||ss<1||ss>100){release();throw new Error('Scale must be 1..100%.');}
    await send(`M,${held?Math.round(Number($('throttle').value)*ts/100):0},${held?Math.round(Number($('steer').value)*ss/100):0}`);
   }else await send('H');
  }
  if(Date.now()-lastStatus>=500)await pollStatus();
 }catch(e){release();if(manual)stopping=true;say(e.message);}
 finally{polling=false;}
},100);

pollStatus().catch(()=>say('Join the MEDA-Race82 hotspot and open http://192.168.4.1. This page is served by the Arduino.'));
