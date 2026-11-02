#include "ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
Color bg={13,21,30,255},panel={23,34,46,255},muted={145,163,181,255},accent={65,222,177,255};
bool uiBlocked=false;
static int pixelScale(int size){return size<13?1:size<25?2:size<35?3:size<50?4:6;}
// 글자 간격: 지금과 같은 1 px(2배), 큰 글씨는 글꼴 1픽셀만큼 / letter gap: 1 px at 2x (as before), one font pixel at larger scales
static float gapFor(int s){return s<=2?1.0f:(float)s;}
void UiText(const char *t,int x,int y,int size,Color c){int s=pixelScale(size),h=10*s;
 DrawTextEx(GetFontDefault(),t,(Vector2){(float)x,(float)(y+(size-h)/2)},(float)h,gapFor(s),c);}
int UiMeasure(const char *t,int size){int s=pixelScale(size);return (int)MeasureTextEx(GetFontDefault(),t,(float)(10*s),gapFor(s)).x;}
bool button(int x,int y,int w,const char *t,bool active){Rectangle r={(float)x,(float)y,(float)w,34};bool hover=!uiBlocked&&CheckCollisionPointRec(GetMousePosition(),r);DrawRectangleRounded(r,.18,6,active?accent:hover?(Color){62,81,98,255}:(Color){39,54,69,255});UiText(t,x+10,y+10,14,active?bg:RAYWHITE);return hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);}
void label(int x,int y,const char *t){UiText(t,x,y,16,muted);}
void metric(int x,int y,const char *name,const char *value){DrawRectangleRounded((Rectangle){x,y,245,80},.1,6,panel);label(x+16,y+12,name);UiText(value,x+16,y+37,28,RAYWHITE);}
void bar(int x,int y,int w,double value){DrawRectangle(x,y,w,8,panel);DrawRectangle(x,y,(int)(w*fmax(0,fmin(1,value))),8,accent);}
// 금액 표시: "EUR 1,234" 형식(정수) 또는 소수 둘째 자리. / Money text: "EUR 1,234" (whole) or with cents.
static const char *money(double v,bool cents){static char buf[8][64];static int k;char *b=buf[k=(k+1)%8];
 bool neg=v<0;if(neg)v=-v;long long c=(long long)floor(v*100+.5),w=cents?c/100:(c+50)/100;neg=neg&&(cents?c>0:w>0);char d[24];snprintf(d,sizeof d,"%lld",w);
 char g[32];int n=(int)strlen(d),j=0;for(int i=0;i<n;i++){g[j++]=d[i];if((n-i-1)%3==0&&i<n-1)g[j++]=',';}g[j]=0;
 if(cents)snprintf(b,64,"%sEUR %s.%02d",neg?"-":"",g,(int)(c%100));else snprintf(b,64,"%sEUR %s",neg?"-":"",g);return b;}
const char *eur(double v){return money(v,false);}
const char *eur2(double v){return money(v,true);}
const char *mmss(double t){static char b[4][16];static int k;char *c=b[k=(k+1)%4];snprintf(c,16,"%d:%02d",(int)t/60,(int)t%60);return c;}
bool shop(int x,int y,int w,const char *t,double cost,double cash,const char *doneText){
 Rectangle r={(float)x,(float)y,(float)w,34};
 if(cost<0){DrawRectangleRounded(r,.18,6,(Color){30,44,57,255});UiText(doneText?doneText:t,x+10,y+10,14,accent);return false;}
 bool ok=cash+1e-9>=cost,hover=!uiBlocked&&CheckCollisionPointRec(GetMousePosition(),r);
 DrawRectangleRounded(r,.18,6,!ok?(Color){30,44,57,255}:hover?(Color){62,81,98,255}:(Color){39,54,69,255});
 UiText(TextFormat("%s  %.0f",t,cost),x+10,y+10,14,ok?RAYWHITE:muted);
 return ok&&hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT);}
int UiEndScreen(int W,int H,float time,double goal,double t,const UiRow *rows,int n,bool saved){
 int pressed=END_NONE;bool was=uiBlocked;uiBlocked=false;
 DrawRectangle(0,0,W,H,(Color){5,9,14,200});
 // 색종이 효과 / confetti
 Color conf[4]={accent,ORANGE,RAYWHITE,(Color){120,170,255,255}};
 for(int k=0;k<90;k++){float sp=70+(k%5)*22;int cx=(int)fmodf(k*137.5f+sinf(time*1.3f+k)*30,(float)W),cy=(int)fmodf(time*sp+k*97,(float)H+40)-20;
  DrawRectangle(cx,cy,5+(k%3)*2,9-(k%3)*2,conf[k%4]);}
 int px=380,py=170,pw=680,ph=560;
 DrawRectangleRounded((Rectangle){px,py,pw,ph},.04f,8,panel);DrawRectangle(px+24,py,pw-48,4,accent);
 UiText("CONTRACT COMPLETE",px+40,py+36,40,accent);
 label(px+42,py+88,TextFormat("You earned %s in sales for the customer.",eur(goal)));
 // 시간에 따른 메달 / medal by completion time
 const char *medal=t<=540?"GOLD":t<=720?"SILVER":"BRONZE";
 Color mc=t<=540?(Color){232,190,60,255}:t<=720?(Color){196,204,212,255}:(Color){196,124,70,255};
 int mx=px+pw-110,my=py+190;
 DrawRectangle(mx-26,my-90,18,60,(Color){60,90,180,255});DrawRectangle(mx+8,my-90,18,60,(Color){180,60,60,255});
 DrawCircle(mx,my,52,mc);DrawCircle(mx,my,40,(Color){(unsigned char)(mc.r*.8f),(unsigned char)(mc.g*.8f),(unsigned char)(mc.b*.8f),255});
 UiText(medal,mx-UiMeasure(medal,18)/2,my-9,18,bg);
 const char *cap=t<=540?"Gold: under 9:00":t<=720?"Silver: under 12:00":"Bronze: over 12:00";label(mx-UiMeasure(cap,16)/2,my+64,cap);
 // 결과 표 / results table
 for(int k=0;k<n&&k<9;k++){label(px+42,py+140+k*34,rows[k].name);UiText(rows[k].value,px+290,py+138+k*34,20,RAYWHITE);}
 label(px+42,py+454,"Production is paused. Resume to keep running this factory.");
 if(button(px+40,py+494,190,"Resume",true))pressed=END_RESUME;
 if(button(px+245,py+494,160,"New game",false))pressed=END_NEW_GAME;
 if(button(px+420,py+494,220,"Export CSV report",false))pressed=END_EXPORT;
 if(saved)label(px+420,py+534,"Saved to the game folder");
 uiBlocked=was;return pressed;
}
