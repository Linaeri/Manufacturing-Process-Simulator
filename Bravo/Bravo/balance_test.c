// 그래픽 없이 게임을 자동으로 플레이하여 진행 속도를 측정합니다.
// Plays the game automatically without graphics and reports progression pacing.
// Run:   ./balance_test [seconds between player actions, default 4] [metal|saw]
// Build: gcc -std=c99 -O2 balance_test.c panel_state.c sawmill_state.c -lm -o balance_test
#include "panel_state.h"
#include "sawmill_state.h"
#include <stdio.h>
#include <stdlib.h>

static const char *NAMES[UP_COUNT]={"Speed M1","Speed M2","Speed M3","Unlock machine","Quality","Auto-buyer","Auto-service",""};
static void stamp(double t){printf("%2d:%02d  ",(int)t/60,(int)t%60);}


// ---- 제재소: 숲, 하베스터, 제재 라인 / Sawmill: forest, harvester, mill line ----
static const char *SAW_NAMES[SU_COUNT]={"Harvester speed","Band saw speed","Planer speed","Moulder speed","Mill machine","Land plot"};
static int sawMain(double delay){
 SawState s;SawInit(&s);s.running=true;s.courier=true; // 포워더 흉내 / imitate the forwarder
 double nextAct=0,nextReport=60,idleAtReport=0;int purchases=0;
 printf("Factory: SAWMILL\nPlayer acts every %.1f s. Contract goal EUR %.0f.\n\n",delay,SAW_CONTRACT_GOAL);
 while(s.elapsed<30*60&&!s.contractDone){
  SawUpdate(&s,1.0/60);
  if(s.elapsed>=nextAct){nextAct+=delay;
   // 핵심 해금: 다음 제재 기계, 그리고 나무가 부족할 때 땅. 1분 안에 살 수 있으면 돈을 모읍니다.
   // Key unlocks: the next mill machine, and land when trees run short. Save for them if within about a minute.
   bool shortTrees=s.starveEma>.15||SawHarvestPerMinute(&s)>SawRegrowPerMinute(&s);
   for(;;){int best=-1,key=-1;double bc=0,kc=0;
    for(int u=0;u<SU_COUNT;u++){double c=SawUpgradeCost(&s,(SawUpgrade)u);if(c<0)continue;
     if(u==SU_LAND&&!shortTrees)continue;                       // 필요할 때만 땅 / land only when needed
     if(u==SU_HARVESTER&&shortTrees)continue;                   // 나무가 부족하면 하베스터 속도는 소용없음 / no point speeding up a starved harvester
     if(best<0||c<bc){best=u;bc=c;}
     if((u==SU_MILL||u==SU_LAND)&&(key<0||c<kc)){key=u;kc=c;}}
    if(key>=0&&kc<=s.money+SawIncomePerMinute(&s)){best=key;bc=kc;}
    if(best<0||s.money<bc)break;
    SawBuyUpgrade(&s,(SawUpgrade)best);purchases++;
    stamp(s.elapsed);printf("BUY  %-16s EUR %6.0f   (level %d)\n",SAW_NAMES[best],bc,s.level[best]);
    shortTrees=s.starveEma>.15||SawHarvestPerMinute(&s)>SawRegrowPerMinute(&s);
   }
  }
  if(s.elapsed>=nextReport){nextReport+=60;stamp(s.elapsed);
   printf("---- money %6.0f | income %5.0f/min | sales %6.0f | felling %4.1f vs regrow %4.1f trees/min | waited for trees %3.0f%% | yard %2d | surplus %d\n",
    s.money,SawIncomePerMinute(&s),s.earned,SawHarvestPerMinute(&s),SawRegrowPerMinute(&s),100*(s.harvIdle-idleAtReport)/60,s.yard,s.surplusRaw);
   idleAtReport=s.harvIdle;}
 }
 printf("\n");
 if(s.contractDone){stamp(s.contractTime);printf("CONTRACT COMPLETE\n");}else printf("Contract NOT complete after 30 min\n");
 printf("Purchases %d | trees %d | logs %d | harvester waited for trees %.1f%% of the game | surplus logs sold raw %d | scrapped %d\n",
  purchases,s.treesFelled,s.logsCut,100*s.harvIdle/s.elapsed,s.surplusRaw,s.scrapTotal);
 return 0;
}
int main(int argc,char **argv){
 double delay=argc>1?atof(argv[1]):4.0;if(delay<=0)delay=4.0;
 if(argc>2&&argv[2][0]=='s')return sawMain(delay);
 const SimConfig *cfg=&METAL_CONFIG;
 NAMES[UP_PROCESS]=cfg->processUpgradeLong;
 PanelState s;PanelInit(&s,cfg);s.running=true;s.courier=true; // 작업자 배달 흉내 / imitate the worker's deliveries
 double nextAct=0,nextReport=60;int purchases=0;
 printf("Player acts every %.1f s. Contract goal EUR %.0f.\n\n",delay,cfg->contractGoal);
 while(s.elapsed<30*60&&!s.contractDone){
  PanelUpdate(&s,1.0/60);
  if(s.elapsed>=nextAct){nextAct+=delay;
   // 사람처럼 행동: 재고 보충, 정비, 그다음 가장 싼 업그레이드 구매.
   // Act like a player: restock, service, then buy the cheapest upgrade available.
   if(!s.level[UP_AUTOBUY]&&s.stock+PanelPalletTotal(&s)<8){int n=(int)(s.money/cfg->materialPrice);if(n>20)n=20;PanelBuyMaterial(&s,n);}
   if(!s.level[UP_AUTOSERVICE])for(int i=0;i<s.owned;i++)if(s.machines[i].wear>=cfg->wearWarn)PanelMaintain(&s,i);
   double reserve=s.level[UP_AUTOBUY]?0:25;
   // 큰 해금(기계, 자동 구매, 성형)은 1분 안에 살 수 있으면 돈을 모읍니다.
   // Save for a key unlock (machine, auto-buyer, forming) when it's within about a minute of income.
   for(;;){int best=-1,key=-1;double bc=0,kc=0;
    for(int u=0;u<UP_COUNT;u++){double c=PanelUpgradeCost(&s,(Upgrade)u);if(c<0)continue;
     if(best<0||c<bc){best=u;bc=c;}
     if((u==UP_MACHINE||u==UP_AUTOBUY||u==UP_PROCESS)&&(key<0||c<kc)){key=u;kc=c;}}
    if(key>=0&&kc<=s.money+PanelIncomePerMinute(&s)){best=key;bc=kc;}
    if(best<0||s.money-reserve<bc)break;
    PanelBuyUpgrade(&s,(Upgrade)best);purchases++;
    stamp(s.elapsed);printf("BUY  %-15s EUR %6.0f   (level %d)\n",NAMES[best],bc,s.level[best]);
   }
  }
  if(s.elapsed>=nextReport){nextReport+=60;stamp(s.elapsed);
   printf("---- money %7.0f | income %6.0f/min | sales %6.0f | defects (last 20) %4.1f%% | stock %d\n",
    s.money,PanelIncomePerMinute(&s),s.earned,100*PanelRecentDefectRate(&s,20),s.stock);}
 }
 printf("\n");
 if(s.contractDone){stamp(s.contractTime);printf("CONTRACT COMPLETE\n");}else printf("Contract NOT complete after 30 min\n");
 printf("Purchases %d | parts %d | defects %d (%.1f%%) | supplier credits %d\n",purchases,s.total,s.defects,s.total?100.0*s.defects/s.total:0,s.credits);
 return 0;
}
