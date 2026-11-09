/* stub calling-convention test: compiles crowdtune.c for 32-bit Linux with fake Win32 + fake game functions */
#define CT_MINIMAL_WINAPI
#define readable readable_real
#include "../src/crowdtune.c"
#undef readable
void *memset(void *d,int c,unsigned n){unsigned char*p=d;while(n--)*p++=(unsigned char)c;return d;}
static int sys3(int n,int a,int b,int c){int r;__asm__ volatile("int $0x80":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c):"memory");return r;}
static void wr(const char*s){sys3(4,1,(int)s,s_len(s));}
static void hx(unsigned v){char b[12];int i;for(i=0;i<8;i++){unsigned d=(v>>(28-4*i))&15;b[i]=d<10?'0'+d:'a'+d-10;}b[8]=' ';b[9]=0;wr(b);}
static int s_cmp(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a-*b;}
/* fake Win32 */
static char attr_ok[260]; static char loaded[4][260]; static int nl;
HANDLE WINAPI CreateThread(LPVOID a, SIZE_T b, LPTHREAD_START_ROUTINE c, LPVOID d, DWORD e, LPDWORD f){return 0;}
void WINAPI Sleep(DWORD a){}
BOOL WINAPI VirtualProtect(LPVOID a, SIZE_T b, DWORD c, LPDWORD d){return 1;}
SIZE_T WINAPI VirtualQuery(LPCVOID p, MEMORY_BASIC_INFORMATION *m, SIZE_T n){m->State=MEM_COMMIT;m->Protect=PAGE_READWRITE;m->BaseAddress=(void*)0;m->RegionSize=0xffffffff;return sizeof *m;}
HMODULE WINAPI GetModuleHandleA(LPCSTR a){return 0;} DWORD WINAPI GetModuleFileNameA(HMODULE a, LPSTR b, DWORD c){s_cpy(b,"/tmp/x/nba2k14.exe",260);return 1;}
HMODULE WINAPI LoadLibraryA(LPCSTR a){ if(nl<4) s_cpy(loaded[nl++],a,260); return (HMODULE)1;} FARPROC WINAPI GetProcAddress(HMODULE a, LPCSTR b){return 0;}
HANDLE WINAPI CreateFileA(LPCSTR a, DWORD b, DWORD c, LPVOID d, DWORD e, DWORD f, HANDLE g){return INVALID_HANDLE_VALUE;}
BOOL WINAPI WriteFile(HANDLE a, LPCVOID b, DWORD c, LPDWORD d, LPVOID e){return 1;} BOOL WINAPI CloseHandle(HANDLE a){return 1;}
DWORD WINAPI SetFilePointer(HANDLE a, LONG b, LONG *c, DWORD d){return 0;} UINT WINAPI GetSystemDirectoryA(LPSTR a, UINT b){s_cpy(a,"C:\\Windows\\System32",b);return 1;}
UINT WINAPI GetPrivateProfileIntA(LPCSTR a, LPCSTR k, int d, LPCSTR f){return d;}
DWORD WINAPI GetPrivateProfileStringA(LPCSTR a, LPCSTR k, LPCSTR d, LPSTR o, DWORD n, LPCSTR f){
  const char *v=""; if(!s_cmp(k,"BigSituations")) v="11"; s_cpy(o,v,n); return s_len(v);}
void WINAPI GetLocalTime(SYSTEMTIME *s){s->wHour=s->wMinute=s->wSecond=0;} BOOL WINAPI FlushInstructionCache(HANDLE a, LPCVOID b, SIZE_T c){return 1;}
HANDLE WINAPI GetCurrentProcess(void){return 0;} static DWORD tick=100000; DWORD WINAPI GetTickCount(void){return tick;} BOOL WINAPI DisableThreadLibraryCalls(HMODULE a){return 1;}
static char mtx[8][96]; static int nm_, lasterr;
HANDLE WINAPI CreateMutexA(LPVOID a, BOOL b, LPCSTR n){int i; for(i=0;i<nm_;i++) if(!s_cmp(mtx[i],n)){lasterr=183;return (HANDLE)1;} s_cpy(mtx[nm_++],n,96); lasterr=0; return (HANDLE)1;}
DWORD WINAPI GetLastError(void){return lasterr;} DWORD WINAPI GetCurrentProcessId(void){return 0x1234;}
DWORD WINAPI GetFileAttributesA(LPCSTR a){return s_cmp(a,attr_ok)?0xFFFFFFFF:0;}

static int sys6(int n,int a,int b,int c,int d,int e,int f){int r;__asm__ volatile("push %%ebp\n mov %7,%%ebp\n int $0x80\n pop %%ebp":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c),"S"(d),"D"(e),"m"(f):"memory");return r;}
static void dd(int v){char t[12];u2d((u32)v,t);wr(t);}
static unsigned setm[4], setf[4];
static unsigned char chans[60][0x34];
__attribute__((force_align_arg_pointer)) int main_(void){
  u8*img=(u8*)sys6(192,0x400000,0x1d7c000,7,0x32,-1,0);
  if((u32)img!=0x400000){wr("mmap fail\n");sys3(1,1,0,0);}
  load_ini(); C.enabled=1; C.playoffs=2; C.gate_in_memory=0; C.events=0;
  setm[1]=601; setf[1]=93;
  *(u32*)(G_SETS)=(u32)setm; *(u32*)(G_SETS+4)=(u32)setf;
  { int k; for(k=0;k<60;k++) ((int*)G_PERM)[k]=k; }
  { u8 ch[0x34]={0}; int k,c3=0; *(u32*)(ch+0x10)=0; *(float*)G_EXCITE=1.0f;
    *(u32*)G_LIVE=0; for(k=0;k<60;k++){*(u32*)(ch+8)=k; if(ct_policy(ch,7)==7) c3++;} wr("not live, excitement 1.0, game asks stand cheer (7): standing "); dd(c3); wr("/60\n");
    *(u32*)G_LIVE=1; c3=0; for(k=0;k<60;k++){*(u32*)(ch+8)=k; if(ct_policy(ch,7)==7) c3++;} wr("tip-off (live, first 45 s): standing "); dd(c3); wr("/60\n");
    C.tipoff_calm=0; C.resume_calm=0; g_live_since=0; c3=0; for(k=0;k<60;k++){*(u32*)(ch+8)=k; if(ct_policy(ch,7)==7) c3++;} wr("live after tip-off: standing "); dd(c3); wr("/60\n");
    C.tipoff_calm=0; C.resume_calm=0; *(u32*)G_LIVE=1; }
  { int q; for(q=0;q<8;q++){C.big_sit[q]=-1;C.score_sit[q]=-1;} C.big_sit[0]=7;C.big_sit[1]=8;C.big_sit[2]=11;C.score_sit[0]=3; C.clap_calm=0.03f;C.clap_good=0.45f;C.cheer_good=0.08f;C.stand_big_max=0.5f;C.seat_cheer_big=0.2f;C.props=0.04f;C.sad=0.2f;C.stand_idle=0;C.allow_stand=1;C.tipoff_calm=0;C.resume_calm=0; }
  { int sits[]={2,3,15,7}; const char *sn[]={"normal play (situation 2)","scoring reaction (situation 3)","chant (situation 15)","big moment (situation 7)"};
    int asks[]={1,2,3,8,15}; int si,ai,k,set;
    for(set=0;set<2;set++){ wr(set?"\n=== WOMEN\n":"=== MEN\n");
     for(si=0;si<4;si++){ wr(sn[si]); wr("\n"); g_sit=sits[si];
      for(ai=0;ai<5;ai++){ int cnt[18]={0}; u8 ch[0x34]={0}; int g=asks[ai]; tick+=60000;
        for(k=0;k<60;k++){*(u32*)(ch+8)=k; *(u32*)(ch+0x10)=set; cnt[ct_policy(ch,g)]++;}
        wr("   game asks "); dd(g); wr(" ->"); { int o; for(o=0;o<18;o++) if(cnt[o]){ wr(" "); dd(o); wr(":"); dd(cnt[o]); } }
        { int st=0,o; for(o=0;o<18;o++){ int stand = (o==6||o==7||o==5); if(stand) st+=cnt[o]; } wr("   standing "); dd(st*100/60); wr("%\n"); }
      }
     }
    }
  }

  /* calm hold: a seated fan asked for 'disappointed' twice within 12 s stays seated */
  { u8 ch[0x34]={0}; int k,held=0,moved=0; g_sit=2; C.sad=1.0f; C.calm_hold=12;
    for(k=0;k<60;k++){*(u32*)(ch+8)=k; *(u32*)(ch+0x10)=0; ct_policy(ch,1);}      /* everybody seated idle now */
    tick+=3000; for(k=0;k<60;k++){*(u32*)(ch+8)=k; if(ct_policy(ch,4)==1) held++;}
    tick+=13000; for(k=0;k<60;k++){*(u32*)(ch+8)=k; if(ct_policy(ch,4)==4) moved++;}
    wr("\ncalm hold: asked 'disappointed' 3 s after sitting down -> stayed seated "); dd(held); wr("/60; 16 s after -> head-in-hands "); dd(moved); wr("/60\n"); C.sad=0.05f; }
  /* buzzer: game clock runs out, game only asks seated idle (1) */
  { u8 ch[0x34]={0}; int k,st=0,clap=0; g_sit=2; C.buzzer_secs=7; C.buzzer_stand=0.35f;
    *(float*)G_CLOCK=3.0f; *(float*)G_SHOT=10.0f; tick+=60000; *(u32*)(ch+0x10)=0; *(u32*)(ch+8)=0; ct_policy(ch,1);
    *(float*)G_CLOCK=0.0f; tick+=500;
    for(k=0;k<60;k++){int o; *(u32*)(ch+8)=k; o=ct_policy(ch,1); if(o==6||o==7||o==5) st++; else if(o==2||o==3) clap++;}
    wr("period-end buzzer (game asks idle): standing "); dd(st); wr("/60, seated cheer/clap "); dd(clap); wr("/60\n");
    tick+=8000; st=0; for(k=0;k<60;k++){int o; *(u32*)(ch+8)=k; o=ct_policy(ch,1); if(o!=1) st++;}
    wr("8 s later: not seated idle "); dd(st); wr("/60\n");
    *(float*)G_CLOCK=40.0f; *(float*)G_SHOT=2.0f; ct_policy(ch,1); *(float*)G_SHOT=0.0f; tick+=300; st=0;
    for(k=0;k<60;k++){int o; *(u32*)(ch+8)=k; o=ct_policy(ch,2); if(o==7) st++;}
    wr("shot-clock buzzer (game asks applause): standing "); dd(st); wr("/60  buzzers counted "); dd(g_buzz_count); wr("\n"); }
  /* props outside the playoffs: game asks towel (8) at a big moment */
  { u8 ch[0x34]={0}; int k,pr=0,po_pr=0; g_sit=7; tick+=60000;
    for(k=0;k<60;k++){int o; *(u32*)(ch+8)=k; *(u32*)(ch+0x10)=0; o=ct_policy(ch,8); if(o>=8&&o<=11) pr++;}
    C.playoffs=1; for(k=0;k<60;k++){int o; *(u32*)(ch+8)=k; o=ct_policy(ch,8); if(o>=8&&o<=11) po_pr++;} C.playoffs=2;
    wr("big moment, game asks towel: props regular season "); dd(pr); wr("/60, playoffs "); dd(po_pr); wr("/60\n"); }

  /* ===== 1.3 scoreboard events ===== */
  { static int sc0, sc1; static u8 gi[0x40]; u8 ch[0x34]={0}; int k;
    u8 *f;
    f=(u8*)F_SCORE_T0; f[0]=0xA1; *(u32*)(f+1)=(u32)&sc0; f[5]=0xC3;
    f=(u8*)F_SCORE_T1; f[0]=0xA1; *(u32*)(f+1)=(u32)&sc1; f[5]=0xC3;
    f=(u8*)F_GAMEINFO; f[0]=0xB8; *(u32*)(f+1)=(u32)gi; f[5]=0xC3;
    *(unsigned short*)(A_TEAM0+0x74)=13; *(unsigned short*)(A_TEAM1+0x74)=14; *(u32*)(gi+0x34)=14;   /* team B is home */
    *(float*)G_QLEN=720.0f; *(float*)G_FULL=1.0f; *(int*)G_PERIOD=2; *(float*)G_CLOCK=400.0f; *(float*)G_SHOT=20.0f;
    { static u32 gs2[4]; gs2[1]=601; *(u32*)G_SETS=(u32)setm; }
    C.events=1; C.calm_hold=12; C.sad=0.03f; g_sit=2; *(u32*)G_LIVE=1; g_live_since=1; g_game_started=1;
    sc0=40; sc1=42;
#define STEP(ms) do{ int _i; for(_i=0;_i<(ms)/100;_i++){ tick+=100; *(float*)G_CLOCK-= (clk_run?0.1f:0.0f); *(u32*)(ch+8)=0; *(u32*)(ch+0x10)=0; ct_policy(ch,1);} }while(0)
#define MEASURE(label,ask) do{ int c[18]={0},o,_k; for(_k=0;_k<60;_k++){ *(u32*)(ch+8)=_k; *(u32*)(ch+0x10)=_k&1; c[ct_policy(ch,ask)]++; } \
      wr(label); wr(": stand "); dd(c[6]+c[7]+c[5]); wr(" head "); dd(c[4]); wr(" hands-up "); dd(c[3]); wr(" clap "); dd(c[2]); wr(" seated "); dd(c[1]); \
      wr("   ["); wr(G.ev && (int)(G.ev_until-tick)>0 ? k_ev_name[G.ev] : "-"); wr("]\n"); }while(0)
    int clk_run=1;
    STEP(1000); MEASURE("\nnormal play, game asks idle", 1);
    MEASURE("normal play, game asks cheer", 3);
    sc1+=2; STEP(300); MEASURE("home basket", 1);
    STEP(6000); sc1+=3; STEP(300); MEASURE("home three", 1);
    STEP(6000); sc0+=2; STEP(300); MEASURE("away basket, game asks cheer", 3);
    STEP(6000); sc1+=2; g_sit=7; STEP(300); MEASURE("home basket + game 'big play' code (dunk)", 2); g_sit=2;
    STEP(7000); sc0+=2; g_sit=7; STEP(300); MEASURE("away basket + 'big play' code (away dunk)", 2); g_sit=2;
    STEP(7000); sc1+=2; clk_run=0; STEP(1500); MEASURE("home basket, clock stops (and-one)", 1);
    clk_run=1; STEP(7000);
    /* shot clock runs out, home three goes in after it */
    *(float*)G_SHOT=0.3f; STEP(200); *(float*)G_SHOT=0.0f; STEP(300); MEASURE("shot clock horn only", 1);
    STEP(800); sc1+=3; *(float*)G_SHOT=24.0f; STEP(300); MEASURE("three goes in after the shot-clock horn", 1);
    STEP(10000);
    /* end of the 1st... here: 2nd quarter, shot in after the horn */
    *(float*)G_CLOCK=0.5f; STEP(400); STEP(300); MEASURE("end-of-quarter horn only", 1);
    STEP(1000); sc1+=2; STEP(300); MEASURE("2 goes in after the quarter horn", 1);
    STEP(9000);
    /* halftime -> 3rd quarter crowd */
    { int t; float f; *(float*)G_EXCITE=1.0f; f=ct_fill(); wr("halftime start: crowd shown "); dd((int)(f*100+0.5f));
      for(t=0;t<30;t++){ STEP(1000); ct_fill(); } wr("% -> after 30 s of halftime "); dd((int)(ct_fill()*100+0.5f)); wr("%\n");
      *(int*)G_PERIOD=3; *(float*)G_CLOCK=720.0f; STEP(200); wr("3rd quarter: tip "); dd((int)(ct_fill()*100+0.5f));
      *(float*)G_CLOCK=630.0f; STEP(200); ct_fill(); wr("%, 1:30 in "); dd((int)(ct_fill()*100+0.5f));
      *(float*)G_CLOCK=500.0f; STEP(200); ct_fill(); wr("%, 3:40 in "); dd((int)(ct_fill()*100+0.5f));
      *(float*)G_CLOCK=300.0f; STEP(200); ct_fill(); wr("%, 7:00 in "); dd((int)(ct_fill()*100+0.5f)); wr("%\n"); }
    /* 4th quarter: home leads, opponent catching up */
    *(int*)G_PERIOD=4; *(float*)G_CLOCK=400.0f; sc1=100; sc0=90; STEP(300);
    sc0+=2; STEP(300); MEASURE("4th, home +10 -> +8 (opp 2-0 run)", 1); STEP(7000);
    sc0+=3; STEP(300); MEASURE("4th, +8 -> +5 (opp 5-0 run)", 1); STEP(7000);
    sc0+=3; STEP(300); MEASURE("4th, +5 -> +2 (opp 8-0 run)", 1); STEP(7000);
    sc1+=2; STEP(300); MEASURE("4th, home answers (+4), close", 1); STEP(7000);
    sc0+=2; STEP(300); sc0+=3; STEP(300); MEASURE("4th, opp takes the lead (-1)", 1); STEP(7000);
    /* close finish */
    *(float*)G_CLOCK=100.0f; sc0=sc1=100; STEP(300); MEASURE("4th, 1:40 left, tied, game asks idle", 1);
    *(float*)G_CLOCK=20.0f; STEP(300); MEASURE("4th, 0:20 left, tied", 1);
    sc1+=2; STEP(300); MEASURE("4th, 0:20 left, home goes +2", 1); STEP(6000);
    *(float*)G_CLOCK=0.3f; STEP(400); MEASURE("final horn, home wins by 2 (right at the horn)", 1);
    STEP(4000); MEASURE("4 s later", 1); STEP(30000); MEASURE("34 s later", 1);
    /* buzzer beater win (new game) */
    { static u32 nk3[4]; nk3[1]=601; *(u32*)G_SETS=(u32)nk3; }
    *(int*)G_PERIOD=4; sc0=sc1=100; *(float*)G_CLOCK=10.0f; STEP(300);
    *(float*)G_CLOCK=0.3f; STEP(300); STEP(500); sc1+=3; STEP(300); MEASURE("home three in right after the final horn", 1);
    STEP(2800); MEASURE("3 s after the horn: buzzer beater wins the game", 1);
    /* loss */
    { static u32 nk4[4]; nk4[1]=601; *(u32*)G_SETS=(u32)nk4; }
    *(int*)G_PERIOD=4; sc0=103; sc1=100; *(float*)G_CLOCK=5.0f; STEP(300); *(float*)G_CLOCK=0.3f; STEP(500); MEASURE("final horn, home loses by 3", 1);
    /* new game: final, comfortable win */
    { static u32 nk[4]; nk[1]=601; *(u32*)G_SETS=(u32)nk; }   /* different key: new game ... set_of wants 601 */
    *(int*)G_PERIOD=4; *(float*)G_CLOCK=5.0f; sc0=90; sc1=104; STEP(500); *(float*)G_CLOCK=0.3f; STEP(500); STEP(3200); MEASURE("final horn, home wins by 14", 1);
    /* blowout: opponent leads by 22 in the 4th */
    { static u32 nk2[4]; nk2[1]=601; *(u32*)G_SETS=(u32)nk2; }
    *(int*)G_PERIOD=4; *(float*)G_CLOCK=500.0f; sc0=110; sc1=88; *(float*)G_EXCITE=1.0f; STEP(300);
    { int t; float f0=ct_fill(), f1; for(t=0;t<60;t++){ STEP(1000); ct_fill(); } f1=ct_fill();
      wr("4th, opponent +22: crowd shown "); dd((int)(f0*100+0.5f)); wr("% -> after 60 s "); dd((int)(f1*100+0.5f)); wr("%\n"); }
    sc0=95; sc1=100; STEP(300); { float f2; int t; for(t=0;t<10;t++){ STEP(1000); ct_fill(); } f2=ct_fill(); wr("4th, home leads: crowd shown "); dd((int)(f2*100+0.5f)); wr("% (stays, nobody comes back)\n"); }
    C.events=0; *(u32*)G_SETS=(u32)setm;
  }
  /* in-memory gate on a fake stock graph */
  { static u8 recs[601*24]; static u32 gs[4]; int i,n=(int)(sizeof k_g2/sizeof k_g2[0]),okc=0,bad=0;
    for(i=0;i<n;i++){ u8*r=recs+24*k_g2[i].i; *(u32*)r=k_g2[i].anim; *(u32*)(r+0x10)=(u32)k_g2[i].grp<<27; *(u32*)(r+0x14)=k_g2[i].gated?0x10000000u:0; }
    gs[0]=(u32)recs; gs[1]=601; *(u32*)G_SETS=(u32)gs; C.gate_in_memory=1; g_gate_key=0; apply_gate();
    for(i=0;i<n;i++){ u32 y=*(u32*)(recs+24*k_g2[i].i+0x14); if(k_g2[i].gated ? !(y&0x10000000u) : (y&0x10000000u)) okc++; else bad++; }
    wr("in-memory gate: records set "); dd(g_gate_done); wr(", correct "); dd(okc); wr(", wrong "); dd(bad); wr("\n"); }
  sys3(1,0,0,0); return 0;
}
__asm__(".globl _start\n_start:\n call main_\n");
