#pragma once
#include <Arduino.h>

void netApplyTime();
void startAP();
bool netConnected();
String netGetIP();
void netBegin();
void netLoop();