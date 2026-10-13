// ─────────────────────────────────────────────────────────────────────
//  DesktopBuddy v6.3 (Fixed Sleep, OTA, and No Duplicate Definitions)
// ─────────────────────────────────────────────────────────────────────
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"
#include "settings.h"
#include "face.h"
#include "weather.h"
#include "net.h"

Adafruit_SH1106G display(SW, SH, &SPI, OLED_DC, OLED_RST, OLED_CS, 8000000UL);

bool     showInfo      = false;
uint32_t lastWeather   = 0;
uint32_t lastTouch     = 0;

uint32_t topPressStart = 0, leftPressStart = 0, rightPressStart = 0;
uint32_t lastTapTime   = 0, lastLeftRelease = 0, lastRightRelease = 0;
uint8_t  tapCount      = 0;
bool     leftWasLong   = false, rightWasLong = false;
const uint32_t TAP_WINDOW = 800;

void touches(){
  if(millis() - lastTouch < 100) return;
  lastTouch = millis();

  static bool pT=false, pL=false, pR=false;
  bool top   = digitalRead(TOUCH_TOP);
  bool left  = digitalRead(TOUCH_LEFT);
  bool right = digitalRead(TOUCH_RIGHT);

  holdLeft = false;
  holdRight = false;
  holdTop = false;

  // Любое новое касание будит устройство (сбрасывает таймер сна через wakeUp() из face.cpp)
  if((top&&!pT) || (left&&!pL) || (right&&!pR)) wakeUp();

  // ── ЛЕВАЯ КНОПКА ──
  if (left && !pL) {
    leftPressStart = millis();
    if (millis() - lastLeftRelease < 1500 && leftWasLong) {
      playThen(A_ANGRY, A_IDLE); leftWasLong = false;
    } else { playLeftAnim(); }
  }
  if (left) { if (millis() - leftPressStart > 400) holdLeft = true; }
  if (!left && pL) {
    if (millis() - leftPressStart > 1000) {
      leftWasLong = true; lastLeftRelease = millis();
      playThen(A_DISGUSTED, A_IDLE);
    }
    leftPressStart = 0;
  }

  // ── ПРАВАЯ КНОПКА ──
  if (right && !pR) {
    rightPressStart = millis();
    if (millis() - lastRightRelease < 1500 && rightWasLong) {
      playThen(A_ANGRY, A_IDLE); rightWasLong = false;
    } else { playRightAnim(); }
  }
  if (right) { if (millis() - rightPressStart > 400) holdRight = true; }
  if (!right && pR) {
    if (millis() - rightPressStart > 1000) {
      rightWasLong = true; lastRightRelease = millis();
      playThen(A_DISGUSTED, A_IDLE);
    }
    rightPressStart = 0;
  }

  // ── ВЕРХНЯЯ КНОПКА ──
  if (top && !pT) {
    topPressStart = millis();
    if (showInfo) { showInfo = false; return; } // Выход из инфо-экрана
    if (millis() - lastTapTime < TAP_WINDOW) tapCount++;
    else tapCount = 1;
    lastTapTime = millis();
  }
  
  if (top && (millis() - topPressStart > 400) && !showInfo) holdTop = true;

  if (!top && pT) {
    if (!showInfo && millis() - topPressStart < 800) {
      if (tapCount >= 3) { showInfo = true; tapCount = 0; }
      else { playTopAnim(); }
    }
    topPressStart = 0;
  }

  pT = top; pL = left; pR = right;
}

void setup(){
  Serial.begin(115200);
  pinMode(TOUCH_TOP,   INPUT);
  pinMode(TOUCH_LEFT,  INPUT);
  pinMode(TOUCH_RIGHT, INPUT);
  randomSeed(analogRead(0));

  SPI.begin(OLED_SCLK, -1, OLED_MOSI);
  display.begin(0x3C, true);
  display.setContrast(0xCF);
  display.clearDisplay(); display.display();

  loadSettings();

  play(A_IDLE);
  lastInteraction = millis();
  everInteracted = true;  // Разрешаем сон, даже если кнопки не нажимались
  lastBlinkTime   = millis(); 
  nextBlinkGap    = 3000;
  scheduleNextRandom(); 

  netBegin();
  
  if(WiFi.status() == WL_CONNECTED){
    netApplyTime();
    weatherUpdate();
  }
  lastWeather = millis();
}

void loop(){
  uint32_t now = millis();
  static uint32_t lastFrame = 0;
  
  if(now - lastFrame < FRAME_MS) return;
  lastFrame = now;
  gTick++;

  touches();
  netLoop();

  // 1. Авто-моргание в IDLE
  if(!isSleeping && player.cur == A_IDLE){
    if(now - lastBlinkTime > nextBlinkGap){
      lastBlinkTime = now; 
      nextBlinkGap = random(2500, 6000);
      playThen(A_BLINK, A_IDLE);
    }
  }

  // 2. Рандомные анимации в простое (не сбрасывают таймер сна!)
  if(!isSleeping && player.cur == A_IDLE && now >= nextRandomAnim){
    playThen(randomAnim(), A_IDLE); 
    scheduleNextRandom();
  }

  // 3. ★ ИСПРАВЛЕННЫЙ СОН: 1 минута после ПОСЛЕДНЕГО КАСАНИЯ (для тестов)
  if(!isSleeping && (now - lastInteraction > 1UL * 60UL * 1000UL)){
    Serial.println("[Sleep] Entering sleep mode");
    isSleeping = true; 
    
    // Разные анимации сна для разнообразия
    static const uint8_t sleepAnims[] = { A_SLEEP_ZZZ, A_SLEEPY, A_BLINK_SLEEPY };
    play(sleepAnims[random(3)]);
    
    // ★ КЛЮЧЕВОЕ ИСПРАВЛЕНИЕ: делаем анимацию бесконечной, чтобы не проснулся сам
    player.duration = -1; 
  }

  // 4. Отрисовка
  if(showInfo){
    if(WiFi.status() == WL_CONNECTED) drawInfoScreen(temperature, weatherCode);
    else                              drawApInfoScreen();
    return;
  }

  updateAnim();
  drawFrame();

  // Обновление погоды каждые 15 минут
  if(now - lastWeather > 900000UL){ 
    weatherUpdate(); 
    lastWeather = now; 
  }
}