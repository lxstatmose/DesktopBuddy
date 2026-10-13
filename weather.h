#pragma once
extern int temperature;
extern int weatherCode;
extern bool weatherDataValid;
void weatherUpdate();
void resolveCityIfNeeded();