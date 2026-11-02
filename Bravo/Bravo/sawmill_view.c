#include "sawmill_view.h"
#include <math.h>
#include "ui.h"
#include <stdio.h>

// ---- 현장 배치(미터) / Site layout (metres) ----
#define SS 2                       // 2배 해상도 / supersampling
static const float ROW_TREES[2]={-18.5f,-33.5f};   // 구획 줄의 나무 선 / tree line of each plot row
static const float ROW_FRONT[2]={-16.0f,-31.0f};   // 도로 쪽 가장자리 / road-side edge
static const float ROW_BACK[2]={-21.0f,-36.0f};
static const float ROAD_Z[2]={-12.5f,-27.5f};      // 숲 도로(폭 6 m) / forestry roads, 6 m wide
static const float PILE_Z[2]={-11.5f,-26.5f};      // 두 차선 사이 길 위의 통나무 더미 / log piles on the road between the two lanes
#define HARV_LANE (-1.4f)          // 하베스터는 숲 쪽 차선 / harvester uses the forest-side lane
#define FWD_LANE 3.5f              // 포워더는 먼 쪽 차선: 돌아도 쓰러지는 나무에 닿지 않음 / forwarder uses the far lane: even turning, it can't reach falling trees
// 줄을 잇는 연결 도로: 하베스터는 항상 서쪽, 포워더는 항상 동쪽이라 경로가 서로 교차하지 않습니다.
// Connector roads between rows: the harvester always uses the west one and the forwarder the east one, so their routes never cross.
#define HARV_SIDE_X (-27.0f)
#define FWD_SIDE_X 30.0f
#define YARD_X (-15.0f)            // 야적장과 포워더 하역 지점 / log yard and the forwarder's unloading point
#define YARD_Z (-4.0f)
#define UNLOAD_Z (-9.0f)           // 포워더 차선에서 바로 하역 / unloads straight from its lane
#define TREE_H 9.0f
// 나무는 하베스터 반대쪽(−x)으로 35° 비스듬히 쓰러져 포워더 차선에 닿지 않습니다.
// Trees fall at a 35° diagonal away from the harvester, so they never reach the forwarder's lane.
#define FALL_COS .819f
#define FALL_SIN .574f
static const float STAGE_X[MILL_STAGES]={-6.0f,3.0f,11.0f};
#define STACK_X 17.5f
#define FENCE_X 35.0f
#define FENCE_Z0 (-44.0f)
#define FENCE_Z1 18.0f
#define GATE_HW 5.0f

static Color accentC={65,222,177,255},grass={86,118,68,255},gravel={146,138,118,255},dirt={120,100,72,255};
static Color bark={98,72,50,255},leaf={46,104,54,255},leafYoung={96,150,70,255},logC={160,124,84,255},logEnd={206,172,120,255};
static Color plankC={214,186,140,255},boardC={226,204,160,255},mouldC={236,214,170,255},defectC={150,96,70,255};

static float plotX(int p){return -16.0f+(p%3)*16.0f;}
static float treeX(int p,int t){return plotX(p)-5.25f+1.5f*t;}
static int rowOf(int p){return p/3;}
// 쓰러지는 x 방향: 서쪽 열은 +x(연결 도로를 피함), 나머지는 −x. 하베스터는 반대편에 섭니다.
// Fall direction in x: the west column falls towards +x (clear of the connector road), the others towards -x; the harvester stands on the other side.
static float fallSign(int p){return p%3==0?1.0f:-1.0f;}

// ---- 나무 / Trees ----
// 밑동에서 dir 방향으로 자라는 나무. canopy 0이면 가지를 모두 친 줄기. / A tree from base along dir; canopy 0 = fully stripped trunk.
static void drawTree(Vector3 b,Vector3 d,float h,float r,float canopy,Color fol){
 Vector3 top=v3(b.x+d.x*h,b.y+d.y*h,b.z+d.z*h);
 DrawCylinderEx(b,top,r,r*.45f,8,bark);
 if(canopy>.02f){float R=1.35f*h/TREE_H*canopy;
  Vector3 a=v3(b.x+d.x*h*.32f,b.y+d.y*h*.32f,b.z+d.z*h*.32f),m=v3(b.x+d.x*h*.62f,b.y+d.y*h*.62f,b.z+d.z*h*.62f);
  DrawCylinderEx(a,m,R,R*.35f,10,fol);DrawCylinderEx(m,top,R*.8f,0,10,shade(fol,1.1f));}
}
static void drawLog(Vector3 a,Vector3 b,float r){DrawCylinderEx(a,b,r,r,10,logC);DrawCylinderEx(a,v3(a.x+(b.x-a.x)*.01f,a.y+(b.y-a.y)*.01f,a.z+(b.z-a.z)*.01f),r*1.01f,r*1.01f,10,logEnd);}
// 통나무 더미(x 방향으로 누움). / A pile of logs lying along x.
static void logPile(float cx,float cz,int n,float len){
 for(int i=0;i<n&&i<32;i++){int layer=i/4,k=i%4;float y=.24f+layer*.42f,z=cz-.66f+k*.44f+(layer%2)*.22f; // 4개씩 쌓아 두 차선 사이에 맞춤 / 4 per layer, fits between the two lanes
  drawLog(v3(cx-len/2,y,z),v3(cx+len/2,y,z),.22f);}
}

// ---- 지형, 울타리, 표지판 / Terrain, fence and signs ----
static unsigned hashU(unsigned x){x^=x>>16;x*=0x45d9f3bu;x^=x>>16;x*=0x45d9f3bu;x^=x>>16;return x;}
static void drawSurroundings(void){
 box(v3(0,-.06f,-15),v3(260,.1f,260),grass);
 // 멀리 있는 언덕 / distant hills
 for(int i=0;i<16;i++){float a=i*PI/8+.2f,r=118+(i%3)*8,h=10+(i%4)*5;box(v3(cosf(a)*r,h/2-1,-15+sinf(a)*r),v3(40,h,40),(Color){(unsigned char)(96+(i%3)*8),126,116,255});}
 // 울타리 밖 숲(카메라 경계 밖) / forest beyond the fence, outside the camera's bounds
 for(unsigned i=0;i<420;i++){unsigned hsh=hashU(i*7919u+13);float x=-80+(hsh%1600)/10.0f,z=-95+((hsh>>11)%1500)/10.0f;
  if(x>-41&&x<41&&z>-49&&z<37)continue;
  float h=8+(hsh>>21)%7;drawTree(v3(x,0,z),v3(0,1,0),h,.25f,1,shade(leaf,.85f+((hsh>>5)%30)/100.0f));}
 // 자갈 작업장과 숲 도로 / gravel mill pad and forestry roads
 box(v3(0,.004f,3),v3(66,.012f,26),gravel);
 for(int r=0;r<2;r++)box(v3(1.5f,.008f,ROAD_Z[r]+1),v3(66,.014f,8),dirt);  // 두 차선을 덮는 도로 / roads covering both lanes
 box(v3(HARV_SIDE_X,.008f,-20.5f),v3(6,.014f,20),dirt);box(v3(FWD_SIDE_X,.008f,-17.5f),v3(6,.014f,22),dirt);
}
static void fence(void){
 Color post={120,126,132,255},mesh={168,174,180,255},rail={140,146,152,255};
 float xs[2]={-FENCE_X,FENCE_X},zs[2]={FENCE_Z0,FENCE_Z1};
 // 옆면 / sides
 for(int s=0;s<2;s++){float x=xs[s];
  for(float z=FENCE_Z0;z<=FENCE_Z1+.01f;z+=3)box(v3(x,1.2f,z),v3(.1f,2.4f,.1f),post);
  box(v3(x,2.35f,(FENCE_Z0+FENCE_Z1)/2),v3(.06f,.06f,FENCE_Z1-FENCE_Z0),rail);
  for(float z=FENCE_Z0;z<=FENCE_Z1;z+=.6f)DrawLine3D(v3(x,.05f,z),v3(x,2.3f,z),mesh);
  for(float y=.5f;y<2.3f;y+=.6f)DrawLine3D(v3(x,y,FENCE_Z0),v3(x,y,FENCE_Z1),mesh);}
 // 뒷면과 앞면(앞면에는 문) / back and front (the front has the gate)
 for(int s=0;s<2;s++){float z=zs[s];
  for(float x=-FENCE_X;x<=FENCE_X+.01f;x+=3){if(s==1&&fabsf(x)<GATE_HW)continue;box(v3(x,1.2f,z),v3(.1f,2.4f,.1f),post);}
  for(int half=0;half<2;half++){float x0=half?(s?GATE_HW:-FENCE_X):-FENCE_X,x1=half?FENCE_X:(s?-GATE_HW:FENCE_X);if(!s&&half)continue;
   box(v3((x0+x1)/2,2.35f,z),v3(x1-x0,.06f,.06f),rail);
   for(float x=x0;x<=x1;x+=.6f)DrawLine3D(v3(x,.05f,z),v3(x,2.3f,z),mesh);
   for(float y=.5f;y<2.3f;y+=.6f)DrawLine3D(v3(x0,y,z),v3(x1,y,z),mesh);}}
 // 문: 노랑·검정 기둥과 닫힌 철망 문 / gate: yellow-and-black posts and closed mesh leaves
 for(int s=-1;s<=1;s+=2){for(float y=.3f;y<2.8f;y+=.6f)box(v3(s*GATE_HW,y,FENCE_Z1),v3(.22f,.3f,.22f),(Color){232,190,40,255}),box(v3(s*GATE_HW,y+.3f,FENCE_Z1),v3(.22f,.3f,.22f),(Color){30,30,32,255});}
 box(v3(0,2.35f,FENCE_Z1),v3(2*GATE_HW,.08f,.08f),(Color){232,190,40,255});box(v3(0,.15f,FENCE_Z1),v3(2*GATE_HW,.08f,.08f),(Color){232,190,40,255});
 for(float x=-GATE_HW;x<=GATE_HW;x+=.5f)DrawLine3D(v3(x,.15f,FENCE_Z1),v3(x,2.35f,FENCE_Z1),(Color){200,190,140,255});
 DrawLine3D(v3(0,.15f,FENCE_Z1),v3(0,2.35f,FENCE_Z1),(Color){30,30,32,255});
}
// 표지판 이미지를 CPU에서 그립니다(글자는 Raylib 기본 글꼴). / Draw the sign images on the CPU (text uses Raylib's default font).
static Texture2D makeSign(int kind){
 Image im=GenImageColor(512,320,RAYWHITE);
 if(kind==0){ // 파란 의무 표지: 안전모 착용 / blue mandatory sign: hard hats
  ImageDrawCircle(&im,256,110,92,(Color){20,82,160,255});
  ImageDrawCircle(&im,256,118,52,RAYWHITE);ImageDrawRectangle(&im,196,118,120,56,(Color){20,82,160,255}); // 모자 윗부분 / dome
  ImageDrawRectangle(&im,186,116,140,14,RAYWHITE);ImageDrawRectangle(&im,250,64,12,40,(Color){20,82,160,255}); // 챙과 골 / brim and ridge
  ImageDrawRectangle(&im,0,214,512,106,(Color){20,82,160,255});
  ImageDrawText(&im,"HARD HATS MUST BE",44,228,40,RAYWHITE);ImageDrawText(&im,"WORN AT ALL TIMES",44,272,40,RAYWHITE);}
 else{ // 빨간 출입 제한 표지 / red restricted-area sign
  ImageDrawRectangle(&im,0,0,512,110,(Color){196,30,36,255});
  ImageDrawText(&im,"RESTRICTED",76,18,60,RAYWHITE);ImageDrawText(&im,"AREA",196,72,32,RAYWHITE);
  ImageDrawText(&im,"AUTHORISED",86,138,52,(Color){30,30,32,255});ImageDrawText(&im,"WORKERS ONLY",54,200,52,(Color){30,30,32,255});
  ImageDrawRectangle(&im,0,280,512,40,(Color){196,30,36,255});ImageDrawText(&im,"SAWMILL SITE",150,288,28,RAYWHITE);}
 Texture2D t=LoadTextureFromImage(im);UnloadImage(im);
 GenTextureMipmaps(&t);SetTextureFilter(t,TEXTURE_FILTER_TRILINEAR);return t;
}
static void drawSign(const SawView *v,int k,float x){
 // 울타리 바깥을 향한 판(+z). 뒤에는 판자와 기둥. / Board facing out of the site (+z), with a backing board and posts.
 float y=1.75f,z=FENCE_Z1+.12f,w=2.4f,h=1.5f;
 box(v3(x-1.0f,1.0f,z-.04f),v3(.1f,2.2f,.1f),(Color){90,94,98,255});box(v3(x+1.0f,1.0f,z-.04f),v3(.1f,2.2f,.1f),(Color){90,94,98,255});
 box(v3(x,y,z-.05f),v3(w+.1f,h+.1f,.06f),(Color){200,204,208,255});
 if(v->signsReady)DrawModelEx(v->sign[k],v3(x,y,z),v3(1,0,0),90,v3(1,1,1),WHITE);
}

// ---- 구획 / Plots ----
static void drawPlot(const SawView *v,const SawState *s,int p){
 int r=rowOf(p);float cx=plotX(p),z0=ROW_BACK[r],z1=ROW_FRONT[r];bool own=p<s->plotsOwned;
 Color tape=own?(Color){232,196,40,255}:(Color){214,40,40,255},post=own?(Color){60,140,70,255}:(Color){230,230,230,255};
 // 말뚝과 테이프로 둘러친 구획 / plot cordoned off with posts and tape
 for(float x=cx-6.2f;x<=cx+6.21f;x+=3.1f){box(v3(x,.5f,z0),v3(.1f,1,.1f),post);box(v3(x,.5f,z1),v3(.1f,1,.1f),post);}
 box(v3(cx-6.2f,.5f,(z0+z1)/2),v3(.1f,1,.1f),post);box(v3(cx+6.2f,.5f,(z0+z1)/2),v3(.1f,1,.1f),post);
 for(int seg2=0;seg2<8;seg2++){Color c=(!own&&seg2%2)?RAYWHITE:tape;float xa=cx-6.2f+seg2*1.55f;
  box(v3(xa+.775f,.85f,z0),v3(1.55f,.06f,.02f),c);box(v3(xa+.775f,.85f,z1),v3(1.55f,.06f,.02f),c);}
 box(v3(cx-6.2f,.85f,(z0+z1)/2),v3(.02f,.06f,z1-z0),tape);box(v3(cx+6.2f,.85f,(z0+z1)/2),v3(.02f,.06f,z1-z0),tape);
 if(!own)box(v3(cx,1.1f,z1+.06f),v3(1.6f,.9f,.06f),(Color){214,40,40,255}); // 판매 표지 / for-sale board
 for(int t=0;t<TREES_PER_PLOT;t++){
  bool job=own&&s->harvBusy&&s->harvPlot==p&&s->harvTree==t;if(job)continue; // 작업 중인 나무는 하베스터가 그림 / drawn with the harvester
  float g=own?s->growth[p][t]:1;Vector3 b=v3(treeX(p,t),0,ROW_TREES[r]);
  if(g<1)DrawCylinderEx(b,v3(b.x,.32f,b.z),.28f,.26f,8,shade(bark,1.2f));               // 그루터기 / stump
  float h=fmaxf(.5f,TREE_H*g);drawTree(v3(b.x+(g<1?.35f:0),0,b.z),v3(0,1,0),h,.06f+.18f*g,1,g>=1?leaf:leafYoung);}
 logPile(cx,PILE_Z[r],v->pile[p],3.6f);
}

// ---- 하베스터 / Harvester ----
// 하베스터가 나무 앞에 서는 곳: 그 나무 옆 3.5 m, 숲 쪽 차선. / Where the harvester stands for a tree: 3.5 m beside it, forest-side lane.
static Vector2 workSpot(int p,int t){return (Vector2){treeX(p,t)-fallSign(p)*3.5f,ROAD_Z[rowOf(p)]+HARV_LANE};}
// 도로를 따라가는 경로(같은 줄이면 직선, 아니면 연결 도로로). / Route along the roads: straight on the same row, otherwise via a connector.
static int roadPath(Vector2 *out,Vector2 from,int fromRow,Vector2 to,int toRow,float lane,float sideX){
 int n=0;out[n++]=from;
 if(fromRow!=toRow){out[n++]=(Vector2){sideX,ROAD_Z[fromRow]+lane};out[n++]=(Vector2){sideX,ROAD_Z[toRow]+lane};}
 out[n++]=to;return n;
}
static float pathLen(const Vector2 *p,int n){float L=0;for(int i=1;i<n;i++)L+=hypotf(p[i].x-p[i-1].x,p[i].y-p[i-1].y);return L;}
static Vector2 pathAt(const Vector2 *p,int n,float d,float *yaw){
 for(int i=1;i<n;i++){float l=hypotf(p[i].x-p[i-1].x,p[i].y-p[i-1].y);
  if(d<=l||i==n-1){float t=l>0?fminf(d/l,1):1;if(l>.01f)*yaw=atan2f(p[i].x-p[i-1].x,p[i].y-p[i-1].y);
   return (Vector2){p[i-1].x+(p[i].x-p[i-1].x)*t,p[i-1].y+(p[i].y-p[i-1].y)*t};}
  d-=l;}
 return p[n-1];
}
// 작업 단계(progress): 이동 → 크레인 → 쓰러뜨림 → 가지치기·절단 → 정리
// Job phases (progress): drive -> reach -> fell -> strip and cut -> tidy up
#define H_DRIVE .30f
#define H_REACH .42f
#define H_FELL .55f
#define H_STRIP .85f
static void drawHarvester(const SawView *v,const SawState *s){
 Vector3 o=v3(v->hx,0,v->hz);float y=v->hyaw;
 Color body={226,170,36,255},dark={44,46,50,255},glass={140,190,210,255},tyre={28,28,30,255};
 obox(o,y,v3(0,1.2f,-.6f),v3(2.4f,1.0f,4.6f),body);                  // 차체 / chassis
 obox(o,y,v3(0,1.9f,-2.3f),v3(2.2f,1.0f,1.4f),shade(body,.85f));      // 엔진 / engine
 obox(o,y,v3(0,2.35f,1.0f),v3(1.9f,1.9f,1.6f),body);                 // 운전실 / cab
 obox(o,y,v3(0,2.55f,1.81f),v3(1.7f,1.2f,.04f),glass);obox(o,y,v3(.96f,2.55f,1.0f),v3(.04f,1.2f,1.3f),glass);obox(o,y,v3(-.96f,2.55f,1.0f),v3(.04f,1.2f,1.3f),glass);
 for(int sx=-1;sx<=1;sx+=2)for(int k=0;k<3;k++){Vector3 c=v3(sx*1.3f,.75f,1.3f-k*1.55f);
  DrawCylinderEx(xform(o,y,v3(c.x-.3f,c.y,c.z)),xform(o,y,v3(c.x+.3f,c.y,c.z)),.72f,.72f,14,tyre);}
 drawPerson(xform(o,y,v3(0,.92f,.95f)),y,0,false,false,true,(Color){236,238,240,255});
 // 크레인: 기둥 → 팔꿈치 → 헤드. 헤드 위치는 작업 단계에서 계산. / Crane: column -> elbow -> head; the head follows the job phase.
 Vector3 base=xform(o,y,v3(0,3.4f,-.2f)),rest=xform(o,y,v3(1.6f,2.2f,2.0f)),head=rest;
 Vector3 trunkBase=v3(0,0,0),dir=v3(0,1,0);float p=s->harvBusy?(float)s->harvProgress:0;bool job=s->harvBusy&&v->jobActive;
 if(job){int pl=s->harvPlot,t=s->harvTree;trunkBase=v3(treeX(pl,t),0,ROW_TREES[rowOf(pl)]);
  float a=p<H_REACH?0:p<H_FELL?(PI/2-.06f)*powf(seg(p,H_REACH,H_FELL),2):PI/2-.06f;dir=v3(fallSign(pl)*sinf(a)*FALL_COS,cosf(a),sinf(a)*FALL_SIN);
  Vector3 grip=v3(trunkBase.x+dir.x*1.2f,trunkBase.y+dir.y*1.2f+.3f,trunkBase.z+dir.z*1.2f);
  if(p<H_DRIVE)head=rest;
  else if(p<H_REACH)head=lerp3(rest,grip,seg(p,H_DRIVE,H_REACH));
  else if(p<H_FELL)head=grip;
  else if(p<H_STRIP){float d=1.2f+(TREE_H-2)*seg(p,H_FELL,H_STRIP);head=v3(trunkBase.x+dir.x*d,.9f,trunkBase.z+dir.z*d);}
  else head=lerp3(v3(trunkBase.x+dir.x*(TREE_H-.8f),.9f,trunkBase.z+dir.z*(TREE_H-.8f)),rest,seg(p,H_STRIP,1));
  // 나무: 서 있다가 쓰러지고, 가지가 벗겨지고, 마지막에 통나무 2개 / the tree stands, falls, is stripped, then lies as 2 logs
  if(p<H_STRIP){float canopy=p<H_FELL?1:1-seg(p,H_FELL,H_STRIP);Vector3 tb=v3(trunkBase.x,p<H_FELL?0:.35f,trunkBase.z);
   drawTree(tb,dir,TREE_H,.24f,canopy,leaf);}
  else{float L=TREE_H*.42f;Vector3 h0=v3(dir.x,0,dir.z),side=v3(dir.z*.5f,0,-dir.x*.5f); // 줄기 방향으로 통나무 2개 / two logs along the trunk
   for(int k=0;k<2;k++){Vector3 a0=v3(trunkBase.x+h0.x*(.6f+k*(L+.2f)),.3f,trunkBase.z+h0.z*(.6f+k*(L+.2f)));
    drawLog(v3(a0.x+side.x*k,.3f,a0.z+side.z*k),v3(a0.x+side.x*k+h0.x*L,.3f,a0.z+side.z*k+h0.z*L),.22f-.02f*k);}}
  DrawCylinderEx(trunkBase,v3(trunkBase.x,.32f,trunkBase.z),.28f,.26f,8,shade(bark,1.2f));}
 Vector3 elbow=v3((base.x+head.x)/2,fmaxf(base.y,head.y)+1.6f,(base.z+head.z)/2);
 DrawCylinderEx(xform(o,y,v3(0,2.0f,-.2f)),base,.3f,.26f,10,dark);
 DrawCylinderEx(base,elbow,.2f,.17f,8,body);DrawCylinderEx(elbow,head,.16f,.13f,8,body);
 box(head,v3(.75f,.75f,.75f),(Color){60,60,64,255});box(v3(head.x,head.y-.45f,head.z),v3(.9f,.12f,.25f),(Color){200,60,40,255}); // 헤드와 톱 / head and saw bar
}

// ---- 포워더 / Forwarder ----
#define ACT_LOAD 100
#define ACT_UNLOAD 200
static void drawForwarder(const SawAgent *a){
 Vector3 o=v3(a->x,0,a->z);float y=a->yaw;
 Color body={58,120,64,255},dark={40,42,46,255},glass={140,190,210,255},tyre={28,28,30,255},stake={90,94,98,255};
 obox(o,y,v3(0,1.15f,2.2f),v3(2.3f,1.0f,2.6f),body);obox(o,y,v3(0,2.25f,2.6f),v3(1.8f,1.4f,1.4f),body);
 obox(o,y,v3(0,2.35f,3.31f),v3(1.6f,.9f,.04f),glass);
 obox(o,y,v3(0,1.0f,-1.6f),v3(2.4f,.5f,4.6f),shade(body,.8f));                    // 적재함 / load bed
 for(int k=0;k<3;k++)for(int sx=-1;sx<=1;sx+=2)obox(o,y,v3(sx*1.15f,1.95f,-3.4f+k*1.6f),v3(.1f,1.5f,.1f),stake);
 obox(o,y,v3(0,1.9f,.6f),v3(2.3f,1.4f,.12f),dark);                                // 앞 칸막이 / headboard
 for(int sx=-1;sx<=1;sx+=2)for(int k=0;k<4;k++){Vector3 c=v3(sx*1.25f,.65f,2.6f-k*1.75f-(k>1?.6f:0));
  DrawCylinderEx(xform(o,y,v3(c.x-.28f,c.y,c.z)),xform(o,y,v3(c.x+.28f,c.y,c.z)),.62f,.62f,12,tyre);}
 for(int i=0;i<a->load&&i<FORWARDER_LOAD;i++){int layer=i/4,k=i%4;float lx=-.8f+k*.53f,ly=1.5f+layer*.44f;
  DrawCylinderEx(xform(o,y,v3(lx,ly,-3.6f)),xform(o,y,v3(lx,ly,.2f)),.22f,.22f,10,logC);}
 Vector3 cb=xform(o,y,v3(0,2.0f,.9f)),ct=xform(o,y,v3(0,3.6f+a->crane,.9f+a->crane)),ch=xform(o,y,v3(1.4f*a->crane,2.6f+a->crane*.6f,-1.0f));
 DrawCylinderEx(cb,ct,.18f,.15f,8,dark);DrawCylinderEx(ct,ch,.13f,.11f,8,body);box(ch,v3(.5f,.4f,.5f),dark); // 크레인과 집게 / crane and grapple
 drawPerson(xform(o,y,v3(0,.8f,2.5f)),y,0,false,false,true,(Color){236,238,240,255});
}
static void route(SawAgent *a,float x,float z,int act){if(a->n<24){a->pts[a->n]=(Vector2){x,z};a->act[a->n++]=act;}}
// 야적장 → 가장 큰 더미 → 야적장 / yard -> biggest pile -> yard
static void startForwarder(SawView *v,int p){
 SawAgent *a=&v->forwarder;float x=a->x,z=a->z,yaw=a->yaw;int load=a->load;
 *a=(SawAgent){.x=x,.z=z,.yaw=yaw,.active=true,.load=load};
 int r=rowOf(p);Vector2 tmp[8];Vector2 from={YARD_X,UNLOAD_Z},to={plotX(p),ROAD_Z[r]+FWD_LANE};
 int n=roadPath(tmp,from,0,to,r,FWD_LANE,FWD_SIDE_X);for(int i=1;i<n;i++)route(a,tmp[i].x,tmp[i].y,i==n-1?ACT_LOAD+p:0);
 n=roadPath(tmp,to,r,from,0,FWD_LANE,FWD_SIDE_X);for(int i=1;i<n;i++)route(a,tmp[i].x,tmp[i].y,i==n-1?ACT_UNLOAD:0);
}
static void stepForwarder(SawView *v,SawState *s,float dt){
 SawAgent *a=&v->forwarder;
 float want=(a->wait>0)?1:0;a->crane+=(want-a->crane)*(1-expf(-5*dt));
 if(!a->active)return;
 if(a->wait>0){a->wait-=dt;return;}
 if(a->k>=a->n){a->active=false;return;}
 Vector2 t=a->pts[a->k];float dx=t.x-a->x,dz=t.y-a->z,d=sqrtf(dx*dx+dz*dz);
 if(d<.03f){a->x=t.x;a->z=t.y;int act=a->act[a->k++];
  if(act>=ACT_UNLOAD){int m=SawUnloadLogs(s,a->load);a->load-=m;
   if(a->load>0){a->k--;a->wait=1;}else a->wait=2;}                         // 야적장이 가득 차면 기다림 / waits if the yard is full
  else if(act>=ACT_LOAD){int p=act-ACT_LOAD,n=SawTakeLogs(s,v->pile[p]<FORWARDER_LOAD-a->load?v->pile[p]:FORWARDER_LOAD-a->load);
   v->pile[p]-=n;v->lastRoadside-=n;a->load+=n;a->wait=2;}
  return;}
 // 목표가 뒤에 있으면 돌지 않고 후진합니다(실제 임업 기계처럼). / If the target is behind, reverse instead of turning round, like real forest machines.
 float head=atan2f(dx,dz),diff=wrapAngle(head-a->yaw);if(fabsf(diff)>PI/2)diff=wrapAngle(head+PI-a->yaw);
 float r=2.2f*dt;a->yaw=wrapAngle(a->yaw+(fabsf(diff)<r?diff:(diff>0?r:-r)));
 if(fabsf(diff)>.12f)return;
 float step=fminf(d,10.0f*dt);a->x+=dx/d*step;a->z+=dz/d*step;
}

// ---- 제재 라인 / Mill line ----
static void drawMill(const SawView *v,const SawState *s){
 Color steel={120,130,140,255},dark={44,50,58,255},concrete={170,170,166,255};
 box(v3(2,.03f,0),v3(30,.06f,6),concrete);                                            // 콘크리트 바닥 / concrete pad
 // 야적장 / log yard
 box(v3(YARD_X-2.6f,.6f,YARD_Z),v3(.2f,1.2f,3),dark);box(v3(YARD_X+2.6f,.6f,YARD_Z),v3(.2f,1.2f,3),dark);
 logPile(YARD_X,YARD_Z+.9f,s->yard,4.6f);
 for(int k=0;k<MILL_STAGES;k++){float x=STAGE_X[k];bool own=k<s->stagesOwned;const MillStage *m=&s->stage[k];
  if(!own){Color g={110,116,122,255};DrawCubeWiresV(v3(x,1.4f,0),v3(3,2.8f,2.4f),g);box(v3(x,.07f,0),v3(3.4f,.02f,2.8f),(Color){160,150,110,255});continue;}
  float p=m->busy?(float)m->progress:0;Color item=(m->pendingDefect&&p>.5f)?defectC:k==0?plankC:k==1?boardC:mouldC;
  if(k==0){ // 밴드 톱: 큰 바퀴 두 개와 통나무를 실은 캐리지 / band saw: two big wheels and a log carriage
   box(v3(x,.15f,.8f),v3(11,.12f,.2f),steel);box(v3(x,.15f,-.2f),v3(11,.12f,.2f),steel);       // 레일 / rails
   box(v3(x+.9f,2.5f,-.9f),v3(.5f,5,.5f),dark);                                                    // 톱 기둥 / saw column
   for(int w=0;w<2;w++){Vector3 c=v3(x+.25f,w?4.2f:1.3f,-.55f);
    DrawCylinderEx(v3(c.x,c.y,c.z-.15f),v3(c.x,c.y,c.z+.15f),.95f,.95f,18,steel);
    float a=v->wheel[0]*(w?1:1);box(v3(c.x+cosf(a)*.75f,c.y+sinf(a)*.75f,c.z+.17f),v3(.18f,.18f,.02f),dark);}
   box(v3(x+.25f,2.75f,.05f),v3(.04f,3.0f,.04f),(Color){220,224,228,255});                        // 톱날 / blade
   float cx=m->busy?lerpf(x-4.5f,x+4.0f,seg(p,.05f,.85f)):x-4.5f;
   box(v3(cx,.45f,.3f),v3(2.2f,.5f,1.4f),(Color){200,150,40,255});                                 // 캐리지 / carriage
   if(m->busy){if(p<.55f)drawLog(v3(cx-1.9f,1.0f,.3f),v3(cx+1.9f,1.0f,.3f),.3f);
    else for(int b=0;b<3;b++)box(v3(cx,.82f+b*.12f,.3f),v3(3.8f,.1f,.5f),item);}
   // 톱밥·폐기 더미 / sawdust and scrap pile
   float hp=fminf(1.6f,.2f+s->scrapTotal*.05f);DrawCylinderEx(v3(x+1.5f,0,-2.4f),v3(x+1.5f,hp,-2.4f),1.3f,.1f,12,(Color){196,160,110,255});}
  else{ // 대패·몰더: 상자형 기계와 회전 커터 / planer and moulder: box machine with spinning cutter heads
   Color c=k==1?(Color){60,110,150,255}:(Color){150,80,60,255};
   box(v3(x,.9f,0),v3(2.6f,1.6f,1.8f),c);box(v3(x,1.85f,0),v3(1.8f,.3f,1.4f),shade(c,.8f));
   box(v3(x-2.4f,.75f,0),v3(2.2f,.08f,.8f),steel);box(v3(x+2.4f,.75f,0),v3(2.2f,.08f,.8f),steel);    // 공급·배출 테이블 / in- and out-feed tables
   for(int h=0;h<2;h++){float a=v->wheel[k]+h*PI/2;Vector3 hc=v3(x-.5f+h,2.15f,0);
    DrawCylinderEx(v3(hc.x,hc.y,hc.z-.5f),v3(hc.x,hc.y,hc.z+.5f),.22f,.22f,10,steel);
    box(v3(hc.x+cosf(a)*.22f,hc.y+sinf(a)*.22f,hc.z),v3(.06f,.06f,1.02f),dark);}
   if(m->busy){float bx=lerpf(x-3.3f,x+3.3f,seg(p,.05f,.95f));box(v3(bx,.86f,0),v3(1.6f,.1f,.45f),item);}
   // 대기열 판재 더미 / queue of boards waiting
   for(int q=0;q<m->queue&&q<STAGE_QUEUE_CAP;q++)box(v3(x-2.6f,.12f+q*.14f,1.6f),v3(1.8f,.12f,.5f),plankC);}
 }
 // 완제품 더미(트럭이 주기적으로 가져감) / finished stack, collected periodically
 int sold=s->sold[1]+s->sold[2]+s->sold[3],n=sold%24;Color fin=s->stagesOwned>=3?mouldC:s->stagesOwned==2?boardC:plankC;
 box(v3(STACK_X,.08f,0),v3(2.2f,.16f,1.4f),(Color){112,82,52,255});
 for(int i=0;i<n;i++)box(v3(STACK_X,.22f+i*.12f,0),v3(2.0f,.1f,1.2f),i%2?fin:shade(fin,.93f));
}
// 작업장 둘레를 도는 작업자(형광 조끼, 노란 안전모) / a worker walking a loop around the mill pad (hi-vis, yellow hard hat)
static void patrolPos(float t,float *x,float *z,float *yaw){
 const float P[4][2]={{-11,3.5f},{15,3.5f},{15,5.5f},{-11,5.5f}};float L[4],tot=0;
 for(int i=0;i<4;i++){int j=(i+1)%4;L[i]=hypotf(P[j][0]-P[i][0],P[j][1]-P[i][1]);tot+=L[i];}
 float d=fmodf(t,tot);
 for(int i=0;i<4;i++){int j=(i+1)%4;if(d<=L[i]){float f=d/L[i];*x=P[i][0]+(P[j][0]-P[i][0])*f;*z=P[i][1]+(P[j][1]-P[i][1])*f;*yaw=atan2f(P[j][0]-P[i][0],P[j][1]-P[i][1]);return;}d-=L[i];}
 *x=P[0][0];*z=P[0][1];*yaw=0;
}

// ---- 공개 함수 / Public functions ----
void SawViewFocus(SawView *v,int target){
 OrbitCam *c=&v->orbit;
 if(target==0){c->focusGoal=v3(0,3,-24);c->dist=30;c->pitch=.55f;}       // 숲 / forest
 else if(target==1){c->focusGoal=v3(2,1.5f,0);c->dist=20;c->pitch=.45f;}  // 제재 라인 / mill
 else{c->focusGoal=v3(0,2,-8);c->yaw=0;c->pitch=.45f;c->dist=c->maxDist;} // 문과 표지판부터 먼 숲까지 모두 보임 / shows everything from the gate signs to the far plots
}
void SawViewInit(SawView *v,int width,int height){
 *v=(SawView){.width=width,.height=height,.hx=-20,.hz=ROAD_Z[0]+HARV_LANE,.hyaw=PI/2};
 v->target=LoadRenderTexture(width*SS,height*SS);SetTextureFilter(v->target.texture,TEXTURE_FILTER_BILINEAR);
 // 카메라: 현장 상자 안(울타리 밖 마당 포함), 구획과 기계를 피함. / Camera: inside the site box (including outside the gate), clear of plots and machines.
 OrbitCam *c=&v->orbit;
 c->cam=(Camera3D){.up={0,1,0},.fovy=60,.projection=CAMERA_PERSPECTIVE};
 c->lo=v3(-38,0,-46);c->hi=v3(38,32,34);
 c->margin=.6f;c->minDist=4;c->maxDist=70;c->pitchMin=.14f;c->pitchMax=1.35f;
 for(int p=0;p<PLOTS;p++){int r=rowOf(p);c->obstacles[c->obstacleCount++]=(Box3){v3(plotX(p)-6.3f,0,ROW_BACK[r]),v3(plotX(p)+6.3f,TREE_H+1.5f,ROW_FRONT[r])};}
 c->obstacles[c->obstacleCount++]=(Box3){v3(STAGE_X[0]-1.2f,0,-1.5f),v3(STAGE_X[0]+1.4f,5.2f,1.2f)};
 for(int k=1;k<MILL_STAGES;k++)c->obstacles[c->obstacleCount++]=(Box3){v3(STAGE_X[k]-1.3f,0,-.9f),v3(STAGE_X[k]+1.3f,2.3f,.9f)};
 c->obstacles[c->obstacleCount++]=(Box3){v3(YARD_X-2.8f,0,YARD_Z-1.6f),v3(YARD_X+2.8f,2.6f,YARD_Z+1.6f)};
 SawViewFocus(v,-1);OrbitSnap(c);
 v->forwarder=(SawAgent){.x=YARD_X,.z=UNLOAD_Z,.yaw=PI/2};
 for(int k=0;k<2;k++){v->signTex[k]=makeSign(k);v->sign[k]=LoadModelFromMesh(GenMeshPlane(2.4f,1.5f,1,1));
  v->sign[k].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=v->signTex[k];}
 v->signsReady=true;
}
void SawViewUnload(SawView *v){
 if(v->signsReady)for(int k=0;k<2;k++){UnloadModel(v->sign[k]);UnloadTexture(v->signTex[k]);}
 UnloadRenderTexture(v->target);
}
void SawViewInput(SawView *v,Rectangle area,float dt){OrbitUpdate(&v->orbit,area,dt);}
void SawViewUpdate(SawView *v,SawState *s,float dt){
 dt=(dt>0)?fminf(dt,.1f):0;v->time+=dt;
 float sdt=s->running?dt*(float)s->speed:0;
 // 새 게임(또는 첫 프레임): 표시 상태를 맞춥니다. / New game or first frame: sync the display.
 int soldNow=0;for(int k=0;k<=MILL_STAGES;k++)soldNow+=s->sold[k];int soldBefore=0;for(int k=0;k<=MILL_STAGES;k++)soldBefore+=v->lastSold[k];
 if(!v->synced||soldNow<soldBefore||s->elapsed<v->lastElapsed){ // 시계가 뒤로 감 = 새 게임 / clock went back = new game
  for(int p=0;p<PLOTS;p++)v->pile[p]=0;
  v->pile[0]=s->roadside;v->lastRoadside=s->roadside;
  for(int k=0;k<=MILL_STAGES;k++)v->lastSold[k]=s->sold[k];
  v->lastScrap=s->scrapTotal;
  v->forwarder=(SawAgent){.x=YARD_X,.z=UNLOAD_Z,.yaw=PI/2};v->hx=-20;v->hz=ROAD_Z[0]+HARV_LANE;v->hyaw=PI/2;v->hRow=0;v->jobActive=false;v->popupCount=0;v->synced=true;}
 // 하베스터: 새 나무가 정해지면 지금 위치에서 그 나무까지 경로를 만듭니다.
 // Harvester: when the simulation picks a new tree, plan a route from where it is now.
 if(s->harvBusy&&(!v->jobActive||s->harvPlot!=v->jobPlot||s->harvTree!=v->jobTree)){
  v->jobActive=true;v->jobPlot=s->harvPlot;v->jobTree=s->harvTree;
  Vector2 to=workSpot(s->harvPlot,s->harvTree);
  v->hn=roadPath(v->hpath,(Vector2){v->hx,v->hz},v->hRow,to,rowOf(s->harvPlot),HARV_LANE,HARV_SIDE_X);v->hlen=pathLen(v->hpath,v->hn);}
 if(s->harvBusy){v->workPlot=s->harvPlot;
  float f=seg((float)s->harvProgress,0,H_DRIVE),yaw=v->hyaw;Vector2 p=pathAt(v->hpath,v->hn,f*v->hlen,&yaw);
  v->hx=p.x;v->hz=p.y;
  if(f<1){if(fabsf(wrapAngle(yaw-v->hyaw))>PI/2)yaw=wrapAngle(yaw+PI);v->hyaw=yaw;}   // 뒤로 가면 후진 / reverse when going backwards
  else{float d1=wrapAngle(PI/2-v->hyaw),d2=wrapAngle(-PI/2-v->hyaw),d=fabsf(d1)<fabsf(d2)?d1:d2;v->hyaw=wrapAngle(v->hyaw+d*(1-expf(-6*sdt)));} // 가까운 도로 방향으로 정렬 / line up with the nearer road direction
  if(f>=1)v->hRow=rowOf(s->harvPlot);}
 else if(v->jobActive&&!s->harvBusy){v->jobActive=false;}
 // 길가 더미: 시뮬레이션 roadside가 늘면 방금 작업한 구획 더미에 추가 / roadside piles: growth goes to the plot just worked
 int dr=s->roadside-v->lastRoadside;if(dr>0)v->pile[v->workPlot]+=dr;
 int sum=0;for(int p=0;p<PLOTS;p++)sum+=v->pile[p];
 if(sum!=s->roadside){for(int p=0;p<PLOTS;p++)v->pile[p]=0;v->pile[v->workPlot]=s->roadside;} // 안전장치 / safety resync
 v->lastRoadside=s->roadside;
 // 포워더: 더미가 6개 이상이거나 야적장이 거의 비면 가장 큰 더미로 출발
 // Forwarder: sets off for the biggest pile once there are 6+ logs waiting, or when the yard is nearly empty
 if(!v->forwarder.active&&s->running&&s->stagesOwned>0&&v->forwarder.load==0){int best=-1;
  for(int p=0;p<PLOTS;p++)if(v->pile[p]>0&&(best<0||v->pile[p]>v->pile[best]))best=p;
  if(best>=0&&(s->roadside>=6||s->yard<4))startForwarder(v,best);}
 stepForwarder(v,s,sdt);
 // 판매 글자와 회전 부품 / sale popups and spinning parts
 int n=0;for(int k=0;k<v->popupCount;k++){v->popups[k].age+=dt;if(v->popups[k].age<1.4f)v->popups[n++]=v->popups[k];}v->popupCount=n;
 for(int k=0;k<=MILL_STAGES;k++){int d=s->sold[k]-v->lastSold[k];
  Vector3 at=k==0?v3(v->hx,4.5f,v->hz):v3(STACK_X,2.6f,0);
  for(int i=0;i<d&&i<2&&v->popupCount<SAW_POPUPS;i++)v->popups[v->popupCount++]=(SawPopup){at,0,(float)SAW_TIER_VALUE[k],false};
  v->lastSold[k]=s->sold[k];}
 if(s->scrapTotal>v->lastScrap&&v->popupCount<SAW_POPUPS)v->popups[v->popupCount++]=(SawPopup){v3(STAGE_X[0]+1.5f,2.4f,-2.4f),0,0,true};
 v->lastScrap=s->scrapTotal;
 for(int k=0;k<MILL_STAGES;k++)if(k<s->stagesOwned&&s->stage[k].busy&&!s->stage[k].blocked)v->wheel[k]=fmodf(v->wheel[k]+sdt*(float)s->stage[k].speed*9,2*PI);
 if(s->running)v->patrol+=sdt*1.3f;
 v->lastElapsed=(float)s->elapsed;
}
void SawViewRender(SawView *v,const SawState *s){
 BeginTextureMode(v->target);ClearBackground((Color){150,190,226,255});
 BeginMode3D(v->orbit.cam);
 drawSurroundings();fence();drawSign(v,0,-8.2f);drawSign(v,1,8.2f);
 for(int p=0;p<PLOTS;p++)drawPlot(v,s,p);
 drawMill(v,s);drawHarvester(v,s);drawForwarder(&v->forwarder);
 float px,pz,py;patrolPos(v->patrol,&px,&pz,&py);drawPerson(v3(px,0,pz),py,v->patrol*3.5f,s->running,false,false,(Color){244,184,32,255});
 EndMode3D();EndTextureMode();
}
// 3D 화면 위의 라벨과 판매 글자. 상자 라벨은 이미 그린 라벨과 겹치면 건너뜁니다(먼저 그린 것이 우선).
// Labels and sale popups over the 3D view. A boxed label is skipped if it would overlap one already drawn (first drawn wins).
static Rectangle placed[32];static int nplaced;
static void overlayText(const SawView *v,int x,int y,Vector3 p,const char *t,Color c,int size,bool boxed){
 const Camera3D *cam=&v->orbit.cam;Vector3 f=v3(cam->target.x-cam->position.x,cam->target.y-cam->position.y,cam->target.z-cam->position.z);
 if((p.x-cam->position.x)*f.x+(p.y-cam->position.y)*f.y+(p.z-cam->position.z)*f.z<=0)return;
 Vector2 sp=GetWorldToScreenEx(p,*cam,v->width*SS,v->height*SS);int tw=UiMeasure(t,size),lx=x+(int)(sp.x/SS)-tw/2,ly=y+(int)(sp.y/SS)-size/2;
 if(lx-6<x||ly-5<y||lx+tw+6>x+v->width||ly+size+5>y+v->height)return;
 if(boxed){Rectangle r={(float)(lx-6),(float)(ly-5),(float)(tw+12),(float)(size+10)};
  for(int i=0;i<nplaced;i++)if(CheckCollisionRecs(r,placed[i]))return;
  if(nplaced<32)placed[nplaced++]=r;}
 if(boxed)DrawRectangle(lx-6,ly-5,tw+12,size+10,(Color){13,21,30,210});else UiText(t,lx+1,ly+1,size,(Color){0,0,0,(unsigned char)(c.a*.6f)});
 UiText(t,lx,ly,size,c);
}
void SawViewDraw(const SawView *v,const SawState *s,int x,int y){
 int w=v->width,h=v->height;
 DrawTexturePro(v->target.texture,(Rectangle){0,0,(float)(w*SS),(float)(-h*SS)},(Rectangle){(float)x,(float)y,(float)w,(float)h},(Vector2){0,0},0,WHITE);
 Color muted={145,163,181,255},warn={240,170,40,255};
 nplaced=0;
 overlayText(v,x,y,v3(v->hx,5.4f,v->hz),s->harvBusy?"HARVESTER  FELLING":"HARVESTER  WAITING FOR TREES",s->harvBusy?accentC:warn,14,true);
 if(s->stagesOwned>0)overlayText(v,x,y,v3(v->forwarder.x,4.6f,v->forwarder.z),TextFormat("FORWARDER  %d LOGS",v->forwarder.load),accentC,14,true);
 overlayText(v,x,y,v3(YARD_X,3.2f,YARD_Z),TextFormat("LOG YARD  %d/%d",s->yard,YARD_CAP),accentC,14,true);
 for(int k=0;k<MILL_STAGES;k++){bool own=k<s->stagesOwned;const MillStage *m=&s->stage[k];
  overlayText(v,x,y,v3(STAGE_X[k],k?3.2f:5.9f,0),own?TextFormat("%s  %s",SAW_STAGE_NAME[k],m->blocked?"BLOCKED":m->busy?"WORKING":"WAITING"):TextFormat("%s  LOCKED",SAW_STAGE_NAME[k]),own?(m->blocked?warn:accentC):muted,14,true);}
 for(int p=0;p<PLOTS;p++){int r=rowOf(p);bool own=p<s->plotsOwned;int m=0;for(int t=0;t<TREES_PER_PLOT;t++)m+=s->growth[p][t]>=1;
  overlayText(v,x,y,v3(plotX(p),TREE_H+2,ROW_TREES[r]),own?TextFormat("PLOT %d  %d/%d READY",p+1,m,TREES_PER_PLOT):TextFormat("PLOT %d  FOR SALE",p+1),own?accentC:muted,14,true);}
 for(int k=0;k<v->popupCount;k++){const SawPopup *pp=&v->popups[k];unsigned char a=(unsigned char)(255*clampf(1-pp->age/1.4f,0,1));
  Vector3 at=v3(pp->at.x,pp->at.y+pp->age*1.2f,pp->at.z);
  overlayText(v,x,y,at,pp->bad?"SCRAP":TextFormat("+EUR %.0f",pp->value),pp->bad?(Color){235,90,70,a}:(Color){65,222,177,a},18,false);}
 DrawRectangle(x,y+h-26,268,26,(Color){13,21,30,200});UiText("Drag: orbit   Wheel: zoom",x+8,y+h-20,14,muted);
 DrawRectangleLines(x,y,w,h,(Color){39,54,69,255});
 if(s->contractDone){DrawRectangle(x,y,w,44,(Color){13,21,30,220});UiText(TextFormat("CONTRACT COMPLETE IN %d:%02d",(int)s->contractTime/60,(int)s->contractTime%60),x+16,y+10,24,accentC);}
}
