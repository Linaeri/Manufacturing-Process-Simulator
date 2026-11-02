#ifndef UI_H
#define UI_H
#include "raylib.h"
#include <stdbool.h>
// 두 공장 대시보드가 함께 쓰는 색상과 UI 도우미. / Colours and UI helpers shared by both factory dashboards.
extern Color bg, panel, muted, accent;
// 종료 화면이 열려 있으면 아래 버튼은 클릭을 무시합니다. / While the end screen is open, buttons underneath ignore clicks.
extern bool uiBlocked;
// 픽셀 글꼴은 10 px 격자라 정수 배율(10, 20, 30 …)에서만 고르게 보입니다. 요청한 크기를 가장 가까운 정수 배율로 그립니다.
// The pixel font sits on a 10 px grid, so it only looks even at whole-number scales (10, 20, 30 …).
// Requested sizes are drawn at the nearest whole-number scale, vertically centred where the old size was.
void UiText(const char *t,int x,int y,int size,Color c);
int UiMeasure(const char *t,int size);
bool button(int x,int y,int w,const char *t,bool active);
void label(int x,int y,const char *t);
void metric(int x,int y,const char *name,const char *value);
void bar(int x,int y,int w,double value);
// 구매 버튼: 가격 표시, 돈이 부족하면 흐리게, cost<0이면 doneText. / Shop button: price, greyed when unaffordable, doneText when cost<0.
bool shop(int x,int y,int w,const char *t,double cost,double cash,const char *doneText);
const char *eur(double v);    // "EUR 1,234"
const char *eur2(double v);   // "EUR 1,234.56"
const char *mmss(double t);   // "8:05"
// 계약 완료 화면의 결과 한 줄. / One result row on the contract-complete screen.
typedef struct { const char *name; char value[48]; } UiRow;
enum { END_NONE, END_RESUME, END_NEW_GAME, END_EXPORT };
// 계약 완료 화면(색종이, 메달, 결과표, 버튼). 누른 버튼을 돌려줍니다.
// Contract-complete screen (confetti, medal, results, buttons); returns the button pressed.
int UiEndScreen(int W,int H,float t,double goal,double contractTime,const UiRow *rows,int n,bool saved);
#endif
