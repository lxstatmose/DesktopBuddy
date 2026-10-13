#include "face.h"
#include "config.h"
#include "settings.h"
#include "weather.h"
#include <WiFi.h>
#include <time.h>

FaceState f;
Player    player;
uint32_t  gTick=0;
uint32_t  lastInteraction=0, nextRandomAnim=0, lastBlinkTime=0, nextBlinkGap=3000;
bool      isSleeping=false, everInteracted=false;
const uint32_t SLEEP_TIMEOUT = 3UL * 60UL * 1000UL;

bool holdLeft = false;
bool holdRight = false;
bool holdTop = false;
uint8_t holdReaction = 0;

const int EYE_W=26, EYE_H=20, EYE_SP=18;
const int EYE_CX=SW/2, EYE_CY=SH/2-2, NOSE_GAP=9;

const int animDur[A_COUNT] = {
  -1, 14,28,14,14,26,30, 40,40,40,40,90,70,70,70,70,120,
  70,80,70,50,80,90,60,90,70,80, 90,90,80,70,70,70,50,50,50,50,60,60,
  80,80,80,60,70,110,90,100, 70,40,80,60,60,70,70,60,110,110,90,80,70,70
};

void play(uint8_t a){ player.cur=a; player.frame=0; player.duration=animDur[a]; }
void playThen(uint8_t a,uint8_t after){ play(a); player.next=after; }
void scheduleNextRandom(){ nextRandomAnim = millis() + random(5000, 20000); }
uint8_t randomAnim(){ return (uint8_t)random(1,A_COUNT); }
void setAnimMode(uint8_t mode){}
uint8_t getAnimMode(){ return 0; }

void wakeUp(){
  if(isSleeping){ 
    isSleeping=false; 
    playThen(A_BLINK, A_IDLE); 
  }
  lastInteraction=millis(); 
  everInteracted=true; 
  scheduleNextRandom();
}

float ease(float t){ return t*t*(3-2*t); }
float ping(float t){ return t<0.5f ? t*2 : 2-t*2; }
float osc(float t,float fr){ return sinf(t*fr*6.28318f); }
float seg(float t,float a,float b){
  if(t<=a) return 0; if(t>=b) return 1; return ease((t-a)/(b-a));
}

void resetFace(){
  f.openL=f.openR=1; f.faceX=f.faceY=0; f.gazeX=f.gazeY=0;
  f.scaleX=f.scaleY=1; f.radius=6; f.browL=f.browR=0; f.pupil=0; f.wiggle=0;
}

int tx(){ return (int)(f.faceX + f.gazeX*3.0f); }
int ty(){ return (int)(f.faceY + f.gazeY*3.0f); }
int pox(){ return (int)(f.gazeX*3.0f); }
int poy(){ return (int)(f.gazeY*3.0f); }

void updateAnim(){
  float t  = (player.duration>0) ? (float)player.frame/player.duration : 0;
  float p  = ease(t);
  float pp = ping(t);
  resetFace();

  if (holdLeft || holdRight || holdTop) {
    if (holdReaction == 0) {
      if (holdTop) {
        holdReaction = random(1, 5);
      } else {
        holdReaction = random(5, 9);
      }
    }
    switch(holdReaction) {
      case 1: f.pupil = 1; f.radius = 10; f.faceY = 8; f.gazeY = 0.5f; f.wiggle = 1.0f; f.openL = f.openR = 0.5f; break;
      case 2: f.faceY = 6; f.gazeY = 0.6f; f.openL = f.openR = 0.4f; f.browL = f.browR = 3; f.radius = 8; f.wiggle = 0.2f; break;
      case 3: f.faceY = 4; f.openL = f.openR = 0.5f; f.browL = f.browR = -3; f.radius = 10; f.wiggle = 0.8f; break;
      case 4: f.scaleX = f.scaleY = 1.2f; f.radius = 11; f.openL = f.openR = 1.0f; f.faceY = -4; break;
      case 5: if(holdLeft) { f.faceX = 10; f.gazeX = 1.0f; } else { f.faceX = -10; f.gazeX = -1.0f; } f.openL = f.openR = 0.7f; f.browL = f.browR = -5; f.radius = 4; break;
      case 6: if(holdLeft) { f.faceX = 8; } else { f.faceX = -8; } f.scaleX = f.scaleY = 0.8f; f.radius = 10; f.openL = f.openR = 1.0f; f.browL = f.browR = 5; f.gazeY = -0.3f; break;
      case 7: if(holdLeft) { f.faceX = 12; f.gazeX = 0.8f; } else { f.faceX = -12; f.gazeX = -0.8f; } f.openL = 0.5f; f.openR = 0.8f; f.browL = -3; f.browR = 2; break;
      case 8: if(holdLeft) { f.faceX = 6; } else { f.faceX = -6; } f.gazeX = (holdLeft ? 0.4f : -0.4f); f.openL = 0.9f; f.openR = 0.6f; f.browL = 3; f.browR = -3; f.wiggle = 0.4f; break;
    }
    return;
  } else {
    holdReaction = 0;
  }

  switch(player.cur){
    case A_IDLE: f.faceY = sinf(gTick*0.04f)*2.0f; f.gazeX = sinf(gTick*0.011f)*0.3f; f.wiggle = 0.5f; break;
    case A_BLINK: f.openL=f.openR=1-pp; break;
    case A_BLINK_DOUBLE: { float s=t*2; f.openL=f.openR=(s<1)?(1-ping(s)):(1-ping(s-1)); break; }
    case A_WINK_L: f.openL=1-pp; f.browR=(int)(pp*3); break;
    case A_WINK_R: f.openR=1-pp; f.browL=(int)(pp*3); break;
    case A_BLINK_SLEEPY: f.openL=f.openR=1-pp; f.faceY=pp*4; break;
    case A_FLUTTER: f.openL=f.openR=0.5f+0.5f*osc(t,6); break;
    case A_LOOK_LEFT:  f.faceX=-p*16; f.gazeX=-p; break;
    case A_LOOK_RIGHT: f.faceX= p*16; f.gazeX= p; break;
    case A_LOOK_UP:    f.faceY=-p*12; f.gazeY=-p; break;
    case A_LOOK_DOWN:  f.faceY= p*12; f.gazeY= p; break;
    case A_LOOK_AROUND: { float a=seg(t,0,0.30f), b=seg(t,0.38f,0.68f), c=seg(t,0.76f,1); f.faceX = -a*22 + b*44 - c*22; f.gazeX = -a + b*2 - c; break; }
    case A_PEEK_LEFT: { float out=seg(t,0,0.28f), back=seg(t,0.34f,0.5f), stay=seg(t,0.8f,1); f.faceX = -out*52 + back*16; f.gazeX = out - stay; f.openL=f.openR = 0.6f + out*0.4f; break; }
    case A_PEEK_RIGHT: { float out=seg(t,0,0.28f), back=seg(t,0.34f,0.5f), stay=seg(t,0.8f,1); f.faceX = out*52 - back*16; f.gazeX = -out + stay; f.openL=f.openR = 0.6f + out*0.4f; break; }
    case A_PEEK_TOP: { float out=seg(t,0,0.28f), back=seg(t,0.34f,0.5f); f.faceY = -out*34 + back*12; f.gazeY=out; f.openL=f.openR = 0.6f + out*0.4f; break; }
    case A_PEEK_BOTTOM: { float out=seg(t,0,0.28f), back=seg(t,0.34f,0.5f); f.faceY = out*30 - back*10; f.gazeY=-out; f.openL=f.openR = 0.6f + out*0.4f; break; }
    case A_FOLLOW_DOT: { float ang=t*6.28318f*2.0f; float dotX=64+cosf(ang)*46, dotY=30+sinf(ang*1.7f)*18; f.gazeX=(dotX-64)/64.0f; f.gazeY=(dotY-30)/30.0f; f.faceX=f.gazeX*10; f.faceY=f.gazeY*6; break; }
    case A_HAPPY: f.faceY=-fabsf(osc(t,3))*8; f.openL=f.openR=0.6f+0.4f*fabsf(osc(t,3)); f.radius=10; f.browL=f.browR=-2; f.wiggle=1.0f; break;
    case A_SAD: f.faceY=p*14; f.openL=f.openR=0.55f; f.gazeY=0.5f; f.browL=f.browR=4; f.radius=4; break;
    case A_ANGRY: f.faceX=osc(t,7)*3*(1-t); f.openL=f.openR=0.7f; f.browL=f.browR=-5; f.radius=3; f.gazeY=0.2f; break;
    case A_SURPRISED: f.scaleX=f.scaleY=1+pp*0.45f; f.faceY=-pp*6; f.radius=11; f.openL=f.openR=1; break;
    case A_SLEEPY: f.faceY=p*8+sinf(gTick*0.1f)*1.5f; f.openL=f.openR=0.25f+0.1f*osc(t,1); f.gazeY=0.6f; f.radius=8; break;
    case A_SCARED: { float run=seg(t,0,0.3f); f.faceX=run*38+(random(3)-1); f.faceY=run*14+(random(3)-1); f.scaleX=f.scaleY=1+run*0.25f; f.radius=11; f.browL=f.browR=5; f.gazeY=-0.3f; break; }
    case A_DISGUSTED: f.faceX=-p*8; f.gazeX=-0.4f; f.openL=0.5f; f.openR=0.85f; f.browL=-3; f.browR=2; break;
    case A_LOVE: f.pupil=1; f.radius=10; f.faceY=osc(t,2)*3; f.wiggle=0.8f; break;
    case A_COOL: f.faceX=p*10-pp*10; f.openL=f.openR=0.5f; f.radius=3; f.browL=f.browR=-2; break;
    case A_CONFUSED: f.gazeX=osc(t,2)*0.4f; f.openL=0.9f; f.openR=0.6f; f.browL=3; f.browR=-3; f.wiggle=0.6f; break;
    case A_LAUGH: f.faceY=-fabsf(osc(t,5))*6; f.faceX=osc(t,5)*2; f.openL=f.openR=0.35f+0.35f*fabsf(osc(t,5)); f.radius=9; break;
    case A_CRY: f.faceY=p*6; f.openL=f.openR=0.55f; f.browL=f.browR=4; f.gazeY=0.4f; break;
    case A_DIZZY: { float a=t*6.28318f*2; f.faceX=cosf(a)*24; f.faceY=sinf(a)*10; f.pupil=4; break; }
    case A_SPIN: { float a=t*6.28318f; f.faceX=cosf(a)*34; f.faceY=sinf(a)*16; f.gazeX=cosf(a+1.57f)*0.5f; f.gazeY=sinf(a+1.57f)*0.5f; break; }
    case A_JUMP: { float crouch=seg(t,0,0.2f), rise=seg(t,0.2f,0.5f), fall=seg(t,0.5f,0.8f), land=seg(t,0.8f,1); if(t<0.2f){ f.scaleY=1-crouch*0.3f; f.scaleX=1+crouch*0.25f; f.faceY=crouch*5; } else if(t<0.5f){ f.faceY=5-rise*30; f.scaleY=1+rise*0.25f; f.scaleX=1-rise*0.15f; } else if(t<0.8f){ f.faceY=-25+fall*30; f.scaleY=1+(1-fall)*0.25f; } else { f.scaleY=1-land*0.25f*(1-land)*4; f.scaleX=1+land*0.2f*(1-land)*4; } break; }
    case A_FALL: { float d1=seg(t,0,0.4f), b1=seg(t,0.4f,0.6f), d2=seg(t,0.6f,0.8f); if(t<0.4f) f.faceY=-40+d1*d1*45; else if(t<0.6f) f.faceY=5-b1*14; else if(t<0.8f) f.faceY=-9+d2*d2*14; else f.faceY=5; f.scaleY=(t<0.4f)?1:((t<0.45f)?0.7f:1); break; }
    case A_SHAKE_NO: f.faceX=osc(t,4)*10*(1-t); break;
    case A_NOD_YES:  f.faceY=osc(t,3)*8; f.openL=f.openR=0.85f; break;
    case A_ZOOM_IN:  f.scaleX=f.scaleY=0.4f+p*1.4f; f.openL=f.openR=p; break;
    case A_ZOOM_OUT: f.scaleX=f.scaleY=1.8f-p*1.4f; f.openL=f.openR=1-p; break;
    case A_FADE_GHOST: f.openL=f.openR=0.3f+0.7f*fabsf(osc(t,2)); f.faceY=osc(t,1)*6; break;
    case A_GLITCH: f.faceX=(random(13)-6); f.faceY=(random(7)-3); f.openL=(random(3)==0)?0.3f:1; f.openR=(random(3)==0)?0.3f:1; f.gazeX=(random(7)-3)*0.2f; break;
    case A_HEART_EYES:  f.pupil=1; f.radius=10; f.faceY=osc(t,2)*3; break;
    case A_STAR_EYES:   f.pupil=2; f.scaleX=f.scaleY=1+pp*0.15f; break;
    case A_SPIRAL_EYES: f.pupil=4; f.gazeX=osc(t,2)*0.4f; f.gazeY=osc(t,3)*0.4f; break;
    case A_X_EYES:      f.pupil=3; f.openL=f.openR=0.8f; f.faceY=p*4; break;
    case A_DOLLAR_EYES: f.pupil=5; f.scaleX=f.scaleY=1+pp*0.1f; break;
    case A_SLEEP_ZZZ:   f.faceY=p*10; f.openL=f.openR=0.12f; f.gazeY=0.6f; f.radius=8; break;
    case A_SEARCH:      f.gazeX=osc(t,2)*0.9f; f.faceX=f.gazeX*12; f.openL=f.openR=0.9f; break;
    case A_RADAR:       f.openL=f.openR=0.7f; f.radius=3; f.gazeY=osc(t,3)*0.5f; break;
    case A_WAVE_HELLO: f.faceX=osc(t,2)*8; f.wiggle=1.5f; f.openL=0.7f+0.3f*osc(t,2); f.openR=0.7f-0.3f*osc(t,2); break;
    case A_POP_IN: f.scaleX=f.scaleY=(t<0.3f)?(t/0.3f)*1.35f:(1.35f-seg(t,0.3f,0.55f)*0.35f); f.openL=f.openR=(t<0.3f)?(t/0.3f):1; break;
    case A_MELT: f.faceY=p*16; f.openL=f.openR=1-p*0.7f; f.scaleY=1+p*0.5f; f.scaleX=1-p*0.2f; f.radius=6+p*5; break;
    case A_GROW:   f.scaleX=f.scaleY=0.5f+p*1.3f; break;
    case A_SHRINK: f.scaleX=f.scaleY=1.8f-p*1.6f; break;
    case A_JELLY: f.scaleX=1+osc(t,4)*0.18f*(1-t); f.scaleY=1-osc(t,4)*0.18f*(1-t); break;
    case A_PULSE: { float beat=(fmodf(t*3,1)<0.15f)?0.15f:0; f.scaleX=f.scaleY=1+beat; break; }
    case A_FLICKER: f.openL=f.openR=(random(10)>3)?1:0.15f; break;
    case A_RAIN:  f.openL=f.openR=0.6f; f.browL=f.browR=3; f.gazeY=0.3f; f.faceY=p*4; break;
    case A_SNOW:  f.openL=f.openR=0.85f; f.gazeY=-0.2f+osc(t,1)*0.2f; f.gazeX=osc(t,0.5f)*0.4f; break;
    case A_FIREWORKS: f.scaleX=f.scaleY=1+pp*0.25f; f.pupil=2; f.radius=10; break;
    case A_BREATHE: f.scaleX=f.scaleY=1+osc(t,1)*0.05f; f.openL=f.openR=0.85f+osc(t,1)*0.15f; break;
    case A_SWING: f.faceX=osc(t,1.5f)*30*(1-t); f.gazeX=osc(t,1.5f)*0.5f*(1-t); f.radius=8; break;
    case A_DISCO: { int ph=(int)(t*8)%2; f.openL=(ph==0)?1:0.3f; f.openR=(ph==0)?0.3f:1; f.faceX=(ph==0)?-6:6; break; }
    default: break;
  }
  player.frame++;
  if(player.duration>0 && player.frame>=player.duration) play(player.next);
}

const char* animName(uint8_t a){
  static const char* names[] = {
    "Idle","Blink","Blink x2","Wink L","Wink R","Sleepy blink","Flutter",
    "Look left","Look right","Look up","Look down","Look around",
    "Peek left","Peek right","Peek top","Peek bottom","Follow dot",
    "Happy","Sad","Angry","Surprised","Sleepy",
    "Scared","Disgusted","Love","Cool","Confused",
    "Laugh","Cry","Dizzy","Spin","Jump","Fall",
    "Shake no","Nod yes","Zoom in","Zoom out","Ghost","Glitch",
    "Heart eyes","Star eyes","Spiral eyes","X eyes",
    "Dollar eyes","Sleep ZZZ","Search","Radar",
    "Wave hello","Pop in","Melt","Grow","Shrink","Jelly","Pulse",
    "Flicker","Rain","Snow","Fireworks","Breathe","Swing","Disco"
  };
  return (a<A_COUNT) ? names[a] : "?";
}

void drawPupil(int x,int y,int shape){
  switch(shape){
    case 1: display.fillCircle(x-3,y-1,3,BLACK); display.fillCircle(x+3,y-1,3,BLACK); display.fillTriangle(x-6,y+1,x+6,y+1,x,y+6,BLACK); display.fillRect(x-4,y-3,2,2,WHITE); break;
    case 2: display.drawLine(x,y-5,x,y+5,BLACK); display.drawLine(x-5,y,x+5,y,BLACK); display.drawLine(x-4,y-4,x+4,y+4,BLACK); display.drawLine(x+4,y-4,x-4,y+4,BLACK); display.drawPixel(x-2,y-2,WHITE); break;
    case 3: display.drawLine(x-4,y-4,x+4,y+4,BLACK); display.drawLine(x+4,y-4,x-4,y+4,BLACK); break;
    case 4: display.drawCircle(x,y,4,BLACK); display.drawCircle(x,y,2,BLACK); display.drawPixel(x+1,y,BLACK); display.drawPixel(x-1,y-2,WHITE); break;
    case 5: display.drawFastVLine(x,y-5,10,BLACK); display.drawFastHLine(x-3,y-4,7,BLACK); display.drawFastHLine(x-3,y,7,BLACK); display.drawFastHLine(x-3,y+4,7,BLACK); break;
    default: display.fillCircle(x,y,5,BLACK); display.fillRect(x-3,y-3,2,2,WHITE); break;
  }
}

void drawEye(int cx,int cy,float openAmt,bool isLeft){
  int ox=tx(), oy=ty();
  if(openAmt<0.03f){ int w=(int)(EYE_W*f.scaleX); display.drawFastHLine(cx-w/2+ox, cy+oy, w, WHITE); return; }
  int w=(int)(EYE_W*f.scaleX), h=(int)(EYE_H*f.scaleY*openAmt);
  int r=(int)f.radius; if(r>h/2)r=h/2; if(r>w/2)r=w/2; if(r<0)r=0;
  int x=cx-w/2+ox, y=cy-h/2+oy;
  display.fillRoundRect(x,y,w,h,r,WHITE);
  if(openAmt>0.35f) drawPupil(cx+ox+pox(), cy+oy+poy(), f.pupil);
  int brow=isLeft?f.browL:f.browR;
  if(brow!=0){
    int bx=x+3, by=y-4, bw=w-6;
    if(brow<0){ if(isLeft) display.drawLine(bx,by+brow,bx+bw,by,WHITE); else display.drawLine(bx,by,bx+bw,by+brow,WHITE); }
    else{ if(isLeft) display.drawLine(bx,by,bx+bw,by+brow,WHITE); else display.drawLine(bx,by+brow,bx+bw,by,WHITE); }
  }
}

void drawCatFace(){
  int cx=EYE_CX+tx(); int ny=EYE_CY+EYE_H/2+NOSE_GAP+ty(); int nw=4, nh=3;
  display.drawLine(cx-nw,ny,cx+nw,ny,WHITE); display.drawLine(cx-nw,ny,cx,ny+nh,WHITE); display.drawLine(cx+nw,ny,cx,ny+nh,WHITE);
  float wg=f.wiggle*sinf(gTick*0.4f)*2.0f; int wy=ny+1, lx=cx-nw-2, rx=cx+nw+2;
  display.drawLine(lx,wy, lx-16, wy-3+(int)wg, WHITE); display.drawLine(lx,wy+2, lx-18, wy+2+(int)(wg*0.5f), WHITE); display.drawLine(lx,wy+4, lx-16, wy+7+(int)wg, WHITE);
  display.drawLine(rx,wy, rx+16, wy-3-(int)wg, WHITE); display.drawLine(rx,wy+2, rx+18, wy+2-(int)(wg*0.5f), WHITE); display.drawLine(rx,wy+4, rx+16, wy+7-(int)wg, WHITE);
}

void drawExtras(uint8_t anim,float t){
  switch(anim){
    case A_FOLLOW_DOT: { float ang=t*6.28318f*2.0f; display.fillCircle(64+cosf(ang)*46, 30+sinf(ang*1.7f)*18, 2, WHITE); break; }
    case A_RADAR: { float a=t*6.28318f*3; display.drawLine(64,32,64+cosf(a)*60,32+sinf(a)*30,WHITE); display.drawCircle(64,32,20,WHITE); display.drawCircle(64,32,40,WHITE); break; }
    case A_CRY: for(int i=0;i<4;i++){ int sx=20+i*30+tx(), sy=(int)(t*80+i*20)%SH; display.drawFastVLine(sx,sy,5,WHITE); } break;
    case A_SLEEP_ZZZ: for(int i=0;i<3;i++){ int zx=EYE_CX+30+i*8+tx(), zy=EYE_CY-8-i*8-((int)(t*20)%10)+ty(); display.setTextColor(WHITE); display.setTextSize(1); display.setCursor(zx,zy); display.print(i==0?"Z":"z"); } break;
    case A_SEARCH: case A_CONFUSED: { int qx=EYE_CX+32+tx(), qy=EYE_CY-16+ty(); display.setTextColor(WHITE); display.setTextSize(1); display.setCursor(qx,qy); display.print("?"); break; }
    case A_RAIN: for(int i=0;i<14;i++){ int rx=(i*11+(int)(t*60))%SW, ry=(i*17+(int)(t*110))%SH; display.drawFastVLine(rx,ry,4,WHITE); } break;
    case A_SNOW: for(int i=0;i<16;i++){ int sx=(i*9+(int)(t*20)+i*i)%SW, sy=(i*13+(int)(t*40))%SH; display.drawPixel(sx,sy,WHITE); } break;
    case A_FIREWORKS: for(int k=0;k<2;k++){ float st=fmodf(t*2+k*0.5f,1); if(st>0.3f){ int cx=32+k*64, cy=16+k*10, r=(int)((st-0.3f)*50); for(int a=0;a<10;a++){ float ang=a*0.6283f+k; display.drawPixel(cx+cosf(ang)*r, cy+sinf(ang)*r*0.6f, WHITE); } } } break;
    case A_LOVE: case A_HEART_EYES: for(int i=0;i<5;i++){ int hx=(i*29+10)%SW, hy=SH-((int)(t*60+i*15)%SH); display.fillRect(hx,hy,2,2,WHITE); } break;
    case A_DISCO: { int ph=(int)(t*8)%2; for(int i=0;i<SH;i+=8) display.drawPixel(ph?2:SW-3, i, WHITE); break; }
    default: break;
  }
}

void drawFrame(){
  display.clearDisplay();
  float t=(player.duration>0)?(float)player.frame/player.duration :(float)(millis()%3000)/3000.0f;
  int halfSpan=(EYE_W+EYE_SP)/2;
  drawEye(EYE_CX-halfSpan, EYE_CY, f.openL, true);
  drawEye(EYE_CX+halfSpan, EYE_CY, f.openR, false);
  drawCatFace();
  drawExtras(player.cur, t);
  display.display();
}

void drawInfoScreen(int temp, int wcode){
  display.clearDisplay(); display.setTextColor(WHITE);
  struct tm tm;
  if(getLocalTime(&tm,0)){ char buf[6]; snprintf(buf,sizeof(buf),"%02d:%02d",tm.tm_hour,tm.tm_min); display.setTextSize(2); display.setCursor((SW-60)/2,2); display.print(buf); }
  else { display.setTextSize(1); display.setCursor((SW-48)/2, 6); display.print("-- : --"); }
  display.drawFastHLine(4, 22, SW-8, WHITE);
  display.setTextSize(1);
  if(weatherDataValid){
    char tb[10]; snprintf(tb, sizeof(tb), "%d C", temp);
    display.setCursor((SW-strlen(tb)*6)/2, 26); display.print(tb);
    int ix = SW/2, iy = 42;
    if(wcode >= 51){ for(int i=0;i<5;i++) display.drawLine(ix-10+i*5, iy-3, ix-12+i*5, iy+3, WHITE); }
    else if(wcode >= 0){
      display.drawCircle(ix, iy, 5, WHITE);
      if(wcode == 0){ display.drawLine(ix, iy-8, ix, iy-6, WHITE); display.drawLine(ix, iy+6, ix, iy+8, WHITE); display.drawLine(ix-8, iy, ix-6, iy, WHITE); display.drawLine(ix+6, iy, ix+8, iy, WHITE); }
    }
  } else {
    display.setCursor((SW-60)/2, 28); display.print("No weather");
    display.setCursor((SW-78)/2, 38); display.print("data yet");
  }
  if(WiFi.status() == WL_CONNECTED){
    String ip = WiFi.localIP().toString();
    display.setCursor((SW - ip.length()*6)/2, 56); display.print(ip);
  } else if(WiFi.getMode() & WIFI_AP){
    String ip = WiFi.softAPIP().toString();
    display.setCursor(4, 56); display.print("AP:"); display.print(ip);
  }
  display.display();
}

void drawApInfoScreen(){
  display.clearDisplay(); display.setTextColor(WHITE);
  display.setTextSize(1); display.setCursor(4, 2); display.print("Connect to WiFi:");
  display.setCursor(4, 14); display.print("AP: "); display.print(cfg.apName);
  display.setCursor(4, 24); display.print("PW: "); display.print(cfg.apPassword);
  display.setCursor(4, 36); display.print("URL:");
  display.setCursor(4, 46); display.print(WiFi.softAPIP().toString());
  display.drawFastHLine(0, 56, SW, WHITE);
  display.setCursor(8, 58); display.print("tap TOP to exit");
  display.display();
}

void playTopAnim(){
  static const uint8_t anims[] = { A_LOVE, A_HEART_EYES, A_WINK_L, A_WINK_R, A_SURPRISED, A_HAPPY, A_BLINK_DOUBLE, A_FLUTTER };
  playThen(anims[random(sizeof(anims))], A_IDLE);
}
void playLeftAnim(){
  static const uint8_t anims[] = { A_LOOK_RIGHT, A_PEEK_RIGHT, A_DISGUSTED, A_SCARED, A_ANGRY, A_SHAKE_NO, A_COOL, A_CONFUSED };
  playThen(anims[random(sizeof(anims))], A_IDLE);
}
void playRightAnim(){
  static const uint8_t anims[] = { A_LOOK_LEFT, A_PEEK_LEFT, A_DISGUSTED, A_SCARED, A_ANGRY, A_SHAKE_NO, A_COOL, A_CONFUSED };
  playThen(anims[random(sizeof(anims))], A_IDLE);
}