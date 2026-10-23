#pragma once
#include <WiFiS3.h>
#include "control_panel_page.h"

// Local bench access only. Change this shared demonstration password before use.
constexpr char kBenchPassword[] = "MedaBench2026";
constexpr int kCarOverride = 0; // 0 = exact MAC lookup; otherwise 81, 82 or 83.

class BenchLog : public Print {
public:
  String response;
  bool capture = false;
  size_t write(uint8_t c) override {
    if (Serial) Serial.write(c);
    if (capture && response.length() < 2048) response += char(c);
    return 1;
  }
} benchLog;

void setNeutral();
void reportConfiguration();
void processCommandByte(char command);
extern bool receivingThrottle;
extern bool testActive;
extern bool commandFault;
extern bool outputsReady;

WiFiServer benchServer(80);
WiFiClient benchClient;
bool wifiReady = false, remoteMotion = false;
uint32_t remoteToken = 0, tokenCounter = 0, remoteSeen = 0, remoteSequence = 0;
uint32_t requestStarted = 0;
String requestLine;
bool readingHeaders = false;
uint16_t requestBytes = 0;
int carNumber = 0;
char boardMac[18] = "00:00:00:00:00:00";
char benchSsid[40];

void revokeRemote() {
  remoteToken = 0;
  receivingThrottle = false;
}

void startBenchWiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    benchLog.println("Wi-Fi unavailable; USB control remains available."); return;
  }
  // Read identity before entering AP mode (the AP interface may use a different MAC).
  uint8_t mac[6] = {};
  WiFi.macAddress(mac);
  snprintf(boardMac, sizeof(boardMac), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  const char* known[] = {"F4:12:FA:70:5D:80", "D8:85:AC:A8:E9:6C", "34:B7:DA:63:8C:7C"};
  for (int i = 0; i < 3; ++i) if (!strcmp(boardMac, known[i])) carNumber = 81 + i;
  if (kCarOverride >= 81 && kCarOverride <= 83) carNumber = kCarOverride;
  if (carNumber) snprintf(benchSsid, sizeof(benchSsid), "MEDA-Car-%d", carNumber);
  else snprintf(benchSsid, sizeof(benchSsid), "MEDA-Unknown-%02X%02X%02X", mac[3], mac[4], mac[5]);
  wifiReady = WiFi.beginAP(benchSsid, kBenchPassword) == WL_AP_LISTENING;
  if (wifiReady) {
    benchServer.begin();
    benchLog.print("Wi-Fi AP: "); benchLog.println(benchSsid);
    benchLog.print("Open http://"); benchLog.println(WiFi.localIP());
    benchLog.print("Board MAC: "); benchLog.println(boardMac);
  } else benchLog.println("Wi-Fi AP unavailable; USB control remains available.");
}

void replyHttp(int status, const char* body, const char* type = "text/plain") {
  benchClient.print(status == 200 ? "HTTP/1.1 200 OK\r\n" : "HTTP/1.1 409 Conflict\r\n");
  benchClient.print("Connection: close\r\nCache-Control: no-store\r\nContent-Type: ");
  benchClient.print(type); benchClient.print("; charset=utf-8\r\nContent-Length: ");
  benchClient.print(strlen(body)); benchClient.print("\r\n\r\n");
  benchClient.print(body);
  benchClient.stop();
}

void handleBenchRequest() {
  if (requestLine == "GET / HTTP/1.1" || requestLine == "GET /control_panel.html HTTP/1.1") {
    // Large page transfers only occur at neutral, never during a test.
    setNeutral(); revokeRemote();
    replyHttp(200, kControlPanelHtml, "text/html"); return;
  }
  if (requestLine == "POST /claim HTTP/1.1") {
    if (remoteToken || testActive || commandFault || !outputsReady) {
      replyHttp(409, "Control busy or PWM fault. Stop the test first."); return;
    }
    setNeutral(); receivingThrottle = false;
    remoteToken = ++tokenCounter;
    if (!remoteToken) remoteToken = ++tokenCounter;
    remoteSequence = 0; remoteSeen = millis(); remoteMotion = true;
    benchLog.response = "TOKEN:" + String(remoteToken) + "\nCAR:" + String(carNumber) + " MAC:" + boardMac + "\n";
    benchLog.capture = true; reportConfiguration(); benchLog.capture = false;
    replyHttp(200, benchLog.response.c_str()); return;
  }
  // Only one complete, bounded command per POST. No CORS is enabled.
  unsigned long token, sequence; char cmd[32] = {}; int consumed = -1;
  if (sscanf(requestLine.c_str(), "POST /cmd?token=%lu&seq=%lu&c=%31[a-z0-9,?+-] HTTP/1.1%n",
             &token, &sequence, cmd, &consumed) != 3 || consumed != int(requestLine.length()) ||
      !remoteToken || token != remoteToken || sequence <= remoteSequence) {
    replyHttp(409, "Invalid or expired session. Reconnect explicitly."); return;
  }
  remoteSequence = sequence; remoteSeen = millis();
  if (!strcmp(cmd, "q")) {
    setNeutral(); revokeRemote(); replyHttp(200, "Disconnected at neutral."); return;
  }
  bool valid = !strcmp(cmd, "n") || !strcmp(cmd, "x") || !strcmp(cmd, "?");
  if (strchr("tsceg", cmd[0]) && cmd[1]) {
    valid = true;
    for (size_t i = 1; cmd[i]; ++i) {
      if (!isdigit(cmd[i]) && !(cmd[i] == '-' && i == 1) && !(cmd[i] == ',' && cmd[0] == 'g')) valid = false;
    }
  }
  if (!valid) { setNeutral(); revokeRemote(); replyHttp(409, "Invalid command; neutral requested."); return; }
  benchLog.response = ""; benchLog.capture = true;
  receivingThrottle = false; remoteMotion = true;
  if (!strcmp(cmd, "?")) reportConfiguration();
  else {
    for (size_t i = 0; cmd[i]; ++i) processCommandByte(cmd[i]);
    processCommandByte('\n');
  }
  benchLog.capture = false;
  replyHttp(200, benchLog.response.c_str());
}

void pollBenchWiFi() {
  if (!wifiReady) return;
  if (remoteToken && uint32_t(millis() - remoteSeen) > 3000) { setNeutral(); revokeRemote(); }
  if (!benchClient) {
    benchClient = benchServer.available();
    if (!benchClient) return;
    requestStarted = millis(); requestBytes = 0; requestLine = ""; readingHeaders = false;
  }
  if (uint32_t(millis() - requestStarted) > 250) { benchClient.stop(); return; }
  // Bounded work: the main loop can continue checking the motion deadline.
  for (int budget = 0; budget < 128 && benchClient.available(); ++budget) {
    char c = char(benchClient.read());
    if (++requestBytes > 4096) { benchClient.stop(); return; }
    if (!readingHeaders) {
      if (c == '\n') { readingHeaders = true; handleBenchRequest(); return; }
      if (c != '\r') requestLine += c;
      if (requestLine.length() > 160) { benchClient.stop(); return; }
    }
  }
}
