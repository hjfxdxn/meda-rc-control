// Arduino UNO R4 WiFi マイク + HC-SR04 動作確認
// HC-SR04: VCC -> 5V, GND -> GND, Trig -> D9, Echo -> D11
// HC-SR04 V2020のデフォルトGPIOモードを使用．測定間隔は200 ms以上．
// 対象：KY-037 / KY-038 など、AO・DO出力を持つマイクモジュール
// 配線：AO -> A0、DO -> D7、GND -> GND
// VCCは使用するモジュールの仕様に合わせること（5V対応品は5V）．
// D8はIMU用に予約されているため、確認用LEDにはLED_BUILTINを使う．

const int led = LED_BUILTIN;
const int digitalPin = 7;
const int analogPin = A0;
const int ultrasonicTrigPin = 9;
const int ultrasonicEchoPin = 11;
const unsigned long RANGE_INTERVAL_MS = 250UL;
const unsigned long ECHO_TIMEOUT_US = 40000UL;
unsigned long lastRangeMs = 0;
float distanceCm = -1.0f; // -1: 未測定またはタイムアウト

float readDistanceCm()
{
  digitalWrite(ultrasonicTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultrasonicTrigPin, HIGH);
  delayMicroseconds(12); // 仕様: 10 usより長いパルス
  digitalWrite(ultrasonicTrigPin, LOW);
  const unsigned long echoUs = pulseIn(ultrasonicEchoPin, HIGH, ECHO_TIMEOUT_US);
  if (echoUs == 0) return -1.0f;
  return echoUs * 0.0343f / 2.0f; // 約20℃での音速、往復距離を補正
}

// 元のプログラムと同じくHIGHでLEDを点灯．
// 音を検出したときDOがLOWになるモジュールではLOWに変更する．
const int SOUND_ACTIVE_LEVEL = HIGH;
const float ADC_REFERENCE_V = 5.0f;  // 公称値．精密な電圧測定用ではない
const int ADC_MAX = 4095;           // 12 bit：0～4095
const unsigned long SAMPLE_WINDOW_US = 20000UL; // 20 ms
const unsigned long REPORT_INTERVAL_MS = 100UL;
const unsigned long LED_HOLD_MS = 150UL; // 短い反応を目視できるよう保持

unsigned long lastReportMs = 0;
unsigned long lastDetectionMs = 0;
bool detectionSeen = false;

void setup()
{
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(digitalPin, INPUT);
  pinMode(ultrasonicTrigPin, OUTPUT);
  digitalWrite(ultrasonicTrigPin, LOW);
  pinMode(ultrasonicEchoPin, INPUT);
  analogReadResolution(12); // UNO R4のADC読み取り分解能を明示
  Serial.begin(9600);
  // Serial接続待ちはしない．USB未接続でもLEDで確認可能．
}

void loop()
{
  if (millis() - lastRangeMs >= RANGE_INTERVAL_MS) {
    lastRangeMs = millis();
    distanceCm = readDistanceCm();
  }

  int minValue = ADC_MAX;
  int maxValue = 0;
  unsigned long sum = 0;
  unsigned long count = 0;
  bool detectedInWindow = false;
  const unsigned long windowStartUs = micros();

  // 100 msごとの単発測定では短い音を見落としやすいため、
  // 20 ms間サンプリングして振幅（最大値－最小値）を求める．
  do {
    const int value = analogRead(analogPin);
    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
    sum += value;
    ++count;

    if (digitalRead(digitalPin) == SOUND_ACTIVE_LEVEL) {
      detectedInWindow = true;
    }
  } while (micros() - windowStartUs < SAMPLE_WINDOW_US);

  const unsigned long nowMs = millis();
  if (detectedInWindow) {
    lastDetectionMs = nowMs;
    detectionSeen = true;
  }
  digitalWrite(led, detectionSeen &&
                    (nowMs - lastDetectionMs < LED_HOLD_MS) ? HIGH : LOW);

  if (nowMs - lastReportMs >= REPORT_INTERVAL_MS) {
    lastReportMs = nowMs;
    const float meanVoltage = (sum / (float)count) * ADC_REFERENCE_V / ADC_MAX;
    const float peakToPeakVoltage = (maxValue - minValue) * ADC_REFERENCE_V / ADC_MAX;
    const int digitalValue = digitalRead(digitalPin);

    // シリアルモニタ／プロッタ：9600 baud
    // DOはその瞬間の生値．LEDは短い検出を150 ms保持する．
    Serial.print("Mean_V:");
    Serial.print(meanVoltage, 3);
    Serial.print("\tPeakToPeak_V:");
    Serial.print(peakToPeakVoltage, 3);
    Serial.print("\tDO:");
    Serial.print(digitalValue);
    Serial.print("\tDistance_cm:");
    Serial.println(distanceCm, 1);
  }
}

// 注意：これは単独のテスト用スケッチ．サーボやESCは駆動しない．
// サンプリング区間とSerial出力はブロッキングするため、
// 自動走行プログラムへ組み込む場合は処理方式を見直すこと．
// 超音波のpulseIn待機中（最大40 ms）はマイクを取得しない．
// Distance_cmは最新の測定値を保持．-1を障害物なしと解釈しないこと．
// ポテンショメータは多くのモジュールでDOの比較しきい値を調整する．
// AOを必ず2.5 Vに調整するものではない．音圧レベル(dB)は測定しない．
