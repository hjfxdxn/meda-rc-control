# MEDA RC Control — UNO R4 WiFi 测试程序

两个独立测试程序及其浏览器界面。每次选择一个 `.ino` 上传到 Arduino UNO R4 WiFi；上传另一个程序会替换板上的程序。

| 测试 | Arduino 程序 | 浏览器界面 | 串口波特率 |
|---|---|---|---|
| 麦克风 + HC-SR04 | [microphone_test.ino](microphone_test/microphone_test.ino) | [sensor_monitor.html](microphone_test/sensor_monitor.html) | 9600 |
| ESC + 转向舵机 PWM | [pwm_bench_test.ino](pwm_bench_test/pwm_bench_test.ino) | [control_panel.html](pwm_bench_test/control_panel.html) | 115200 |

## 使用

1. 下载仓库（Code → Download ZIP）并解压，或使用 Git 克隆。
2. Arduino IDE 安装 Arduino UNO R4 Boards，选择 UNO R4 WiFi 及对应串口，打开所需 `.ino` 并上传。
3. 关闭 Arduino IDE 串口监视器和串口绘图器，以释放串口。
4. 用桌面版 Chrome 或 Edge 打开对应的本地 HTML 文件，点击连接并选择开发板。GitHub 的文件预览不是运行界面。
5. 更换测试程序前，先在网页中断开串口。

## 麦克风与超声波接线

断电后按模块丝印接线；两个模块与 Arduino 共地。

| 模块 | 模块引脚 | UNO R4 WiFi |
|---|---|---|
| 麦克风（支持 5V 的 AO/DO 模块） | VCC / + | 5V |
| 麦克风 | GND / G | GND |
| 麦克风 | AO | A0 |
| 麦克风 | DO | D7 |
| HC-SR04 | VCC | 5V |
| HC-SR04 | GND | GND |
| HC-SR04 | Trig | D9 |
| HC-SR04 | Echo | D11 |

麦克风检测指示使用板载 LED。默认 HIGH 为声音有效电平；如模块为 LOW 有效，请修改程序的 `SOUND_ACTIVE_LEVEL`，并在网页选择 LOW。

超声波使用默认 GPIO 模式，每 250 ms 测量一次。`Distance_cm:-1` 表示未测量或回波超时，不能视为没有障碍物。麦克风峰峰值不是分贝。程序等待回波时最多暂停麦克风采样 40 ms；网页中的 DO 是串口报告瞬间的状态，短促声音可能漏报。网页距离提示不控制车辆。

参考：[HC-SR04 V2020 说明书](https://akizukidenshi.com/goodsaffix/hc-sr04_v20.pdf)。

## PWM 台架测试

转向信号接 D5，ESC 信号接 D6，并共地。舵机与 ESC 按各自规格供电，不要用 Arduino GPIO 供电。通电前架空驱动轮。

程序中的脉宽来自课程示例，并非车辆实测标定值；中立不等于已验证的制动命令。测试结束后程序请求回中，必要时断开 ESC 电源停止测试。

已有标定记录见 [STEERING_CALIBRATION.md](STEERING_CALIBRATION.md)。

## 验证状态

传感器网页已检查 JavaScript 语法及正常、缺失字段、无回波数据的解析。仓库整理上传不代表已完成 Arduino 编译或实物联合测试。
