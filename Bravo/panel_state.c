#include "panel_state.h"
#include <math.h>
#include <stdio.h>
#include <limits.h>
// 초기화: 패널 기본값만 복원합니다.
// Initialization restores panel defaults only.
void PanelInit(PanelState *s) {
 *s=(PanelState){.stock=150,.supplied=150,.target=500,.speed=1,.rng=42};
 for(int i=0;i<MACHINES;i++) s->machines[i]=(Machine){.enabled=true,.speed=1};
}
void PanelRestock(PanelState *s) { if(s->supplied<=INT_MAX-100){s->stock+=100;s->supplied+=100;} }
void PanelMaintain(PanelState *s,int id){if(id>=0&&id<MACHINES)s->machines[id].wear=0;}
static double duration(const PanelState *s){return s->process?3.0:2.0;}
// 설정 기반 예상치이며 실제 생산을 수행하지 않습니다.
// Configuration-based estimate; this does not produce any items.
double PanelRate(const PanelState *s){double r=0;for(int i=0;i<MACHINES;i++)if(s->machines[i].enabled&&s->machines[i].wear<200)r+=60*s->machines[i].speed*s->speed/duration(s);return r;}
double PanelCost(const PanelState *s){return (s->supplied-s->stock)*2.5+s->energy*.22;}
double PanelRevenue(const PanelState *s){return s->good*(s->process?9.0:7.5);}
// 생산 데이터만 갱신하며 3D 장면이나 카메라는 사용하지 않습니다.
// Update production data without a 3D scene or camera.
void PanelUpdate(PanelState *s,double dt){
 if(!s->running||!isfinite(dt)||dt<=0)return;
 // 작은 시간 단위로 나누어 기계 간 자원 소비를 균등하게 처리합니다.
 // Small time slices keep resource sharing fair between machines.
 while(dt>1e-9){double step=fmin(dt,.02);dt-=step;s->elapsed+=step*s->speed;
 for(int i=0;i<MACHINES;i++){
 Machine *m=&s->machines[i];if(!m->enabled||m->wear>=200)continue;
 if(!m->busy){if(s->stock==0)continue;s->stock--;m->busy=true;}
 double work=step*s->speed*m->speed;
 s->energy+=step*s->speed*(4+4*m->speed*m->speed)/3600;
 m->progress+=work/duration(s);
 if(m->progress>=1-1e-9){
 m->progress=fmax(0,m->progress-1);m->busy=false;m->cycles++;m->wear++;
 s->rng=s->rng*1664525u+1013904223u;
 double risk=.025+fmax(0,m->speed-1)*.15+m->wear*.0003;
 bool bad=(s->rng%10000)<risk*10000;
 s->total++;if(bad){s->defects++;m->defects++;}else{s->good++;m->good++;}
 s->records[s->nextRecord]=(Product){s->total,i,s->elapsed,bad};s->nextRecord=(s->nextRecord+1)%RECORDS;if(s->recordCount<RECORDS)s->recordCount++;
 }
 }
 }
}
int PanelAlerts(const PanelState *s,const char **out,int cap){int n=0;
#define ALERT(x) do{if(n<cap)out[n]=(x);n++;}while(0)
 if(s->stock==0)ALERT("OUT OF MATERIALS - Restock to resume new jobs");
 else if(s->stock<=30)ALERT("LOW MATERIAL STOCK - 30 sheets or fewer");
 bool due=false,stopped=false,energy=false;for(int i=0;i<MACHINES;i++){due|=s->machines[i].wear>=150;stopped|=!s->machines[i].enabled||s->machines[i].wear>=200;energy|=s->machines[i].speed>1.5;}
 if(due)ALERT("MAINTENANCE REQUIRED - Service worn machines");
 if(stopped)ALERT("MACHINE STOPPED - Check Controls tab");
 if(s->total>=10&&(double)s->defects/s->total>.12)ALERT("HIGH DEFECT RATE - Reduce machine speed");
 if(energy&&s->running)ALERT("HIGH ENERGY USE - High speed operation");
 return n;
#undef ALERT
}
// 현재 패널 데이터를 저장합니다. 파일 열기/닫기 실패를 호출자에게 알립니다.
// Save current panel data and report file open/close failures to the caller.
bool PanelExport(const PanelState *s,const char *path){FILE *f=fopen(path,"w");if(!f)return false;
 fprintf(f,"metric,value\nprocess,%s\ntotal,%d\ngood,%d\ndefects,%d\nstock,%d\ntarget,%d\nsimulated_seconds,%.3f\nenergy_kWh,%.6f\ncost_EUR,%.2f\nrevenue_EUR,%.2f\nprofit_EUR,%.2f\n\nproduct_id,machine,simulated_seconds,result\n",s->process?"Forming":"Stamping",s->total,s->good,s->defects,s->stock,s->target,s->elapsed,s->energy,PanelCost(s),PanelRevenue(s),PanelRevenue(s)-PanelCost(s));
 for(int j=0;j<s->recordCount;j++){int k=(s->nextRecord-s->recordCount+j+RECORDS)%RECORDS;Product p=s->records[k];fprintf(f,"%d,%d,%.3f,%s\n",p.id,p.machine+1,p.time,p.defective?"defect":"good");}
 return fclose(f)==0;
}
