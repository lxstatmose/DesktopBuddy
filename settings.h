#pragma once
#include <Arduino.h>

struct Settings {
  char  city[33];
  float lat;
  float lon;
  float tzHours;
  char  apName[33];
  char  apPassword[65];
  char  wifiSsid[33];
  char  wifiPass[65];
  uint8_t animMode;
};

extern Settings cfg;
void loadSettings();
void saveSettings();