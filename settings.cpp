#include "settings.h"
#include <Preferences.h>

Settings cfg;
static Preferences prefs;

void loadSettings(){
  prefs.begin("buddy", true);
  String c = prefs.getString("city", "Moscow");
  strncpy(cfg.city, c.c_str(), 32); cfg.city[32]=0;
  cfg.lat     = prefs.getFloat("lat", 55.7558f);
  cfg.lon     = prefs.getFloat("lon", 37.6173f);
  cfg.tzHours = prefs.getFloat("tz",  3.0f);

  String ap = prefs.getString("ap", "DesktopBuddy");
  strncpy(cfg.apName, ap.c_str(), 32); cfg.apName[32]=0;
  String pw = prefs.getString("pw", "buddy1234");
  strncpy(cfg.apPassword, pw.c_str(), 64); cfg.apPassword[64]=0;

  String ws = prefs.getString("wssid", "");
  strncpy(cfg.wifiSsid, ws.c_str(), 32); cfg.wifiSsid[32]=0;
  String wp = prefs.getString("wpass", "");
  strncpy(cfg.wifiPass, wp.c_str(), 64); cfg.wifiPass[64]=0;

  cfg.animMode = prefs.getUInt("am", 0);
  prefs.end();
}

void saveSettings(){
  prefs.begin("buddy", false);
  prefs.putString("city", cfg.city);
  prefs.putFloat("lat",  cfg.lat);
  prefs.putFloat("lon",  cfg.lon);
  prefs.putFloat("tz",   cfg.tzHours);
  prefs.putString("ap",   cfg.apName);
  prefs.putString("pw",   cfg.apPassword);
  prefs.putString("wssid", cfg.wifiSsid);
  prefs.putString("wpass", cfg.wifiPass);
  prefs.putUInt("am", cfg.animMode);
  prefs.end();
}