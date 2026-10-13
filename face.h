#pragma once
#include <Arduino.h>

enum Anim : uint8_t {
  A_IDLE = 0, A_BLINK, A_BLINK_DOUBLE, A_WINK_L, A_WINK_R, A_BLINK_SLEEPY, A_FLUTTER,
  A_LOOK_LEFT, A_LOOK_RIGHT, A_LOOK_UP, A_LOOK_DOWN, A_LOOK_AROUND,
  A_PEEK_LEFT, A_PEEK_RIGHT, A_PEEK_TOP, A_PEEK_BOTTOM, A_FOLLOW_DOT,
  A_HAPPY, A_SAD, A_ANGRY, A_SURPRISED, A_SLEEPY,
  A_SCARED, A_DISGUSTED, A_LOVE, A_COOL, A_CONFUSED,
  A_LAUGH, A_CRY, A_DIZZY, A_SPIN, A_JUMP, A_FALL,
  A_SHAKE_NO, A_NOD_YES, A_ZOOM_IN, A_ZOOM_OUT, A_FADE_GHOST, A_GLITCH,
  A_HEART_EYES, A_STAR_EYES, A_SPIRAL_EYES, A_X_EYES,
  A_DOLLAR_EYES, A_SLEEP_ZZZ, A_SEARCH, A_RADAR,
  A_WAVE_HELLO, A_POP_IN, A_MELT, A_GROW, A_SHRINK, A_JELLY, A_PULSE,
  A_FLICKER, A_RAIN, A_SNOW, A_FIREWORKS, A_BREATHE, A_SWING, A_DISCO,
  A_COUNT
};

struct FaceState {
  float openL=1, openR=1;
  float faceX=0, faceY=0;
  float gazeX=0, gazeY=0;
  float scaleX=1, scaleY=1;
  float radius=6;
  int   browL=0, browR=0;
  int   pupil=0;
  float wiggle=0;
};

struct Player { uint8_t cur=A_IDLE, next=A_IDLE; int frame=0, duration=-1; };

extern FaceState f;
extern Player    player;
extern uint32_t  gTick;
extern uint32_t  lastInteraction, nextRandomAnim, lastBlinkTime, nextBlinkGap;
extern bool      isSleeping, everInteracted;
extern const uint32_t SLEEP_TIMEOUT;

extern bool holdLeft;
extern bool holdRight;
extern bool holdTop;
extern uint8_t holdReaction;

void    play(uint8_t a);
void    playThen(uint8_t a, uint8_t after);
void    updateAnim();
void    drawFrame();
void    drawInfoScreen(int temp, int wcode);
void    drawApInfoScreen();
void    wakeUp();
void    scheduleNextRandom();
uint8_t randomAnim();
void    playTopAnim();
void    playLeftAnim();
void    playRightAnim();
void    setAnimMode(uint8_t mode);
uint8_t getAnimMode();
const char* animName(uint8_t a);