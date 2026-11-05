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
 'Correction limit (us)':'修正上限 (μs)', 'Run timeout (seconds)':'运行超时 (秒)', 'Matching audio windows':'连续匹配声音窗口数', 'Side: 0 left / 1 right':'侧方：0 左 / 1 右',
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
