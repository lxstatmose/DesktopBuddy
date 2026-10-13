#include "weather.h"
#include "settings.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

int temperature = 0;
int weatherCode = -1;
bool weatherDataValid = false;

void resolveCityIfNeeded(){
  if(!(cfg.lat==0.0f && cfg.lon==0.0f)) return;
  if(cfg.city[0]==0) return;
  if(WiFi.status()!=WL_CONNECTED) return;
  WiFiClientSecure cl; cl.setInsecure();
  HTTPClient h;
  String u = "https://geocoding-api.open-meteo.com/v1/search?name="
           + String(cfg.city) + "&count=1&language=ru&format=json";
  if(h.begin(cl,u) && h.GET()==HTTP_CODE_OK){
    JsonDocument d;
    if(!deserializeJson(d,h.getStream()) && d["results"].size()>0){
      cfg.lat = d["results"][0]["latitude"]  | 0.0f;
      cfg.lon = d["results"][0]["longitude"] | 0.0f;
      saveSettings();
    }
  }
  h.end();
}

void weatherUpdate(){
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("[Weather] WiFi not connected");
    return;
  }
  resolveCityIfNeeded();
  if(cfg.lat == 0.0f && cfg.lon == 0.0f) {
    Serial.println("[Weather] Coordinates are 0. Check city name in settings");
    weatherDataValid = false;
    return;
  }
  Serial.printf("[Weather] Fetching for: %s (%.4f, %.4f)\n", cfg.city, cfg.lat, cfg.lon);
  WiFiClientSecure cl; cl.setInsecure();
  HTTPClient h;
  String u = "https://api.open-meteo.com/v1/forecast?latitude="
           + String(cfg.lat, 4) + "&longitude=" + String(cfg.lon, 4)
           + "&current=temperature_2m,weather_code";
  if(h.begin(cl, u) && h.GET() == HTTP_CODE_OK){
    JsonDocument d;
    if(!deserializeJson(d, h.getStream())){
      temperature = (int)round((float)(d["current"]["temperature_2m"] | 0.0f));
      weatherCode = d["current"]["weather_code"] | -1;
      weatherDataValid = true;
      Serial.printf("[Weather] Success: %d C, code %d\n", temperature, weatherCode);
    } else {
      Serial.println("[Weather] JSON parse error");
      weatherDataValid = false;
    }
  } else {
    Serial.printf("[Weather] HTTP request failed, code: %d\n", h.GET());
    weatherDataValid = false;
  }
  h.end();
}