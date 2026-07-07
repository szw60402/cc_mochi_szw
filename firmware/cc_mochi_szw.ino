/*
 * ╔══════════════════════════════════════════════════════════════╗
 * CLAWD MOCHI — ESP32-C3 Super Mini + ST7789 1.54" 240×240
 *
 * Wiring:
 * SDA → GPIO 10  (hardware SPI MOSI)
 * SCL → GPIO 8   (hardware SPI SCK)
 * RST → GPIO 2
 * DC  → GPIO 1
 * CS  → GPIO 4
 * BL  → GPIO 3
 * VCC → 3V3
 * GND → GND
 *
 * WiFi: "ClaWD-Mochi"  pw: clawd1234  → http://192.168.4.1
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <NimBLEDevice.h>
#include <esp_system.h>  // reset reason detection

RTC_DATA_ATTR bool hasBooted = false;
void executeCmd(char cmd);
class ClawdBLECb:public NimBLECharacteristicCallbacks{void onWrite(NimBLECharacteristic*p){std::string v=p->getValue();if(v.length()>0)executeCmd(v[0]);}};
#include <time.h>

#define TFT_CS  4
#define TFT_DC  1
#define TFT_RST 2
#define TFT_BLK 3

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ⚠️ Replace with your own WiFi credentials before flashing!
// const char* WIFI_SSID = "YOUR_SSID";
// const char* WIFI_PASS = "YOUR_PASSWORD";
WebServer server(80);

#define DISP_W 240
#define DISP_H 240

#define EYE_W   30
#define EYE_H   60
#define EYE_GAP 120
#define EYE_OX  0     // horizontal offset
#define EYE_OY  40    // vertical offset upward (subtracted from centre)

uint16_t C_ORANGE, C_DARKBG, C_MUTED, C_GREEN;

#define C_WHITE ST77XX_WHITE
#define C_BLACK ST77XX_BLACK

#define VIEW_EYES_NORMAL  0
#define VIEW_EYES_SQUISH  1
#define VIEW_CODE         2
#define VIEW_DRAW         3
#define VIEW_EYES_ANGRY   4
#define VIEW_EYES_SAD     5
#define VIEW_EYES_TIRED   6
#define VIEW_EYES_SLEEP   7
#define VIEW_EYES_THINK   8
#define VIEW_EYES_HAPPY   9
#define VIEW_EYES_ANNOYED 10
#define VIEW_EYES_KISS    11
#define VIEW_EYES_WINK    12
#define VIEW_EYES_BUBBLE  13
#define VIEW_EYES_BORED    14
#define VIEW_EYES_CONFUSED 15
#define VIEW_EYES_DIZZY    16
#define VIEW_EYES_DEAD     17
#define VIEW_EYES_LOOKAROUND 18
#define VIEW_EYES_WRITING 19
#define VIEW_EYES_EDITING 20
#define VIEW_EYES_EXECUTING 21
#define VIEW_EYES_ANALYZING 22
#define VIEW_EYES_CODING 23
#define VIEW_EYES_SURPRISE 24
#define VIEW_EYES_SMUG 25
#define VIEW_EYES_PANIC 26
#define VIEW_EYES_PROUD 27
#define VIEW_EYES_FABLE5 28

uint8_t  currentView  = VIEW_EYES_NORMAL;
volatile uint8_t pendingView  = 255;
bool     busy         = false;
bool     backlightOn  = true;
uint8_t  animSpeed    = 1;   // 1=slow(default) 2=normal 3=fast

uint32_t lastExprMs    = 0;
uint32_t nextExprMs    = 10000;  // first switch after 10s
bool          manualOverride = false;
uint32_t overrideEnd   = 0;
int8_t        lastTimeHour  = -1;     // track hour changes
uint8_t       exprPlayCount = 0;      // how many times current expr has played
#define       EXPR_REPEAT   10        // play same expr this many times before switching

uint16_t animBgColor  = 0;
uint16_t drawBgColor  = 0;   // background for canvas

#define TERM_COLS      15
#define TERM_ROWS       8
#define TERM_CHAR_W    12
#define TERM_CHAR_H    20
#define TERM_PAD_X      8
#define TERM_PAD_Y     18

bool    termMode    = false;
String  termLines[TERM_ROWS];
uint8_t termRow     = 0;
uint8_t termCol     = 0;

#define LOGO_CX 120
#define LOGO_CY 105

#define LOGO_TRI_COUNT 162
static const int16_t LOGO_TRIS[][6] PROGMEM = {
  {120,105,65,134,100,114},{120,105,100,114,101,113},{120,105,101,113,100,112},
  {120,105,100,112,99,112},{120,105,99,112,93,111},{120,105,93,111,73,111},
  {120,105,73,111,55,110},{120,105,55,110,38,109},{120,105,38,109,34,108},
  {120,105,34,108,30,103},{120,105,30,103,30,100},{120,105,30,100,34,98},
  {120,105,34,98,39,98},{120,105,39,98,50,99},{120,105,50,99,67,100},
  {120,105,67,100,80,101},{120,105,80,101,98,103},{120,105,98,103,101,103},
  {120,105,101,103,101,102},{120,105,101,102,100,101},{120,105,100,101,100,100},
  {120,105,100,100,82,88},{120,105,82,88,63,76},{120,105,63,76,53,69},
  {120,105,53,69,48,65},{120,105,48,65,45,61},{120,105,45,61,44,54},
  {120,105,44,54,49,49},{120,105,49,49,55,49},{120,105,55,49,57,49},
  {120,105,57,49,64,55},{120,105,64,55,78,66},{120,105,78,66,96,79},
  {120,105,96,79,99,81},{120,105,99,81,100,81},{120,105,100,81,100,80},
  {120,105,100,80,99,78},{120,105,99,78,89,60},{120,105,89,60,78,41},
  {120,105,78,41,73,34},{120,105,73,34,72,29},{120,105,72,29,72,28},
  {120,105,72,28,72,27},{120,105,72,27,71,26},{120,105,71,26,71,25},
  {120,105,71,25,71,24},{120,105,71,24,77,16},{120,105,77,16,80,15},
  {120,105,80,15,87,16},{120,105,87,16,91,19},{120,105,91,19,95,29},
  {120,105,95,29,103,46},{120,105,103,46,114,68},{120,105,114,68,118,75},
  {120,105,118,75,119,81},{120,105,119,81,120,83},{120,105,120,83,121,83},
  {120,105,121,83,121,82},{120,105,121,82,122,69},{120,105,122,69,124,54},
  {120,105,124,54,126,34},{120,105,126,34,126,28},{120,105,126,28,129,21},
  {120,105,129,21,135,18},{120,105,135,18,139,20},{120,105,139,20,143,25},
  {120,105,143,25,142,28},{120,105,142,28,140,42},{120,105,140,42,136,64},
  {120,105,136,64,133,78},{120,105,133,78,135,78},{120,105,135,78,136,76},
  {120,105,136,76,144,67},{120,105,144,67,156,51},{120,105,156,51,162,45},
  {120,105,162,45,168,38},{120,105,168,38,172,35},{120,105,172,35,180,35},
  {120,105,180,35,185,43},{120,105,185,43,183,52},{120,105,183,52,175,62},
  {120,105,175,62,168,71},{120,105,168,71,159,83},{120,105,159,83,153,94},
  {120,105,153,94,154,94},{120,105,154,94,155,94},{120,105,155,94,176,90},
  {120,105,176,90,188,88},{120,105,188,88,201,85},{120,105,201,85,208,88},
  {120,105,208,88,208,91},{120,105,208,91,206,97},{120,105,206,97,191,101},
  {120,105,191,101,174,104},{120,105,174,104,148,110},{120,105,148,110,148,111},
  {120,105,148,111,148,111},{120,105,148,111,160,112},{120,105,160,112,165,112},
  {120,105,165,112,177,112},{120,105,177,112,200,114},{120,105,200,114,205,118},
  {120,105,205,118,209,123},{120,105,209,123,208,126},{120,105,208,126,199,131},
  {120,105,199,131,187,128},{120,105,187,128,159,121},{120,105,159,121,149,119},
  {120,105,149,119,147,119},{120,105,147,119,147,120},{120,105,147,120,156,128},
  {120,105,156,128,170,141},{120,105,170,141,189,158},{120,105,189,158,190,163},
  {120,105,190,163,188,166},{120,105,188,166,185,166},{120,105,185,166,169,153},
  {120,105,169,153,162,148},{120,105,162,148,148,136},{120,105,148,136,147,136},
  {120,105,147,136,147,137},{120,105,147,137,150,142},{120,105,150,142,168,168},
  {120,105,168,168,169,176},{120,105,169,176,168,179},{120,105,168,179,163,180},
  {120,105,163,180,158,179},{120,105,158,179,148,165},{120,105,148,165,137,149},
  {120,105,137,149,129,134},{120,105,129,134,128,135},{120,105,128,135,123,189},
  {120,105,123,189,120,192},{120,105,120,192,115,194},{120,105,115,194,110,191},
  {120,105,110,191,108,185},{120,105,108,185,110,174},{120,105,110,174,113,160},
  {120,105,113,160,116,148},{120,105,116,148,118,134},{120,105,118,134,119,129},
  {120,105,119,129,119,129},{120,105,119,129,118,129},{120,105,118,129,107,144},
  {120,105,107,144,91,166},{120,105,91,166,78,180},{120,105,78,180,75,181},
  {120,105,75,181,70,178},{120,105,70,178,70,173},{120,105,70,173,73,169},
  {120,105,73,169,91,146},{120,105,91,146,102,132},{120,105,102,132,109,124},
  {120,105,109,124,109,123},{120,105,109,123,108,123},{120,105,108,123,61,153},
  {120,105,61,153,52,155},{120,105,52,155,49,151},{120,105,49,151,49,146},
  {120,105,49,146,51,144},{120,105,51,144,65,134},{120,105,65,134,65,134},
};
#define LOGO_SEG_COUNT 162
static const int16_t LOGO_SEGS[][4] PROGMEM = {
  {65,134,100,114},{100,114,101,113},{101,113,100,112},{100,112,99,112},
  {99,112,93,111},{93,111,73,111},{73,111,55,110},{55,110,38,109},
  {38,109,34,108},{34,108,30,103},{30,103,30,100},{30,100,34,98},
  {34,98,39,98},{39,98,50,99},{50,99,67,100},{67,100,80,101},
  {80,101,98,103},{98,103,101,103},{101,103,101,102},{101,102,100,101},
  {100,101,100,100},{100,100,82,88},{82,88,63,76},{63,76,53,69},
  {53,69,48,65},{48,65,45,61},{45,61,44,54},{44,54,49,49},
  {49,49,55,49},{55,49,57,49},{57,49,64,55},{64,55,78,66},
  {78,66,96,79},{96,79,99,81},{99,81,100,81},{100,81,100,80},
  {100,80,99,78},{99,78,89,60},{89,60,78,41},{78,41,73,34},
  {73,34,72,29},{72,29,72,28},{72,28,72,27},{72,27,71,26},
  {71,26,71,25},{71,25,71,24},{71,24,77,16},{77,16,80,15},
  {80,15,87,16},{87,16,91,19},{91,19,95,29},{95,29,103,46},
  {103,46,114,68},{114,68,118,75},{118,75,119,81},{119,81,120,83},
  {120,83,121,83},{121,83,121,82},{121,82,122,69},{122,69,124,54},
  {124,54,126,34},{126,34,126,28},{126,28,129,21},{129,21,135,18},
  {135,18,139,20},{139,20,143,25},{143,25,142,28},{142,28,140,42},
  {140,42,136,64},{136,64,133,78},{133,78,135,78},{135,78,136,76},
  {136,76,144,67},{144,67,156,51},{156,51,162,45},{162,45,168,38},
  {168,38,172,35},{172,35,180,35},{180,35,185,43},{185,43,183,52},
  {183,52,175,62},{175,62,168,71},{168,71,159,83},{159,83,153,94},
  {153,94,154,94},{154,94,155,94},{155,94,176,90},{176,90,188,88},
  {188,88,201,85},{201,85,208,88},{208,88,208,91},{208,91,206,97},
  {206,97,191,101},{191,101,174,104},{174,104,148,110},{148,110,148,111},
  {148,111,148,111},{148,111,160,112},{160,112,165,112},{165,112,177,112},
  {177,112,200,114},{200,114,205,118},{205,118,209,123},{209,123,208,126},
  {208,126,199,131},{199,131,187,128},{187,128,159,121},{159,121,149,119},
  {149,119,147,119},{147,119,147,120},{147,120,156,128},{156,128,170,141},
  {170,141,189,158},{189,158,190,163},{190,163,188,166},{188,166,185,166},
  {185,166,169,153},{169,153,162,148},{162,148,148,136},{148,136,147,136},
  {147,136,147,137},{147,137,150,142},{150,142,168,168},{168,168,169,176},
  {169,176,168,179},{168,179,163,180},{163,180,158,179},{158,179,148,165},
  {148,165,137,149},{137,149,129,134},{129,134,128,135},{128,135,123,189},
  {123,189,120,192},{120,192,115,194},{115,194,110,191},{110,191,108,185},
  {108,185,110,174},{110,174,113,160},{113,160,116,148},{116,148,118,134},
  {118,134,119,129},{119,129,119,129},{119,129,118,129},{118,129,107,144},
  {107,144,91,166},{91,166,78,180},{78,180,75,181},{75,181,70,178},
  {70,178,70,173},{70,173,73,169},{73,169,91,146},{91,146,102,132},
  {102,132,109,124},{109,124,109,123},{109,123,108,123},{108,123,61,153},
  {61,153,52,155},{52,155,49,151},{49,151,49,146},{49,146,51,144},
  {51,144,65,134},{65,134,65,134},
};

//  HELPERS

int speedMs(int ms) {
  if (animSpeed == 3) return ms / 2;
  if (animSpeed == 1) return ms * 2;
  return ms;
}

uint16_t hexToRgb565(String hex) {
  hex.replace("#", "");
  if (hex.length() != 6) return C_WHITE;
  long v = strtol(hex.c_str(), nullptr, 16);
  return tft.color565((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

void setBacklight(bool on) {
  backlightOn = on;
  digitalWrite(TFT_BLK, on ? HIGH : LOW);
}

void initColours() {
  C_ORANGE = tft.color565(218, 17, 0);
  C_DARKBG = tft.color565(10,  12,  16);
  C_MUTED  = tft.color565(90,  88,  86);
  C_GREEN  = tft.color565(80, 220, 130);
  animBgColor = C_ORANGE;
  drawBgColor = C_ORANGE;
}

//  LOGO

void drawLogoFilled(uint16_t bg, uint16_t fg) {
  tft.fillScreen(bg);
  for (uint16_t i = 0; i < LOGO_TRI_COUNT; i++) {
    tft.fillTriangle(
      pgm_read_word(&LOGO_TRIS[i][0]), pgm_read_word(&LOGO_TRIS[i][1]),
      pgm_read_word(&LOGO_TRIS[i][2]), pgm_read_word(&LOGO_TRIS[i][3]),
      pgm_read_word(&LOGO_TRIS[i][4]), pgm_read_word(&LOGO_TRIS[i][5]),
      fg);
  }
  tft.setTextColor(fg); tft.setTextSize(2);
  tft.setCursor(LOGO_CX - 54, 210); tft.print("Anthropic");
  tft.setCursor(LOGO_CX - 53, 210); tft.print("Anthropic");
}

//  VIEWS

// Eye helpers — shared constants via #define EYE_*
inline int16_t eyeLX(int16_t ox) {
  return (DISP_W - (EYE_W * 2 + EYE_GAP)) / 2 + EYE_OX + ox;
}
inline int16_t eyeRX(int16_t ox) { return eyeLX(ox) + EYE_W + EYE_GAP; }
inline int16_t eyeY()            { return (DISP_H - EYE_H) / 2 - EYE_OY; }
inline int16_t eyeCY()           { return eyeY() + EYE_H / 2; }

void drawNormalEyes(int16_t ox = 0, bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
  }
}

void drawChevron(int16_t cx, int16_t cy, int16_t arm, int16_t reach,
                 uint8_t thk, bool rightFacing, uint16_t col) {
  for (int8_t t = -(int8_t)thk; t <= (int8_t)thk; t++) {
    if (rightFacing) {
      tft.drawLine(cx - reach/2, cy - arm + t, cx + reach/2, cy + t,      col);
      tft.drawLine(cx + reach/2, cy + t,       cx - reach/2, cy + arm + t, col);
    } else {
      tft.drawLine(cx + reach/2, cy - arm + t, cx - reach/2, cy + t,      col);
      tft.drawLine(cx - reach/2, cy + t,       cx + reach/2, cy + arm + t, col);
    }
  }
}

void drawSquishEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), cy = eyeCY();
  const int16_t arm = EYE_H / 3, reach = EYE_W;
  drawChevron(lx + EYE_W/2, cy, arm, reach, 10, true,  C_BLACK);
  drawChevron(rx + EYE_W/2, cy, arm, reach, 10, false, C_BLACK);
}

void drawCodeView() {
  termMode = false;
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0,          DISP_W, 4, C_ORANGE);
  tft.fillRect(0, DISP_H - 4, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_ORANGE); tft.setTextSize(4);
  tft.setCursor((DISP_W - 144) / 2, DISP_H / 2 - 52); tft.print("Claude");
  tft.setTextColor(C_WHITE);  tft.setTextSize(4);
  tft.setCursor((DISP_W - 96) / 2,  DISP_H / 2 + 8);  tft.print("Code");
  tft.fillRect((DISP_W - 96) / 2, DISP_H / 2 + 52, 96, 3, C_ORANGE);
}

//  ANIMATIONS

// Delay that can be interrupted by pending commands
bool animDelay(int ms) {
  ms = speedMs(ms);
  uint32_t start = millis();
  while (millis() - start < (uint32_t)ms) {
    server.handleClient();
    if (pendingView != 255) { busy = false; return true; }
    delay(1);
  }
  return false;
}

void animNormalEyes() {
  busy = true;
  const int16_t offs[] = {-16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 5; i++) { drawNormalEyes(offs[i]); if(animDelay(80)){busy=false;return;} }
  drawNormalEyes(0, true);  animDelay(100);
  drawNormalEyes(0, false); if(animDelay(70)){busy=false;return;}
  drawNormalEyes(0, true);  animDelay(70);
  drawNormalEyes(0, false);
  busy = false;
}

void animSquishEyes() {
  busy = true;
  const int16_t offs[] = {0, -20, 20, -16, 16, -10, 10, -5, 5, 0};
  for (uint8_t i = 0; i < 10; i++) { drawSquishEyes(offs[i]); if(animDelay(90)){busy=false;return;} }
  drawSquishEyes(0);
  busy = false;
}

void animLookaroundEyes() {
  busy = true;
  // Left-right look around
  const int16_t offs[] = {0, -16, -16, 16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 8; i++) { drawNormalEyes(offs[i]); if(animDelay(120)){busy=false;return;} }
  drawNormalEyes(0);
  busy = false;
}

void draw生气Eyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey + 12, EYE_W, EYE_H - 12, C_BLACK);
  tft.fillRect(rx, ey + 12, EYE_W, EYE_H - 12, C_BLACK);
  for (int8_t t = -4; t <= 4; t++) {
    tft.drawLine(lx - 6, ey + t,      lx + EYE_W + 2, ey + 14 + t, C_BLACK);
    tft.drawLine(rx - 2, ey + 14 + t, rx + EYE_W + 6, ey + t,      C_BLACK);
  }
}
void anim生气Eyes() {
  busy = true;
  const int16_t shk[] = {-10, 10, -8, 8, -5, 5, -3, 3, 0};
  for (uint8_t i = 0; i < 9; i++) { draw生气Eyes(shk[i]); if(animDelay(55)){busy=false;return;} }
  draw生气Eyes(0);
  busy = false;
}

void draw悲伤Eyes(uint8_t tearY = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  tft.fillRect(lx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  tft.fillRect(rx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  for (int8_t t = -3; t <= 3; t++) {
    tft.drawLine(lx,          ey + 10 + t, lx + EYE_W, ey + t,      C_BLACK);
    tft.drawLine(rx,          ey + t,      rx + EYE_W, ey + 10 + t, C_BLACK);
  }
  if (tearY > 0) {
    uint16_t tc = tft.color565(80, 160, 255);
    tft.fillRect(lx + EYE_W / 2 - 2, ey + EYE_H + tearY, 4, 10, tc);
    tft.fillRect(rx + EYE_W / 2 - 2, ey + EYE_H + tearY, 4, 10, tc);
  }
}
void anim悲伤Eyes() {
  busy = true;
  draw悲伤Eyes(0); if(animDelay(400)){busy=false;return;}
  for (uint8_t t = 0; t <= 24; t += 4) { draw悲伤Eyes(t); if(animDelay(80)){busy=false;return;} }
  draw悲伤Eyes(24); if(animDelay(500)){busy=false;return;}
  draw悲伤Eyes(0);
  busy = false;
}

void draw疲惫Eyes(uint8_t droop = 30) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  uint8_t h = (droop >= EYE_H - 4) ? 4 : (EYE_H - droop);
  tft.fillRect(lx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  tft.fillRect(rx, ey + EYE_H - h, EYE_W, h, C_BLACK);
}
void anim疲惫Eyes() {
  busy = true;
  for (uint8_t d = 0; d <= 44; d += 4) { draw疲惫Eyes(d); if (animDelay(70)) { busy = false; return; } }
  if (animDelay(600)) { busy = false; return; }
  for (uint8_t d = 44; d > 30; d -= 4) { draw疲惫Eyes(d); if (animDelay(70)) { busy = false; return; } }
  draw疲惫Eyes(30);
  busy = false;
}

void draw睡觉Eyes(uint8_t zzzCount = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), cy = eyeCY();
  // Thinner lines, same position as tired last frame
  const uint8_t slpH = EYE_H / 3;
  tft.fillRect(lx, cy, EYE_W, slpH, C_BLACK);
  tft.fillRect(rx, cy, EYE_W, slpH, C_BLACK);
  // z's float upward from eye level
  tft.setTextColor(C_BLACK);
  if (zzzCount >= 1) { tft.setTextSize(1); tft.setCursor(rx + EYE_W + 4,  cy - 6);  tft.print("z"); }
  if (zzzCount >= 2) { tft.setTextSize(2); tft.setCursor(rx + EYE_W + 10, cy - 22); tft.print("z"); }
  if (zzzCount >= 3) { tft.setTextSize(3); tft.setCursor(rx + EYE_W + 18, cy - 42); tft.print("Z"); }
}
void anim睡觉Eyes() {
  busy = true;
  draw睡觉Eyes(0); if (animDelay(400))  { busy = false; return; }
  draw睡觉Eyes(1); if (animDelay(600))  { busy = false; return; }
  draw睡觉Eyes(2); if (animDelay(600))  { busy = false; return; }
  draw睡觉Eyes(3); if (animDelay(900))  { busy = false; return; }
  draw睡觉Eyes(1); if (animDelay(300))  { busy = false; return; }
  draw睡觉Eyes(0);
  busy = false;
}

void draw思考Eyes(int16_t ox = 0, uint8_t bubble = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  const int16_t cy = eyeCY();
  // Both eyes look upward (rect shifted up, shorter)
  tft.fillRect(lx, ey, EYE_W, EYE_H - 10, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H - 10, C_BLACK);
  // Thought cloud: dots getting bigger as they go up-right
  if (bubble >= 1) tft.fillCircle(rx + EYE_W + 6,  ey + 20,  3, C_BLACK);
  if (bubble >= 2) tft.fillCircle(rx + EYE_W + 14, ey + 4,   5, C_BLACK);
  if (bubble >= 3) {
    // Big thought bubble (single circle)
    int16_t cx = rx + EYE_W + 22, ccy = ey - 14;
    tft.fillCircle(cx, ccy, 10, C_BLACK);
  }
}
void anim思考Eyes() {
  busy = true;
  draw思考Eyes(0, 0); if(animDelay(400)){busy=false;return;}
  // Bubbles appear one by one
  draw思考Eyes(0, 1); if(animDelay(250)){busy=false;return;}
  draw思考Eyes(0, 2); if(animDelay(250)){busy=false;return;}
  draw思考Eyes(0, 3); if(animDelay(600)){busy=false;return;}
  // Eyes drift while thinking
  for (int8_t o = 0; o <= 16; o += 4) { draw思考Eyes(o, 3); if(animDelay(80)){busy=false;return;} }
  draw思考Eyes(16, 3); if(animDelay(400)){busy=false;return;}
  for (int8_t o = 16; o >= -16; o -= 4) { draw思考Eyes(o, 3); if(animDelay(80)){busy=false;return;} }
  draw思考Eyes(-16, 3); if(animDelay(400)){busy=false;return;}
  // Look back center, bubbles fade
  for (int8_t o = -16; o <= 0; o += 4) { draw思考Eyes(o, 3); if(animDelay(80)){busy=false;return;} }
  draw思考Eyes(0, 3); if(animDelay(200)){busy=false;return;}
  draw思考Eyes(0, 2); if(animDelay(150)){busy=false;return;}
  draw思考Eyes(0, 1); if(animDelay(150)){busy=false;return;}
  draw思考Eyes(0, 0);
  busy = false;
}

// 垂直于线段方向的等宽描边
void solidSeg(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thk) {
  float dx = x2-x1, dy = y2-y1;
  float len = sqrtf(dx*dx + dy*dy);
  if (len < 1.0f) return;
  int16_t px = (int16_t)roundf(-dy/len * thk);
  int16_t py = (int16_t)roundf( dx/len * thk);
  tft.fillTriangle(x1+px, y1+py, x1-px, y1-py, x2+px, y2+py, C_BLACK);
  tft.fillTriangle(x1-px, y1-py, x2-px, y2-py, x2+px, y2+py, C_BLACK);
}

void draw开心Eyes(int16_t oy = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lcx = eyeLX(0) + EYE_W/2;
  const int16_t rcx = eyeRX(0) + EYE_W/2;
  const int16_t cy  = eyeCY() + oy;
  const int16_t hw  = EYE_W/2 + 2;
  const int16_t arm = EYE_H/3;
  solidSeg(lcx-hw, cy+arm, lcx,    cy-arm, 5);
  solidSeg(lcx,    cy-arm, lcx+hw, cy+arm, 5);
  solidSeg(rcx-hw, cy+arm, rcx,    cy-arm, 5);
  solidSeg(rcx,    cy-arm, rcx+hw, cy+arm, 5);
}

void anim开心Eyes() {
  busy = true;
  const int16_t b[] = {0, -10, 0, -8, 0, -6, 0, -4, 0};
  for (uint8_t i = 0; i < 9; i++) { draw开心Eyes(b[i]); if(animDelay(75)){busy=false;return;} }
  draw开心Eyes(0);
  busy = false;
}

void draw烦躁Eyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  for (int8_t t = -3; t <= 3; t++)
    tft.drawLine(rx - 2, ey + 10 + t, rx + EYE_W + 2, ey + t, C_BLACK);
}
void anim烦躁Eyes() {
  busy = true;
  draw烦躁Eyes(0); if(animDelay(300)){busy=false;return;}
  for (int8_t ox = 0; ox <= 12; ox += 3) { draw烦躁Eyes(ox); if(animDelay(80)){busy=false;return;} }
  for (int8_t ox = 12; ox >= -12; ox -= 3) { draw烦躁Eyes(ox); if(animDelay(80)){busy=false;return;} }
  for (int8_t ox = -12; ox <= 0; ox += 3) { draw烦躁Eyes(ox); if(animDelay(80)){busy=false;return;} }
  draw烦躁Eyes(0);
  busy = false;
}

// Draw a ♥ heart - sharper and less round
void drawHeart(int16_t cx, int16_t cy, uint8_t sz, uint16_t col) {
  // Two smaller circles with wider gap for deep indent
  int16_t r  = sz * 4 / 7;           // smaller bump radius (less round)
  int16_t bx = sz * 3 / 7;           // wider horizontal offset for deeper indent
  tft.fillCircle(cx - bx, cy - r/2, r, col);
  tft.fillCircle(cx + bx, cy - r/2, r, col);
  // Sharp triangle pointing down
  tft.fillTriangle(cx - sz, cy, cx + sz, cy, cx, cy + sz + r, col);
}

void draw亲亲Eyes(uint8_t phase = 0, uint8_t hs = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  const int16_t cy = eyeCY();
  // Left eye: normal rect |
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  // Right eye: < chevron (same as squish)
  drawChevron(rx + EYE_W/2, cy, EYE_H / 3, EYE_W, 10, false, C_BLACK);
  // Heart below eyes
  if (hs > 0) {
    uint16_t pk = tft.color565(255, 80, 140);
    int16_t hx = DISP_W / 2, hy = cy + EYE_H / 2 + 20;
    if (phase <= 1) {
      // Normal heart (phase 0) or pulsing bigger (phase 1)
      uint8_t sz = hs + phase * 2;
      drawHeart(hx, hy, sz, pk);
    } else if (phase == 2) {
      // Burst: expanding ring + scattered particles
      drawHeart(hx, hy, hs, pk);
      // White flash ring
      tft.drawCircle(hx, hy, hs + 4, C_WHITE);
      tft.drawCircle(hx, hy, hs + 6, C_WHITE);
    } else if (phase >= 3) {
      // Exploding particles flying outward
      uint16_t gl = tft.color565(255, 180, 200);
      uint8_t spread = (phase - 2) * 8;
      // Scattered heart fragments (small circles)
      tft.fillCircle(hx - spread, hy - spread/2, 3, pk);
      tft.fillCircle(hx + spread, hy - spread/2, 3, pk);
      tft.fillCircle(hx - spread/2, hy - spread, 2, pk);
      tft.fillCircle(hx + spread/2, hy - spread, 2, pk);
      tft.fillCircle(hx - spread, hy + spread/3, 2, gl);
      tft.fillCircle(hx + spread, hy + spread/3, 2, gl);
      tft.fillCircle(hx, hy + spread, 2, gl);
      // Sparkle dots
      tft.fillCircle(hx - spread - 4, hy - 6, 1, C_WHITE);
      tft.fillCircle(hx + spread + 4, hy - 4, 1, C_WHITE);
      tft.fillCircle(hx - 2, hy - spread - 3, 1, C_WHITE);
    }
  }
}
void anim亲亲Eyes() {
  busy = true;
  // Eyes appear
  draw亲亲Eyes(0, 0); if(animDelay(300)){busy=false;return;}
  // Heart grows
  for (uint8_t s = 3; s <= 12; s += 2) { draw亲亲Eyes(0, s); if(animDelay(70)){busy=false;return;} }
  draw亲亲Eyes(0, 12); if(animDelay(600)){busy=false;return;}
  // Heart pulses bigger
  draw亲亲Eyes(1, 12); if(animDelay(200)){busy=false;return;}
  // Burst flash
  draw亲亲Eyes(2, 12); if(animDelay(100)){busy=false;return;}
  // Particles explode outward
  draw亲亲Eyes(3, 12); if(animDelay(80)){busy=false;return;}
  draw亲亲Eyes(4, 12); if(animDelay(80)){busy=false;return;}
  draw亲亲Eyes(5, 12); if(animDelay(120)){busy=false;return;}
  // Clear
  draw亲亲Eyes(0, 0); if(animDelay(400)){busy=false;return;}
  busy = false;
}

void drawWinkEyes(bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  const int16_t cy = eyeCY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, cy - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, cy - 3, EYE_W, 6, C_BLACK);
  }
}
void animWinkEyes() {
  busy = true;
  drawWinkEyes(false); if(animDelay(600)){busy=false;return;}
  drawWinkEyes(true);  animDelay(120);
  drawWinkEyes(false); if(animDelay(500)){busy=false;return;}
  drawWinkEyes(true);  animDelay(120);
  drawWinkEyes(false); if(animDelay(400)){busy=false;return;}
  drawWinkEyes(true);  animDelay(120);
  drawWinkEyes(false);
  busy = false;
}

void drawBubbleEyes(uint8_t phase = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  uint16_t bc = tft.color565(120, 190, 255);
  int16_t base = ey + EYE_H + 8;
  int16_t bxs[] = { (int16_t)(lx + EYE_W/2 - 8), (int16_t)(lx + EYE_W/2 + 8),
                    (int16_t)(rx + EYE_W/2 - 8), (int16_t)(rx + EYE_W/2 + 8) };
  uint8_t rs[] = {5, 4, 6, 4};
  for (uint8_t i = 0; i < 4; i++) {
    int16_t by = base - ((phase + i * 18) % 90);
    if (by > ey - 12 && by < base + 10)
      tft.drawCircle(bxs[i], by, rs[i], bc);
  }
}
void animBubbleEyes() {
  busy = true;
  for (uint8_t p = 0; p < 72; p += 3) { drawBubbleEyes(p); if(animDelay(55)){busy=false;return;} }
  drawBubbleEyes(0);
  busy = false;
}

void draw无聊Eyes(int16_t ox = 0, int16_t prevOx = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  uint8_t h = EYE_H * 2 / 5;
  tft.fillRect(lx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  tft.fillRect(rx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  // 根据移动方向决定倾斜：向左移动→\形，向右移动→/形
  int16_t delta = ox - prevOx;
  int16_t ly = (delta < 0) ? -4 : 0;
  int16_t ry = (delta < 0) ?  0 : -4;
  for (int8_t t = -2; t <= 2; t++) {
    tft.drawLine(lx, ey+EYE_H-h+ly+t, lx+EYE_W, ey+EYE_H-h+ry+t, C_BLACK);
    tft.drawLine(rx, ey+EYE_H-h+ly+t, rx+EYE_W, ey+EYE_H-h+ry+t, C_BLACK);
  }
}
void anim无聊Eyes() {
  busy = true;
  int16_t prev = 0;
  draw无聊Eyes(0, 0); if(animDelay(500)){busy=false;return;}
  for (int8_t ox = 0; ox <= 24; ox += 4) { prev = ox; draw无聊Eyes(ox, ox-4); if(animDelay(100)){busy=false;return;} }
  draw无聊Eyes(24, 24); if(animDelay(800)){busy=false;return;}
  prev = 24;
  for (int8_t ox = 24; ox >= -24; ox -= 4) { int16_t p = prev; prev = ox; draw无聊Eyes(ox, p); if(animDelay(100)){busy=false;return;} }
  draw无聊Eyes(-24, -24); if(animDelay(800)){busy=false;return;}
  prev = -24;
  for (int8_t ox = -24; ox <= 0; ox += 4) { int16_t p = prev; prev = ox; draw无聊Eyes(ox, p); if(animDelay(100)){busy=false;return;} }
  draw无聊Eyes(0, 0);
  busy = false;
}

void drawConfusedEyes(int16_t ox = 0, bool showQ = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  if (showQ) {
    tft.setTextColor(C_BLACK); tft.setTextSize(3);
    tft.setCursor(eyeRX(ox) + EYE_W + 2, ey - 4);
    tft.print("?");
  }
}
void animConfusedEyes() {
  busy = true;
  drawConfusedEyes(0, true); if(animDelay(400)){busy=false;return;}
  for (int8_t o = 0; o <= 20; o += 4) { drawConfusedEyes(o, o >= 8); if(animDelay(70)){busy=false;return;} }
  drawConfusedEyes(20, true); if(animDelay(500)){busy=false;return;}
  for (int8_t o = 20; o >= -20; o -= 4) { drawConfusedEyes(o, o <= 0); if(animDelay(70)){busy=false;return;} }
  drawConfusedEyes(-20, true); if(animDelay(400)){busy=false;return;}
  for (int8_t o = -20; o <= 0; o += 4) { drawConfusedEyes(o, false); if(animDelay(70)){busy=false;return;} }
  drawConfusedEyes(0, false);
  busy = false;
}

void drawDizzyEyes(uint8_t phase = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lcx = eyeLX(0) + EYE_W / 2;
  const int16_t rcx = eyeRX(0) + EYE_W / 2;
  const int16_t cy  = eyeCY();
  const uint8_t maxR = EYE_W / 2 + 4;
  // Draw thick spiral for each eye using parametric spiral
  // Spiral: r = a + b*theta, offset by phase for animation
  float angleOffset = phase * 0.8f;
  for (float theta = 0; theta < 6.28f * 2.5f; theta += 0.15f) {
    float r = 2.0f + theta * 1.8f;
    if (r > maxR) break;
    float a = theta + angleOffset;
    int16_t px = (int16_t)(cosf(a) * r);
    int16_t py = (int16_t)(sinf(a) * r);
    // Draw 3px thick dots along spiral
    tft.fillCircle(lcx + px, cy + py, 2, C_BLACK);
    tft.fillCircle(rcx + px, cy + py, 2, C_BLACK);
  }
}
void animDizzyEyes() {
  busy = true;
  for (uint8_t p = 0; p < 24; p++) {
    drawDizzyEyes(p % 8);
    animDelay(90);
  }
  drawDizzyEyes(0);
  busy = false;
}

void drawDeadEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t cy  = eyeCY();
  const int16_t arm = EYE_W / 2;        // 15px — 刚好在眼睛范围内
  const int16_t lcx = eyeLX(ox) + EYE_W / 2;
  const int16_t rcx = eyeRX(ox) + EYE_W / 2;
  const int8_t  thk = 5;
  for (int8_t t = -thk; t <= thk; t++) {
    // \ 对角线，垂直偏移方向 (+t, -t)
    tft.drawLine(lcx-arm+t, cy-arm-t, lcx+arm+t, cy+arm-t, C_BLACK);
    tft.drawLine(rcx-arm+t, cy-arm-t, rcx+arm+t, cy+arm-t, C_BLACK);
    // / 对角线，垂直偏移方向 (+t, +t)
    tft.drawLine(lcx+arm+t, cy-arm+t, lcx-arm+t, cy+arm+t, C_BLACK);
    tft.drawLine(rcx+arm+t, cy-arm+t, rcx-arm+t, cy+arm+t, C_BLACK);
  }
}
void animDeadEyes() {
  busy = true;
  tft.fillScreen(C_WHITE); if(animDelay(70)){busy=false;return;}
  tft.fillScreen(animBgColor); if(animDelay(70)){busy=false;return;}
  tft.fillScreen(C_WHITE); if(animDelay(70)){busy=false;return;}
  drawDeadEyes(0); if(animDelay(400)){busy=false;return;}
  const int16_t shk[] = {-8, 8, -6, 6, -4, 4, -2, 2, 0};
  for (uint8_t i = 0; i < 9; i++) { drawDeadEyes(shk[i]); if(animDelay(50)){busy=false;return;} }
  drawDeadEyes(0);
  busy = false;
}



void draw书写(uint8_t phase) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  uint8_t sh=EYE_H/3,py=ey2+EYE_H+6;
  tft.fillRect(lx,eyeCY()-sh/2,EYE_W,sh,C_BLACK);
  tft.fillRect(rx,eyeCY()-sh/2,EYE_W,sh,C_BLACK);
  tft.drawFastHLine(lx,py,EYE_W,C_BLACK);
  tft.drawFastHLine(rx,py,EYE_W,C_BLACK);
  uint8_t w1=constrain(phase*3,0,EYE_W-12);
  uint8_t w2=constrain((phase-6)*3,0,EYE_W-8);
  uint8_t w3=constrain((phase-12)*3,0,EYE_W-4);
  if(w1>0)tft.fillRect(lx+4,py+6,w1,3,C_BLACK);
  if(w2>0)tft.fillRect(rx-8,py+14,w2,3,C_BLACK);
  if(w3>0)tft.fillRect(lx+8,py+22,w3,3,C_BLACK);
  if(w3>=EYE_W-12&&(phase%2==0))tft.fillRect(lx+8+w3,py+22,3,3,C_BLACK);
  tft.drawFastHLine(lx,py+30,EYE_W,C_BLACK);
  tft.drawFastHLine(rx,py+30,EYE_W,C_BLACK);
}
void anim书写() { busy=true;
  for(uint8_t p=0;p<=24;p++){draw书写(p);if(animDelay(100)){busy=false;return;}}
  if(animDelay(1000)){busy=false;return;}
  for(uint8_t p=24;p>0;p-=2){draw书写(p);if(animDelay(60)){busy=false;return;}}
  draw书写(0);busy=false;
}

void draw编辑(int16_t bx) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  tft.fillRect(lx,ey2,EYE_W,EYE_H,C_BLACK);
  tft.fillRect(rx,ey2,EYE_W,EYE_H,C_BLACK);
  int16_t iw=EYE_W/3,ix=rx+2+constrain(bx,0,EYE_W-iw-2);
  tft.fillRect(ix,ey2+3,iw,EYE_H-6,animBgColor);
}
void anim编辑() { busy=true;
  draw编辑(0);if(animDelay(500)){busy=false;return;}
  for(int16_t b=0;b<=EYE_W/3*2;b+=2){draw编辑(b);if(animDelay(60)){busy=false;return;}}
  if(animDelay(400)){busy=false;return;}
  for(int16_t b=EYE_W/3*2;b>=2;b-=2){draw编辑(b);if(animDelay(60)){busy=false;return;}}
  tft.fillRect(eyeRX(0)+2,eyeY()+3,EYE_W/3,EYE_H-6,C_BLACK);
  if(animDelay(100)){busy=false;return;}
  draw编辑(2);if(animDelay(300)){busy=false;return;}
  draw编辑(0);busy=false;
}

void draw执行(uint8_t ph) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  uint16_t a=(ph%6<3)?C_BLACK:animBgColor,b2=(ph%6<3)?animBgColor:C_BLACK;
  for(uint8_t x=0;x<EYE_W;x+=4){uint16_t cc=(x/4)%2==0?a:b2;
    tft.fillRect(lx+x,ey2,2,EYE_H,cc);tft.fillRect(rx+x,ey2,2,EYE_H,cc);
  }
}
void anim执行() { busy=true;
  for(uint8_t p=0;p<40;p++){draw执行(p);if(animDelay(50)){busy=false;return;}}
  draw执行(0);busy=false;
}

void draw分析(int16_t plx,int16_t ply,int16_t prx,int16_t pry) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  uint8_t pw=EYE_W/3,ph=EYE_H/3;
  tft.fillRect(lx+constrain(plx,0,EYE_W-pw),ey2+constrain(ply,0,EYE_H-ph),pw,ph,C_BLACK);
  tft.fillRect(rx+constrain(prx,0,EYE_W-pw),ey2+constrain(pry,0,EYE_H-ph),pw,ph,C_BLACK);
}
void anim分析() { busy=true;
  const int16_t p[]={0,0,0,0,8,8,8,8,16,4,16,4,12,16,12,16,4,12,4,12,0,0,0,0,8,12,8,12,16,16,16,16,4,4,4,4,12,8,12,8};
  for(uint8_t i=0;i<10;i++){draw分析(p[i*4],p[i*4+1],p[i*4+2],p[i*4+3]);if(animDelay(160)){busy=false;return;}}
  draw分析(0,0,0,0);busy=false;
}

void draw编码(uint8_t frame) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  tft.fillRect(lx,ey2,EYE_W,EYE_H,C_BLACK);tft.fillRect(rx,ey2,EYE_W,EYE_H,C_BLACK);
  uint16_t g=tft.color565(32,200,80);
  int16_t L=lx+2,R=lx+EYE_W-2,rL=rx+2,rR=rx+EYE_W-2,bot=ey2+EYE_H;
  for(uint8_t i=0;i<6;i++){
    int16_t xl=L+(R-L)*i/5,xr=rL+(rR-rL)*i/5;int16_t dh=5+(i*2)%7;uint8_t spd=2+(i%2);
    int16_t yo=(frame*spd+i*31)%(EYE_H+dh+8),y=ey2+yo-dh;
    if(y<ey2){dh-=(ey2-y);y=ey2;}if(y+dh>bot)dh=bot-y;
    if(dh>=4&&xl>=L&&xl+2<=R)tft.fillRect(xl,y,2,dh,g);
    if(dh>=4&&xr>=rL&&xr+2<=rR)tft.fillRect(xr,y,2,dh,g);
  }
}
void anim编码() { busy=true;
  for(uint8_t f=0;f<60;f++){draw编码(f);if(animDelay(45)){busy=false;return;}}
  draw编码(0);busy=false;
}

void draw震惊(uint8_t ph) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  uint8_t ew=EYE_W+8,eh=EYE_H+8;int16_t ly=ey2-4,lxx=lx-4,rxx=rx+4;
  tft.fillRect(lxx,ly,ew,eh,C_BLACK);tft.fillRect(rxx,ly,ew,eh,C_BLACK);
  tft.fillRect(lxx+ew/2-2,ly+eh/2-2,4,4,animBgColor);tft.fillRect(rxx+ew/2-2,ly+eh/2-2,4,4,animBgColor);
}
void anim震惊() { busy=true;
  draw震惊(0);if(animDelay(500)){busy=false;return;}
  const int16_t shk[]={-6,6,-5,5,-4,4,-3,3,-2,2,0};
  for(uint8_t i=0;i<11;i++){draw震惊(i%2);if(animDelay(40)){busy=false;return;}}
  drawNormalEyes(0,true);if(animDelay(100)){busy=false;return;}
  drawNormalEyes(0,false);busy=false;
}

void draw得意(int16_t ox) {
  tft.fillScreen(animBgColor);
  const int16_t lx=eyeLX(ox),rx=eyeRX(ox),ey2=eyeY();
  tft.fillRect(lx,ey2,EYE_W,EYE_H,C_BLACK);
  tft.fillRect(rx,ey2+EYE_H/2,EYE_W,EYE_H/2,C_BLACK);
  for(int8_t t=-3;t<=3;t++)tft.drawLine(rx-2,ey2+10+t,rx+EYE_W+2,ey2+t,C_BLACK);
}
void anim得意() { busy=true;
  draw得意(0);if(animDelay(500)){busy=false;return;}
  for(int8_t o=0;o<=16;o+=4){draw得意(o);if(animDelay(100)){busy=false;return;}}
  if(animDelay(600)){busy=false;return;}drawNormalEyes(0,true);if(animDelay(100)){busy=false;return;}
  draw得意(16);if(animDelay(300)){busy=false;return;}
  for(int8_t o=16;o>=0;o-=4){draw得意(o);if(animDelay(80)){busy=false;return;}}
  draw得意(0);busy=false;
}

void draw慌乱(uint8_t ph) {
  tft.fillScreen(C_WHITE);
  const int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  int16_t ps=EYE_W/4,ox1=(ph%3==0)?2:(ph%3==1?-2:0),ox2=(ph%3==1)?2:(ph%3==2?-2:0);
  tft.fillRect(lx+ox1+EYE_W/2-ps/2,ey2+EYE_H/2-ps/2,ps,ps,C_BLACK);
  tft.fillRect(rx+ox2+EYE_W/2-ps/2,ey2+EYE_H/2-ps/2,ps,ps,C_BLACK);
  uint16_t sw=tft.color565(80,160,255);
  if(ph>=1)tft.fillRect(lx+EYE_W/2-1,ey2-6,3,8,sw);
  if(ph>=2){tft.fillRect(lx+EYE_W/2-1,ey2-6,3,8,sw);tft.fillRect(rx+EYE_W/2-1,ey2-6,3,8,sw);}
}
void anim慌乱() { busy=true;
  for(uint8_t p=0;p<24;p++){draw慌乱(p);if(animDelay(60)){busy=false;return;}}
  tft.fillScreen(animBgColor);drawNormalEyes(0);busy=false;
}

const int16_t ey_pride=eyeY();
void draw骄傲(uint8_t sp) {
  tft.fillScreen(animBgColor);
  const int16_t lcx=eyeLX(0)+EYE_W/2,rcx=eyeRX(0)+EYE_W/2,cy=eyeCY();
  int16_t hw=EYE_W/2+2,arm=EYE_H/3;
  solidSeg(lcx-hw,cy+arm,lcx,cy-arm,5);solidSeg(lcx,cy-arm,lcx+hw,cy+arm,5);
  solidSeg(rcx-hw,cy+arm,rcx,cy-arm,5);solidSeg(rcx,cy-arm,rcx+hw,cy+arm,5);
  if(sp>=1){tft.fillCircle(lcx-12,ey_pride-8,3,C_BLACK);tft.fillCircle(rcx+12,ey_pride-6,2,C_BLACK);}
  if(sp>=2){tft.fillCircle(lcx-12,ey_pride-8,3,C_BLACK);tft.fillCircle(rcx+12,ey_pride-6,2,C_BLACK);tft.fillCircle(lcx-4,ey_pride-14,2,C_BLACK);tft.fillCircle(rcx+4,ey_pride-12,3,C_BLACK);}
  if(sp>=3){tft.fillCircle(lcx-12,ey_pride-8,3,C_BLACK);tft.fillCircle(rcx+12,ey_pride-6,2,C_BLACK);tft.fillCircle(lcx-4,ey_pride-14,2,C_BLACK);tft.fillCircle(rcx+4,ey_pride-12,3,C_BLACK);tft.fillCircle(lcx-18,ey_pride-2,2,C_BLACK);tft.fillCircle(rcx+18,ey_pride-4,2,C_BLACK);}
}
void anim骄傲() { busy=true;
  for(uint8_t s=0;s<=3;s++){draw骄傲(s);if(animDelay(180)){busy=false;return;}}
  if(animDelay(800)){busy=false;return;}
  for(uint8_t s=3;s>0;s--){draw骄傲(s);if(animDelay(100)){busy=false;return;}}
  tft.fillScreen(animBgColor);draw开心Eyes(0);busy=false;
}

void drawFable5(uint8_t ph) {
  tft.fillScreen(animBgColor);
  int16_t lx=eyeLX(0),rx=eyeRX(0),ey2=eyeY();
  const uint16_t pal[12]={tft.color565(80,20,180),tft.color565(60,30,200),tft.color565(35,50,220),tft.color565(20,80,210),tft.color565(15,110,190),tft.color565(30,140,160),tft.color565(150,30,140),tft.color565(200,40,80),tft.color565(20,180,100),tft.color565(60,180,180),tft.color565(40,160,200),tft.color565(100,40,200)};
  const uint8_t BW=8,BH=7;uint8_t bx=(EYE_W)/BW,by=(EYE_H)/BH;
  for(uint8_t col=0;col<bx;col++){for(uint8_t row=0;row<by;row++){
    int16_t w=(int16_t)(col*7+row*11+ph*3)&63;if(w>31)w=63-w;
    uint8_t ci=(uint8_t)((w*12)>>5);if(ci>11)ci=11;
    int16_t x=lx+col*BW,y=ey2+row*BH;uint8_t wd=BW,ht=BH;
    if(x+wd>lx+EYE_W)wd=lx+EYE_W-x;if(y+ht>ey2+EYE_H)ht=ey2+EYE_H-y;
    if(wd>0&&ht>0){tft.fillRect(x,y,wd,ht,pal[ci]);tft.fillRect(rx-eyeLX(0)+x,y,wd,ht,pal[ci]);}
  }}
}
void animFable5() { busy=true;
  for(uint8_t p=0;p<64;p++){drawFable5(p);if(animDelay(55)){busy=false;return;}}
  drawNormalEyes(0);busy=false;
}


int8_t getHour() {
  struct tm t;
  if (!getLocalTime(&t)) return -1;
  return t.tm_hour;
}

void runExpr(uint8_t v) {
  currentView = v;
  switch (v) {
    case VIEW_EYES_NORMAL:   animNormalEyes();   break;
    case VIEW_EYES_SQUISH:   animSquishEyes();   break;
    case VIEW_EYES_ANGRY:    anim生气Eyes();    break;
    case VIEW_EYES_SAD:      anim悲伤Eyes();      break;
    case VIEW_EYES_TIRED:    anim疲惫Eyes();    break;
    case VIEW_EYES_SLEEP:    anim睡觉Eyes();    break;
    case VIEW_EYES_THINK:    anim思考Eyes();    break;
    case VIEW_EYES_HAPPY:    anim开心Eyes();    break;
    case VIEW_EYES_ANNOYED:  anim烦躁Eyes();  break;
    case VIEW_EYES_KISS:     anim亲亲Eyes();     break;
    case VIEW_EYES_WINK:     animWinkEyes();     break;
    case VIEW_EYES_BORED:    anim无聊Eyes();    break;
    case VIEW_EYES_CONFUSED: animConfusedEyes(); break;
    case VIEW_EYES_DIZZY:    animDizzyEyes();    break;
    case VIEW_EYES_DEAD:     animDeadEyes();     break;
    case VIEW_EYES_LOOKAROUND: animLookaroundEyes(); break;
    case VIEW_EYES_WRITING: anim书写(); break;
    case VIEW_EYES_EDITING: anim编辑(); break;
    case VIEW_EYES_EXECUTING: anim执行(); break;
    case VIEW_EYES_ANALYZING: anim分析(); break;
    case VIEW_EYES_CODING: anim编码(); break;
    case VIEW_EYES_SURPRISE: anim震惊(); break;
    case VIEW_EYES_SMUG: anim得意(); break;
    case VIEW_EYES_PANIC: anim慌乱(); break;
    case VIEW_EYES_PROUD: anim骄傲(); break;
    case VIEW_EYES_FABLE5: animFable5(); break;
    default: animNormalEyes(); break;
  }
}


void animLogoReveal() {
  busy = true;
  tft.fillScreen(animBgColor);
  for (uint16_t i = 0; i < LOGO_SEG_COUNT; i++) {
    int16_t x1 = pgm_read_word(&LOGO_SEGS[i][0]);
    int16_t y1 = pgm_read_word(&LOGO_SEGS[i][1]);
    int16_t x2 = pgm_read_word(&LOGO_SEGS[i][2]);
    int16_t y2 = pgm_read_word(&LOGO_SEGS[i][3]);
    tft.drawLine(x1, y1, x2, y2, C_WHITE);
    tft.drawLine(x1 + 1, y1, x2 + 1, y2, C_WHITE);
    if (i % 4 == 0) { server.handleClient(); if(animDelay(8)){busy=false;return;} }
  }
  drawLogoFilled(animBgColor, C_WHITE);
  delay(1500);
  busy = false;
}

//  WEB PAGE
const char INDEX_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="zh-CN"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>蟹爪摩奇</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#1c1c20;font-family:'Courier New',monospace;color:#e8e4dc;display:flex;flex-direction:column;align-items:center;padding:20px 14px 52px;gap:14px;min-height:100vh}
.hdr{text-align:center;padding:2px 0 4px}
.mascot{font-size:15px;color:#c96a3e;line-height:1.3;font-weight:bold;font-family:'Courier New',monospace;display:block;letter-spacing:1px}
.sitename{font-size:10px;color:#5a5048;margin-top:8px;letter-spacing:3px}
.sec{width:100%;max-width:390px;font-size:10px;color:#8a8278;letter-spacing:2px;font-weight:bold;padding:0 2px}
.g{display:grid;grid-template-columns:1fr 1fr;gap:8px;width:100%;max-width:390px}
.b{background:#252428;border:1.5px solid #38343a;border-radius:12px;color:#d8d4cc;font-family:'Courier New',monospace;padding:14px 6px 10px;cursor:pointer;text-align:center;transition:all .12s;user-select:none}
.b:active:not(:disabled){transform:scale(.94)}
.b:disabled{opacity:.3;cursor:default}
.b .ic{font-size:20px;display:block;margin-bottom:4px;line-height:1;color:#c96a3e}
.b .nm{font-size:12px;font-weight:bold;color:#e8e4dc}
.b .ht{font-size:9px;color:#8a8278;margin-top:3px}
.b.on{border-color:#c96a3e;background:#201408}
.r{display:flex;gap:8px;width:100%;max-width:390px}
.r button{flex:1;background:#252428;border:1.5px solid #38343a;border-radius:10px;color:#b8b4ac;font-family:'Courier New',monospace;font-size:11px;font-weight:bold;padding:12px 4px;cursor:pointer;transition:all .12s}
.r button:active{transform:scale(.94)}
#t{position:fixed;bottom:18px;left:50%;transform:translateX(-50%);background:#252428;border:1.5px solid #38343a;border-radius:9px;font-size:12px;color:#d8d4cc;padding:7px 16px;opacity:0;transition:opacity .18s;pointer-events:none;white-space:nowrap;z-index:99}
#t.show{opacity:1}
</style></head><body>
<div class="hdr"><span class="mascot">&#x2590;&#x259B;&#x2588;&#x2588;&#x2588;&#x259C;&#x258C;<br>&#x259C;&#x2588;&#x2588;&#x2588;&#x2588;&#x2588;&#x259B;<br>&#x2598;&#x2598; &#x259D;&#x259D;</span><div class="sitename">蟹爪摩奇 · 控制器</div></div>
<div class="sec">// 表情</div>
<div class="g">
  <button class="b" onclick="x('w')"><span class="ic">&#9632; &#9632;</span><span class="nm">正常眼</span><span class="ht">晃动+眨眼</span></button>
  <button class="b" onclick="x('s')"><span class="ic">&gt; &lt;</span><span class="nm">眯眼</span><span class="ht">睁眼/闭眼</span></button>
  <button class="b" onclick="x('e')"><span class="ic">&gt;﹏&lt;</span><span class="nm">生气</span><span class="ht">抖动+眉毛</span></button>
  <button class="b" onclick="x('f')"><span class="ic">╥_╥</span><span class="nm">悲伤</span><span class="ht">泪滴</span></button>
  <button class="b" onclick="x('g')"><span class="ic">_＿_</span><span class="nm">疲惫</span><span class="ht">眼皮下垂</span></button>
  <button class="b" onclick="x('h')"><span class="ic">－ －</span><span class="nm">睡觉</span><span class="ht">Zzz飘起</span></button>
  <button class="b" onclick="x('i')"><span class="ic">O ．</span><span class="nm">思考</span><span class="ht">眼珠漂移</span></button>
  <button class="b" onclick="x('j')"><span class="ic">^ ^</span><span class="nm">开心</span><span class="ht">开心</span></button>
  <button class="b" onclick="x('k')"><span class="ic">¬ ¬</span><span class="nm">烦躁</span><span class="ht">烦躁</span></button>
  <button class="b" onclick="x('l')"><span class="ic">| ♡</span><span class="nm">亲亲</span><span class="ht">眨眼+爱心</span></button>
  <button class="b" onclick="x('m')"><span class="ic">– –</span><span class="nm">眨眼</span><span class="ht">blink blink</span></button>
  <button class="b" onclick="x('n')"><span class="ic">_ _</span><span class="nm">无聊</span><span class="ht">无聊</span></button>
  <button class="b" onclick="x('o')"><span class="ic">| |?</span><span class="nm">疑问</span><span class="ht">eyes + ?</span></button>
  <button class="b" onclick="x('p')"><span class="ic">@ @</span><span class="nm">晕</span><span class="ht">dizzy spin</span></button>
  <button class="b" onclick="x('u')"><span class="ic">X X</span><span class="nm">死掉</span><span class="ht">flash + shake</span></button>
  <button class="b" onclick="x('b')"><span class="ic">◄ ►</span><span class="nm">左顾右盼</span><span class="ht">look around</span></button>
  <button class="b" onclick="x('c')"><span class="ic">▬▬</span><span class="nm">书写</span><span class="ht">纸面书写</span></button>
  <button class="b" onclick="x('t')"><span class="ic">◧ ▤</span><span class="nm">编辑</span><span class="ht">选中拖拽</span></button>
  <button class="b" onclick="x('v')"><span class="ic">|||</span><span class="nm">执行</span><span class="ht">脉冲条纹</span></button>
  <button class="b" onclick="x('x')"><span class="ic">⊙ ⊙</span><span class="nm">分析</span><span class="ht">交叉扫描</span></button>
  <button class="b" onclick="x('y')"><span class="ic">│⌄│</span><span class="nm">编码</span><span class="ht">数码雨</span></button>
  <button class="b" onclick="x('z')"><span class="ic">◎ ◎</span><span class="nm">震惊</span><span class="ht">放大+颤</span></button>
  <button class="b" onclick="x('1')"><span class="ic">◉ –</span><span class="nm">得意</span><span class="ht">眯眼+闪</span></button>
  <button class="b" onclick="x('2')"><span class="ic">° °</span><span class="nm">慌乱</span><span class="ht">反色+汗</span></button>
  <button class="b" onclick="x('3')"><span class="ic">^★^</span><span class="nm">骄傲</span><span class="ht">星星爆发</span></button>
  <button class="b" onclick="x('4')"><span class="ic">▓▓</span><span class="nm">极光</span><span class="ht">像素波动</span></button>

<div class="r"><button onclick="x('r')">&#8635; 自动模式</button><button onclick="f('backlight?on=1')">&#9728; 背光</button></div>
<div id="t"></div>
<script>
var ak='w';
function x(k){
 fetch('/cmd?k='+k).catch(function(){});
 var t=document.getElementById('t');t.textContent=k;t.className='show';clearTimeout(t._t);t._t=setTimeout(function(){t.className=''},800);
 var oe=document.getElementById('b'+ak);if(oe)oe.classList.remove('on');
 var ne=document.getElementById('b'+k);if(ne)ne.classList.add('on');ak=k;
}
function f(p){fetch('/'+p).catch(function(){})}
</script></body></html>)rawhtml";//  WEB ROUTES

void routeRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void routeCmd() {
  if (!server.hasArg("k") || server.arg("k").isEmpty()) {
    server.send(400, "application/json", "{\"e\":1}"); return;
  }
  const char c = server.arg("k")[0];

  if (termMode) {
    if (c == 'q') { termMode = false; drawCodeView(); }
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }

  if (busy && c != 'r') {
    server.send(200, "application/json", "{\"ok\":1}");
    switch(c){case'w':pendingView=0;break;case's':pendingView=1;break;case'd':pendingView=2;break;case'e':pendingView=4;break;case'f':pendingView=5;break;case'g':pendingView=6;break;case'h':pendingView=7;break;case'i':pendingView=8;break;case'j':pendingView=9;break;case'k':pendingView=10;break;case'l':pendingView=11;break;case'm':pendingView=12;break;case'n':pendingView=14;break;case'o':pendingView=15;break;case'p':pendingView=16;break;case'u':pendingView=17;break;case'b':pendingView=18;break;case'c':pendingView=19;break;case't':pendingView=20;break;case'v':pendingView=21;break;case'x':pendingView=22;break;case'y':pendingView=23;break;case'z':pendingView=24;break;case'1':pendingView=25;break;case'2':pendingView=26;break;case'3':pendingView=27;break;case'4':pendingView=28;break;}
    return;
  }

  server.send(200, "application/json", "{\"ok\":1}");
  if (c != 'r') { manualOverride = true; overrideEnd = millis() + 600000UL; }
  switch (c) {
    case 'w': currentView = VIEW_EYES_NORMAL;  animNormalEyes();  break;
    case 's': currentView = VIEW_EYES_SQUISH;  animSquishEyes();  break;
    case 'd':
      break; // terminal removed
    case 'a':
      currentView = VIEW_EYES_NORMAL;
      animLogoReveal();
      break;
    case 'e': currentView = VIEW_EYES_ANGRY;   anim生气Eyes();   break;
    case 'f': currentView = VIEW_EYES_SAD;     anim悲伤Eyes();     break;
    case 'g': currentView = VIEW_EYES_TIRED;   anim疲惫Eyes();   break;
    case 'h': currentView = VIEW_EYES_SLEEP;   anim睡觉Eyes();   break;
    case 'i': currentView = VIEW_EYES_THINK;   anim思考Eyes();   break;
    case 'j': currentView = VIEW_EYES_HAPPY;   anim开心Eyes();   break;
    case 'k': currentView = VIEW_EYES_ANNOYED; anim烦躁Eyes(); break;
    case 'l': currentView = VIEW_EYES_KISS;    anim亲亲Eyes();    break;
    case 'm': currentView = VIEW_EYES_WINK;     animWinkEyes();     break;
    case 'n': currentView = VIEW_EYES_BORED;    anim无聊Eyes();    break;
    case 'o': currentView = VIEW_EYES_CONFUSED; animConfusedEyes(); break;
    case 'p': currentView = VIEW_EYES_DIZZY;    animDizzyEyes();    break;
    case 'u': currentView = VIEW_EYES_DEAD;     animDeadEyes();     break;
    case 'b': currentView = VIEW_EYES_LOOKAROUND; animLookaroundEyes(); break;
    case 'r': manualOverride = false; break;
    case 'c': currentView = VIEW_EYES_WRITING; anim书写(); break;
    case 't': currentView = VIEW_EYES_EDITING; anim编辑(); break;
    case 'v': currentView = VIEW_EYES_EXECUTING; anim执行(); break;
    case 'x': currentView = VIEW_EYES_ANALYZING; anim分析(); break;
    case 'y': currentView = VIEW_EYES_CODING; anim编码(); break;
    case 'z': currentView = VIEW_EYES_SURPRISE; anim震惊(); break;
    case '1': currentView = VIEW_EYES_SMUG; anim得意(); break;
    case '2': currentView = VIEW_EYES_PANIC; anim慌乱(); break;
    case '3': currentView = VIEW_EYES_PROUD; anim骄傲(); break;
    case '4': currentView = VIEW_EYES_FABLE5; animFable5(); break;
  }
}







void routeBacklight() {
  setBacklight(server.hasArg("on") && server.arg("on") == "1");
  server.send(200, "application/json", "{\"ok\":1}");
}


void routeState() {
  String j = "{\"view\":";
  j += currentView;
  j += ",\"busy\":";   j += busy        ? "true" : "false";
  j += ",\"term\":";   j += termMode    ? "true" : "false";
  j += ",\"bl\":";     j += backlightOn ? "true" : "false";
  j += ",\"speed\":";  j += animSpeed;
  j += "}";
  server.send(200, "application/json", j);
}

void routeNotFound() { server.send(404, "text/plain", "not found"); }

//  SETUP

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  setBacklight(true);
  SPI.begin(8, -1, 10, TFT_CS);   // SCK=8, MOSI=10
  tft.init(240, 240);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  initColours();

  tft.fillScreen(animBgColor);
  tft.setTextColor(C_WHITE); tft.setTextSize(3);
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 - 22); tft.print("Clawd");
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 + 14); tft.print("Mochi");
  delay(1200);
  // 只有真正断电重上电才播放开机动画。RTC 记忆区分真上电 vs 热复位。
  if (!hasBooted) {
    animLogoReveal();
    hasBooted = true;
  }

  tft.fillScreen(C_DARKBG);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(12, 80); tft.print("Connecting...");
  WiFi.mode(WIFI_STA);WiFi.begin("dummy","dummy");
  delay(200);WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ClaWD-Mochi", "clawd1234");
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0,0,DISP_W,4,C_ORANGE);
  tft.setTextColor(C_WHITE);tft.setTextSize(2);
  tft.setCursor(12,16);tft.print("ClaWD-Mochi");
  tft.setTextColor(C_MUTED);tft.setTextSize(1);
  tft.setCursor(12,46);tft.print("WiFi: ClaWD-Mochi");
  tft.setCursor(12,62);tft.print("PW: clawd1234");
  tft.setCursor(12,78);tft.print("http://192.168.4.1");
  delay(2000);

  server.on("/",            HTTP_GET, routeRoot);
  server.on("/cmd",         HTTP_GET, routeCmd);
  //server.on("/char",        HTTP_GET, routeChar);
  //server.on("/speed",       HTTP_GET, routeSpeed);
  //server.on("/redraw",      HTTP_GET, routeRedraw);
  //server.on("/canvas",      HTTP_GET, routeCanvas);
  //server.on("/draw/clear",  HTTP_GET, routeDrawClear);
  //server.on("/draw/stroke", HTTP_GET, routeDrawStroke);
  server.on("/backlight",   HTTP_GET, routeBacklight);
  server.on("/state",       HTTP_GET, routeState);
  server.onNotFound(routeNotFound);
  server.begin();

  NimBLEDevice::init("Clawd-Mochi");
  NimBLEServer* pBs=NimBLEDevice::createServer();
  NimBLEService* pSv=pBs->createService("FFE0");
  pSv->createCharacteristic("FFE1",NIMBLE_PROPERTY::WRITE)->setCallbacks(new ClawdBLECb());
  pSv->start();
  pBs->getAdvertising()->addServiceUUID("FFE0");
  pBs->getAdvertising()->setName("Clawd-Mochi");
  pBs->getAdvertising()->start();

  lastExprMs  = millis();
  nextExprMs  = 10000;
}

//  SERIAL STATE MACHINE — PC-driven real-time expression control

//  LOOP

void checkSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();
  if (cmd == 'a') return;
  if (cmd != 'w' && cmd != 's' && cmd != 'e' && cmd != 'f' && cmd != 'g' &&
      cmd != 'h' && cmd != 'i' && cmd != 'j' && cmd != 'k' && cmd != 'l' &&
      cmd != 'm' && cmd != 'n' && cmd != 'o' && cmd != 'p' && cmd != 'u' &&
      cmd != 'b' && cmd != 'c' && cmd != 't' && cmd != 'v' && cmd != 'x' &&
      cmd != 'y' && cmd != 'z' && cmd != '1' && cmd != '2' && cmd != '3' && cmd != '4') return;
  switch (cmd) {
    case 'w': if(!busy){runExpr(VIEW_EYES_NORMAL);}else{pendingView=0;}break;
    case 's': if(!busy){runExpr(VIEW_EYES_SQUISH);}else{pendingView=1;}break;
    case 'e': if(!busy){runExpr(VIEW_EYES_ANGRY);}else{pendingView=4;}break;
    case 'f': if(!busy){runExpr(VIEW_EYES_SAD);}else{pendingView=5;}break;
    case 'g': if(!busy){runExpr(VIEW_EYES_TIRED);}else{pendingView=6;}break;
    case 'h': if(!busy){runExpr(VIEW_EYES_SLEEP);}else{pendingView=7;}break;
    case 'i': if(!busy){runExpr(VIEW_EYES_THINK);}else{pendingView=8;}break;
    case 'j': if(!busy){runExpr(VIEW_EYES_HAPPY);}else{pendingView=9;}break;
    case 'k': if(!busy){runExpr(VIEW_EYES_ANNOYED);}else{pendingView=10;}break;
    case 'l': if(!busy){runExpr(VIEW_EYES_KISS);}else{pendingView=11;}break;
    case 'm': if(!busy){runExpr(VIEW_EYES_WINK);}else{pendingView=12;}break;
    case 'n': if(!busy){runExpr(VIEW_EYES_BORED);}else{pendingView=14;}break;
    case 'o': if(!busy){runExpr(VIEW_EYES_CONFUSED);}else{pendingView=15;}break;
    case 'p': if(!busy){runExpr(VIEW_EYES_DIZZY);}else{pendingView=16;}break;
    case 'u': if(!busy){runExpr(VIEW_EYES_DEAD);}else{pendingView=17;}break;
    case 'b': if(!busy){runExpr(VIEW_EYES_LOOKAROUND);}else{pendingView=18;}break;
    case 'c': if(!busy){runExpr(VIEW_EYES_WRITING);}else{pendingView=19;}break;
    case 't': if(!busy){runExpr(VIEW_EYES_EDITING);}else{pendingView=20;}break;
    case 'v': if(!busy){runExpr(VIEW_EYES_EXECUTING);}else{pendingView=21;}break;
    case 'x': if(!busy){runExpr(VIEW_EYES_ANALYZING);}else{pendingView=22;}break;
    case 'y': if(!busy){runExpr(VIEW_EYES_CODING);}else{pendingView=23;}break;
    case 'z': if(!busy){runExpr(VIEW_EYES_SURPRISE);}else{pendingView=24;}break;
    case '1': if(!busy){runExpr(VIEW_EYES_SMUG);}else{pendingView=25;}break;
    case '2': if(!busy){runExpr(VIEW_EYES_PANIC);}else{pendingView=26;}break;
    case '3': if(!busy){runExpr(VIEW_EYES_PROUD);}else{pendingView=27;}break;
    case '4': if(!busy){runExpr(VIEW_EYES_FABLE5);}else{pendingView=28;}break;
  }
}

void loop() {
  checkSerial();
  server.handleClient();
  if (pendingView != 255) {
    uint8_t pv = pendingView; pendingView = 255;
    busy = false; runExpr(pv);
    exprPlayCount = 0; lastExprMs = millis(); nextExprMs = 20000UL;
    return;
  }
  if (busy) return;

  uint32_t now = millis();

  // 2. 手动模式：20秒过期后自动回到时间逻辑内的表情
  if (manualOverride) {
    if (now - (overrideEnd - 600000UL) >= 20000UL) {
      manualOverride = false;
      int8_t h = getHour();
      if (h >= 23 || h < 8) {
        // 不用runExpr避免阻塞，直接设置状态让轮播接管
        currentView = VIEW_EYES_SLEEP;
        draw睡觉Eyes(3);
        lastExprMs = now;
        nextExprMs = 8000;
        exprPlayCount = 0;
        return;
      }
      if (h >= 12 && h < 13) {
        currentView = VIEW_EYES_TIRED;
        draw疲惫Eyes(30);
        lastExprMs = now;
        nextExprMs = 15000;
        exprPlayCount = 0;
        return;
      }
      lastExprMs = now;
      nextExprMs = 3000;
      exprPlayCount = 0;
      return;
    }
    static uint32_t lastBlinkMs = 0;
    if (currentView == VIEW_EYES_NORMAL && (now - lastBlinkMs > 4000)) {
      animNormalEyes();
      lastBlinkMs = now;
    }
    return;
  }

  // 3. 自动轮巡冷却时间检查
  if (now - lastExprMs < nextExprMs) return;

  // 基于特定时间的强规则（持续播放，不切换）
  int8_t h = getHour();
  // 23:00 - 08:00 → 睡觉持续播放
  if (h >= 23 || h < 8) {
    currentView = VIEW_EYES_SLEEP;
    anim睡觉Eyes();  // 这里用anim因为是正常轮播周期，不是切换
    lastExprMs = now;
    nextExprMs = 8000;
    return;
  }
  // 12:00 - 13:00 → 困持续播放
  if (h >= 12 && h < 13) {
    currentView = VIEW_EYES_TIRED;
    anim疲惫Eyes();
    lastExprMs = now;
    nextExprMs = 15000;
    return;
  }

  // 4. 同一表情连播 EXPR_REPEAT 次后再切换
  if (exprPlayCount < EXPR_REPEAT) {
    runExpr(currentView);          // 重播当前表情
    exprPlayCount++;
    lastExprMs = now;
    nextExprMs = 2000;
    return;
  }

  // 5. 连播次数用完，随机挑一个新表情
  exprPlayCount = 0;
  uint8_t nextView;
  if(currentView==VIEW_EYES_FABLE5){nextView=VIEW_EYES_FABLE5;}else{uint8_t choice=random(0,100);

  if(choice<45)nextView=VIEW_EYES_NORMAL;else if(choice<75)nextView=VIEW_EYES_TIRED;else nextView=VIEW_EYES_SLEEP;}

  // 触发选中的新表情（第一次播放）
  runExpr(nextView);
  exprPlayCount = 1;
  lastExprMs = now;
  nextExprMs = random(6000, 10000);
}