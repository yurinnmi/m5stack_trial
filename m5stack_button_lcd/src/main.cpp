#include <M5Unified.h>

namespace {
// RGB565 color definitions.
constexpr uint16_t COLOR_BLACK     = 0x0000;
constexpr uint16_t COLOR_WHITE     = 0xFFFF;
constexpr uint16_t COLOR_YELLOW    = 0xFFE0;
constexpr uint16_t COLOR_DARK_BLUE = 0x0010;  // dark navy blue

void drawCenteredMessage(uint16_t backgroundColor, uint16_t textColor, const char* message) {
  auto& display = M5.Display;

  display.startWrite();
  display.fillScreen(backgroundColor);

  // Use a Japanese-capable font. Text is UTF-8 encoded in this source file.
  display.setFont(&fonts::efontJA_24);
  display.setTextSize(2);
  display.setTextDatum(middle_center);
  display.setTextColor(textColor, backgroundColor);
  display.setTextPadding(0);

  display.drawString(message, display.width() / 2, display.height() / 2);
  display.endWrite();
}
}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  // Landscape orientation for M5Stack Core ESP32 LCD.
  M5.Display.setRotation(1);

  // Initial blank screen. The requested display state is selected by buttons A/B/C.
  M5.Display.fillScreen(COLOR_BLACK);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    drawCenteredMessage(COLOR_WHITE, COLOR_BLACK, "おはよう");
  }

  if (M5.BtnB.wasPressed()) {
    drawCenteredMessage(COLOR_YELLOW, COLOR_BLACK, "こんにちは");
  }

  if (M5.BtnC.wasPressed()) {
    drawCenteredMessage(COLOR_DARK_BLUE, COLOR_WHITE, "こんにちは");
  }

  delay(10);
}
