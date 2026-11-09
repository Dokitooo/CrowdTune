/* stub calling-convention test: compiles crowdtune.c for 32-bit Linux with fake Win32 + fake game functions */
#define CT_MINIMAL_WINAPI
#define readable readable_real
#include "../src/crowdtune.c"
#undef readable
void *memset(void *d,int c,unsigned n){unsigned char*p=d;while(n--)*p++=(unsigned char)c;return d;}
static int sys6(int n,int a,int b,int c,int d,int e,int f){int r;__asm__ volatile("push %%ebp\n mov %7,%%ebp\n int $0x80\n pop %%ebp":"=a"(r):"a"(n),"b"(a),"c"(b),"d"(c),"S"(d),"D"(e),"m"(f):"memory");return r;}
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
HANDLE WINAPI GetCurrentProcess(void){return 0;} DWORD WINAPI GetTickCount(void){return 0;} BOOL WINAPI DisableThreadLibraryCalls(HMODULE a){return 1;}
static char mtx[8][96]; static int nm_, lasterr;
HANDLE WINAPI CreateMutexA(LPVOID a, BOOL b, LPCSTR n){int i; for(i=0;i<nm_;i++) if(!s_cmp(mtx[i],n)){lasterr=183;return (HANDLE)1;} s_cpy(mtx[nm_++],n,96); lasterr=0; return (HANDLE)1;}
DWORD WINAPI GetLastError(void){return lasterr;} DWORD WINAPI GetCurrentProcessId(void){return 0x1234;}
DWORD WINAPI GetFileAttributesA(LPCSTR a){return s_cmp(a,attr_ok)?0xFFFFFFFF:0;}
/* fake game: same calling conventions as 0x9251B0 / 0x924BE0 */
unsigned seen_eax, seen_a1, seen_a2;
__attribute__((naked)) void fake_mood(void){ __asm__ volatile(
  "mov %eax, seen_eax\n mov 4(%esp),%ecx\n mov %ecx, seen_a1\n mov 8(%esp),%ecx\n mov %ecx, seen_a2\n"
  "mov $1, %eax\n mov $0x1111,%ecx\n mov $0x2222,%edx\n ret $8\n"); }
unsigned s_a1,s_a2,s_a3;
__attribute__((naked)) void fake_sit(void){ __asm__ volatile(
  "mov 4(%esp),%ecx\n mov %ecx,s_a1\n mov 8(%esp),%ecx\n mov %ecx,s_a2\n mov 12(%esp),%ecx\n mov %ecx,s_a3\n mov $11,%eax\n ret $12\n"); }
/* game memory the policy reads */
unsigned char chan[0x34] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 5}; static float exc; static int perm[60]; static unsigned set0[2]; static unsigned setp[2];
__attribute__((force_align_arg_pointer)) int main_(void){
  unsigned r_eax, r_ebx, r_esi, r_edi, r_ebp;
  int i;
  load_ini(); C.enabled=1;
  /* point the policy at our fake memory by remapping the constants: this test only checks the stub ABI,
     so policy is exercised with set_of() returning -1 (unknown set) -> group passes through unchanged */
  g_f_mood=(u32)fake_mood; g_f_sit=(u32)fake_sit;
  /* caller like 0x92664E: eax=state, push edx, push esi, call */
  __asm__ volatile(
    "push %%ebp\n mov $0x77,%%eax\n mov $0xAAAA,%%ebx\n mov $chan,%%esi\n mov $0xCCCC,%%edi\n mov $0x5555,%%ebp\n"
    "push $0xDDDD\n push %%esi\n call ct_mood_stub\n"
    "mov %%ebp,%%ecx\n pop %%ebp\n"
    : "=a"(r_eax), "=b"(r_ebx), "=S"(r_esi), "=D"(r_edi), "=c"(r_ebp) :: "edx", "memory");
  wr("mood: fake saw eax,a1,a2 / after eax,ebx,esi,edi,ebp: "); hx(seen_eax);hx(seen_a1);hx(seen_a2);wr("/ ");hx(r_eax);hx(r_ebx);hx(r_esi);hx(r_edi);hx(r_ebp);wr("\n");
  int ok1 = seen_eax==0x77 && seen_a1==(unsigned)chan && seen_a2==0xDDDD && r_ebx==0xAAAA && r_esi==(unsigned)chan && r_eax==1 && r_edi==0xCCCC && r_ebp==0x5555;
  unsigned rv;
  __asm__ volatile("push $3\n push $2\n push $1\n call ct_sit_stub\n" : "=a"(rv) :: "ecx","edx","memory");
  wr("sit: ret,args,recorded: "); hx(rv);hx(s_a1);hx(s_a2);hx(s_a3);hx(g_sit);wr("\n");
  int ok2 = rv==11 && s_a1==1 && s_a2==2 && s_a3==3 && g_sit==11;
  s_cpy(g_dir,"G:\\game\\",260);
  int m1=mc_first("CrowdTune"), m2=mc_first("CrowdTune"), m3=mc_first("SigExt");
  wr("guard: first="); hx(m1); wr("second="); hx(m2); wr("other mod="); hx(m3); wr("\n");
  int ok3 = m1==1 && m2==0 && m3==1;
  nl=0; attr_ok[0]=0; mc_load_chain("xinput1_3"); wr("chain (none): "); wr(loaded[0]); wr("\n");
  int ok4 = !s_cmp(loaded[0],"C:\\Windows\\System32\\xinput1_3.dll");
  nl=0; s_cpy(attr_ok,"G:\\game\\xinput1_3_next.dll",260); mc_load_chain("xinput1_3"); wr("chain (_next present): "); wr(loaded[0]); wr("\n");
  int ok5 = !s_cmp(loaded[0],"G:\\game\\xinput1_3_next.dll");
  nl=0; s_cpy(attr_ok,"G:\\game\\xinput1_3_chain.dll",260); mc_load_chain("xinput1_3"); wr("chain (_chain present): "); wr(loaded[0]); wr("\n");
  int ok6 = !s_cmp(loaded[0],"G:\\game\\xinput1_3_chain.dll");
  ok2 = ok2 && ok3 && ok4 && ok5 && ok6;

  /* fill stub: caller like 0x92376F - result in st(0), every register and xmm0/xmm1 unchanged */
  { u8*img=(u8*)sys6(192,0x1200000,0xD00000,3,0x32,-1,0); float f=0; unsigned a1,b1,c1,d1,x0;
    if((u32)img!=0x1200000){wr("mmap fail\n");}
    *(float*)G_EXCITE=1.0f; *(u32*)G_SETS=0x1234; C.playoffs=2; C.attendance=0.85f;
    __asm__ volatile("mov $0x11,%%eax\n mov $0x22,%%ecx\n mov $0x33,%%edx\n mov $0x44,%%esi\n movd %%esi,%%xmm0\n"
                     "call ct_fill_stub\n fstps %0\n movd %%xmm0,%%edi\n"
                     : "=m"(f), "=a"(a1), "=c"(b1), "=d"(c1), "=S"(d1), "=D"(x0) :: "memory");
    wr("fill: eax,ecx,edx,esi,xmm0 after: "); hx(a1);hx(b1);hx(c1);hx(d1);hx(x0); wr(" value*1000="); hx((unsigned)(f*1000.0f+0.5f)); wr("\n");
    ok2 = ok2 && a1==0x11 && b1==0x22 && c1==0x33 && d1==0x44 && x0==0x44 && (unsigned)(f*1000.0f+0.5f)==850; }
  wr(ok1&&ok2 ? "STUB ABI OK\n" : "STUB ABI FAIL\n");
  sys3(1,!(ok1&&ok2),0,0); return 0;
}
__asm__(".globl _start\n_start:\n call main_\n");
