#pragma once
// Initialize after the actuator timers; render only on state changes.
ArduinoLEDMatrix statusMatrix;
bool matrixAttempted = false, statusMatrixReady = false;
int previousMark = -1;
constexpr uint8_t MATRIX_EIGHT[5] = {7,5,7,5,7};
constexpr uint8_t MATRIX_TWO[5] = {7,1,7,4,7};
// Preparing, armed, automatic drive, stopped, manual, idle, sound-test detected.
constexpr uint8_t MATRIX_MARKS[7][5] = {
  {0,0,15,0,0}, {0,1,2,10,4}, {4,2,1,2,4}, {0,9,6,6,9},
  {9,15,15,9,9}, {15,6,6,6,15}, {7,8,6,1,14}
};
int currentMatrixMark() {
  if (hardStop) return 3;
  if (controlMode == 1) return 4;
  if (soundTest) return soundTestDetected ? 6 : soundTestArmed ? 1 : 0;
  if (controlMode == 0) return 5;
  if (controller.state == race::State::Running) return 2;
  if (controller.state == race::State::Armed) return 1;
  if (controller.state == race::State::Stopped) return 3;
  return 0;
}
void updateMatrixStatus() {
  // Keep the early boot phase free of extra timer allocation.
  if (!matrixAttempted && (wifiAttempted || !ENABLE_WIFI)) {
    matrixAttempted = true;
    statusMatrixReady = statusMatrix.begin();
    if (!statusMatrixReady) Serial.println("LED_MATRIX_INIT_FAILED");
  }
  if (!statusMatrixReady) return;
  int mark = currentMatrixMark();
  if (mark == previousMark) return;
  uint8_t pixels[8][12] = {};
  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < 3; ++col) {
      pixels[row+1][col] = (MATRIX_EIGHT[row] >> (2-col)) & 1;
      pixels[row+1][col+4] = (MATRIX_TWO[row] >> (2-col)) & 1;
    }
    for (int col = 0; col < 4; ++col)
      pixels[row+1][col+8] = (MATRIX_MARKS[mark][row] >> (3-col)) & 1;
  }
  statusMatrix.renderBitmap(pixels,8,12);
  previousMark = mark;
}
