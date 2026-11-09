/*
 * CrowdTune - calmer, more realistic regular-season crowd for NBA 2K14 (PC, RLD build).
 *
 * How the 2K14 crowd works (reverse-engineered from nba2k14.exe):
 *   - The arena runs 60 shared animation "channels"; every seat follows one of them (the
 *     seat -> channel mapping is random and reshuffled every 30 s).
 *   - Each channel plays clips from the crowd.iff behaviour graph (crowdren1 = men,
 *     crowdfemren1 = women). Every clip belongs to a mood group: idle, applause, cheer,
 *     disappointed, props (towel / thunder sticks / foam finger / sign) or chant.
 *   - When a channel's clip ends, the game decides the next mood group (0x9251B0) from the
 *     game situation (0x924BE0) using hard-coded fractions, e.g. a routine "applause" moment
 *     = 50% clapping + 50% STANDING idle. Men and women are mirrored (when one stands the
 *     other sits), so a big part of the arena is always on its feet.
 *   - The game's own excitement level (0x12FD788, about 0.5 .. 1.0) comes from score margin,
 *     game clock, momentum and crowd noise.
 *
 *   - The crowd level (0x12FD788, called "excitement" in older versions) is set from the home
 *     team's popularity (0.75 .. 1.0); the renderer shows a seat when its random byte is below
 *     level * 255 (0x9234F0). It is 1.0 at a Lakers home game: every seat is filled.
 *   - Arena load (0x926D20/0x926C10): every channel starts in mood group [0x12FD88C] = 7 (standing
 *     cheer), or 6 (standing idle) in some phases - the "everybody stands, then sits" at the start.
 *   - Crowd clips have no blending: a channel changes clips at loop ends through the authored
 *     transition clips of the graph, and holds a loop for the node's (min, max) time first.
 *
 * CrowdTune lets the game decide as before, then re-maps the chosen mood per channel:
 *   normal play -> everybody seated, quiet; a seated fan stays seated for CalmHoldSeconds
 *   scoring     -> seated applause from a minority, a few seated hands up
 *   big moment  -> standing share, seated cheering, applause; props / chants only in the playoffs
 *   buzzers     -> period end / shot-clock violation (game and shot clock reach 0): a reaction
 * Everything is set in CrowdTune.ini. CrowdTune.log records what the game asked for.
 *
 * Patched (each checked byte-for-byte; nothing is patched on another exe):
 *   0x92664E  call 0x9251B0  (next mood group for a channel)   -> ct_mood_stub
 *   0x9251FE  call 0x924BE0  (game situation for the crowd)     -> ct_sit_stub (records it)
 *   0x925CD6  movss xmm0,[0xEC703C] (0.6) -> reads our own threshold (energetic applause clips)
 *   0x92376F, 0x9240E8  call 0x925850 (crowd level for the renderer) -> ct_fill_stub (Attendance)
 *   0x12FD88C int[2] {7,6} -> {1,1}: arenas load with everybody seated
 *   in memory, on every arena load: the men's applause records of the stock graph get the same
 *   "big moments only" marks the 1.1 crowd.iff patch had (graph_gate.inc) - stock crowd.iff works.
 *
 * SLAM mod (needs SLAM 1.0+): SLAM\CrowdTune\CrowdTune.asi + CrowdTune.ini, log SLAM\CrowdTune\CrowdTune.log.
 * Every byte change above goes through SLAM's api->patch (registered with CrowdTune as owner, conflict-checked).
 *
 * Build: build/build_linux_clang.sh (clang-cl + lld-link). slam_glue.c holds DllMain and SlamModStart; this
 * file starts in mod_start().
 */
#ifdef _MSC_VER
#  ifndef CT_MINIMAL_WINAPI
#    include <windows.h>
#  else
#    include "win_min.h"
#  endif
#else
#  include "win_min.h"
#endif

typedef unsigned char u8; typedef unsigned int u32;
int _fltused = 0;   /* no CRT: the linker wants this symbol when floats are used */
#define VERSION "1.0"
#include "slam_mod.h"

/* ---------------- game addresses (RLD nba2k14.exe) ---------------- */
#define A_MOOD_CALL   0x92664Eu   /* e8 5d eb ff ff   call 9251b0 */
#define A_SIT_CALL    0x9251FEu   /* e8 dd f9 ff ff   call 924be0 */
#define F_MOOD        0x9251B0u
#define F_SIT         0x924BE0u
#define G_EXCITE      0x12FD788u  /* float */
#define G_PERM        0x12FD798u  /* int[60]: random permutation, one rank per channel */
#define G_SETS        0x1E20878u  /* behaviour graph per actor set: [0] men, [1] women */
#define NCH 60
#define G_LIVE        0x12FD790u  /* int: 1 while the crowd follows live play (set by 0x9248C0) */
#define G_MODE        0x1E85CD4u  /* int: game / presentation phase */
#define A_FILL_CALL1  0x92376Fu   /* e8 dc 20 00 00   call 925850 (per frame: crowd fill -> visible share)   */
#define A_FILL_CALL2  0x9240E8u   /* e8 63 17 00 00   call 925850 (arena load: same)                          */
#define G_START_GRP   0x12FD88Cu  /* int[2] = {7, 6}: mood group every channel starts in when the arena loads */
#define G_CLOCK       0x1DEE638u  /* float: game clock, seconds left in the period */
#define G_SHOT        0x1DEE654u  /* float: shot clock (reset to 24.0 / 14.0)        */
#define G_PERIOD      0x1DEE5ECu  /* int: 1..4 quarters, 5+ overtime */
#define G_QLEN        0x1DEE884u  /* float: length of a period in seconds (quarter-length setting) */
#define G_FULL        0x12FD770u  /* float: the game's full-house crowd level for this game */
#define F_SCORE_T0    0x882310u   /* int(void): points of team 0x1F1F730 */
#define F_SCORE_T1    0x882340u   /* int(void): points of team 0x1F1FA28 */
#define F_GAMEINFO    0x4A8350u   /* game settings; +0x34 low 11 bits = arena (home) team id */
#define A_TEAM0       0x1F1F730u  /* +0x74 word: team id */
#define A_TEAM1       0x1F1FA28u
#define F_GAMEMODE    0x4AE270u
#define F_SCHED_GAME  0x60EE70u
#define F_SERIES      0x493D40u
#define F_M0_A        0x491110u
#define F_M0_B        0xA28A30u
#define F_M0_C        0x493BF0u
#define A_GATE        0x925CD6u   /* f3 0f 10 05 3c 70 ec 00  movss xmm0,[0xEC703C] (0.6): excitement needed for gated clips */

static const u8 k_mood_call[5] = { 0xE8, 0x5D, 0xEB, 0xFF, 0xFF };
static const u8 k_sit_call[5]  = { 0xE8, 0xDD, 0xF9, 0xFF, 0xFF };
static const u8 k_fill1[5] = { 0xE8, 0xDC, 0x20, 0x00, 0x00 };
static const u8 k_fill2[5] = { 0xE8, 0x63, 0x17, 0x00, 0x00 };
static const u8 k_gate[8] = { 0xF3, 0x0F, 0x10, 0x05, 0x3C, 0x70, 0xEC, 0x00 };
static float g_gate = 0.80f;

static char g_ini[MAX_PATH];

/* ---------------- tiny libc ---------------- */
static int s_len(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void s_cpy(char *d, const char *s, int n) { int i = 0; while (s[i] && i < n - 1) { d[i] = s[i]; i++; } d[i] = 0; }
static void s_cat(char *d, const char *s, int n) { int l = s_len(d); s_cpy(d + l, s, n - l); }
static void u2d(u32 v, char *b) { char t[12]; int n = 0, i; do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v); for (i = 0; i < n; i++) b[i] = t[n - 1 - i]; b[n] = 0; }
static void f2s(float f, char *b)             /* 3 decimals, no CRT */
{
    int neg = f < 0; u32 m; char t[16];
    if (neg) f = -f;
    m = (u32)(f * 1000.0f + 0.5f);
    b[0] = 0; if (neg) s_cat(b, "-", 16);
    u2d(m / 1000, t); s_cat(b, t, 16); s_cat(b, ".", 16);
    t[0] = (char)('0' + (m / 100) % 10); t[1] = (char)('0' + (m / 10) % 10); t[2] = (char)('0' + m % 10); t[3] = 0;
    s_cat(b, t, 16);
}
static float s2f(const char *s, float dflt)   /* "0.35" / "-1" / ".5" */
{
    float v = 0, sc = 1; int neg = 0, any = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { neg = 1; s++; } else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (float)(*s - '0'); s++; any = 1; }
    if (*s == '.') { s++; while (*s >= '0' && *s <= '9') { sc *= 0.1f; v += sc * (float)(*s - '0'); s++; any = 1; } }
    return any ? (neg ? -v : v) : dflt;
}

static int cfg_log = 1;
/* CrowdTune.log in the mod's SLAM folder (SLAM adds the time) */
static void logs(const char *a, const char *b, const char *c)
{
    if (!cfg_log) return;
    g_slam->log(g_slamMe, "%s%s%s", a ? a : "", b ? b : "", c ? c : "");
}
static void logd(const char *a, u32 v, const char *c) { char b[12]; u2d(v, b); logs(a, b, c); }

static int readable(const void *p, u32 n)
{
    MEMORY_BASIC_INFORMATION mi;
    if (!VirtualQuery(p, &mi, sizeof mi) || mi.State != MEM_COMMIT) return 0;
    if (mi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return 0;
    return (u32)p + n <= (u32)mi.BaseAddress + (u32)mi.RegionSize;
}
static int same(const u8 *a, const u8 *b, int n) { int i; for (i = 0; i < n; i++) if (a[i] != b[i]) return 0; return 1; }


/* ---------------- settings (CrowdTune.ini) ---------------- */
static struct {
    int   enabled;
    float exc_calm, exc_big;      /* game excitement: below calm = quiet game, at big = full reaction */
    float stand_idle;             /* calm play: share of fans standing idle */
    float clap_calm, clap_good;   /* applause share for a routine / good play (seated) */
    float cheer_good;             /* good play: share doing a seated cheer */
    float stand_good;             /* good play: share standing up (keep small: routine baskets) */
    float stand_big_min, stand_big_max;   /* big moment: standing share from ... to (scales with excitement) */
    float seat_cheer_big;         /* big moment: extra share cheering seated */
    float props;                  /* big moment: share allowed to wave props (towel/sticks/finger/sign) */
    float sad;                    /* opponent's big play: share showing disappointment (seated) */
    int   big_sit[8];             /* game situation codes treated as a high-emotion moment (-1 = unused) */
    int   score_sit[8];           /* game situation codes treated as a scoring reaction */
    int   use_exc, allow_stand;
    int   log_every;              /* seconds between log lines (0 = off) */
    float gate;                   /* excitement needed for clips the crowd.iff marks as 'excited only' */
    int   calm_outside;           /* 1 = intros, pre-game, tip-off ceremony, breaks: crowd stays calm */
    int   tipoff_calm;            /* seconds of live play at the start of a game kept calm (tip-off) */
    int   resume_calm;            /* seconds kept calm whenever play resumes after a break */
    float attendance;             /* regular season: share of the game's crowd that shows up (1 = stock) */
    float attendance_po;          /* playoffs */
    int   start_seated;           /* arena loads with everybody seated (stock: standing) */
    int   calm_hold;              /* normal play: seconds a fan stays in his seat pose before another mood */
    int   buzzer_secs;            /* seconds of reaction after a period-end / shot-clock buzzer (0 = off) */
    float buzzer_stand;           /* standing share at a buzzer */
    int   playoffs;               /* 0 = detect, 1 = always treat as playoffs, 2 = never */
    int   gate_in_memory;         /* apply the 'big moments only' applause marks to the stock graph in memory */
    int   events;                 /* 1 = react to the scoreboard (home baskets, and-1s, buzzers, close games, wins) */
    int   close_margin;           /* close game: score difference at most this ... */
    int   close_secs;             /* ... in the last this many seconds of the 4th / overtime (12-minute quarters) */
    float close_stand;            /* standing share at the very end of a close game */
    int   leave_lead;             /* 4th quarter: fans start leaving when the opponent leads by this much */
    float leave_per_point;        /* share leaving per point of opponent lead beyond leave_lead */
    float leave_max;              /* at most this share leaves */
    int   home;                   /* 0 = detect home team, 1 = team A, 2 = team B (log shows which) */
    float q3_start, q3_filled;      /* 3rd quarter: share back in their seats at the start, full by this share of the quarter */
} C;

static int read_int(const char *k, int d) { return (int)GetPrivateProfileIntA("Crowd", k, d, g_ini); }
static float read_f(const char *k, float d)
{
    char b[64]; GetPrivateProfileStringA("Crowd", k, "", b, sizeof b, g_ini); return s2f(b, d);
}
static void read_list(const char *key, const char *dflt, int *out)
{
    char b[128]; const char *p; int n = 0, i;
    for (i = 0; i < 8; i++) out[i] = -1;
    GetPrivateProfileStringA("Crowd", key, dflt, b, sizeof b, g_ini);
    for (p = b; *p && n < 8;) {
        while (*p == ' ' || *p == ',') p++;
        if (*p < '0' || *p > '9') break;
        { int v = 0; while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; } out[n++] = v; }
    }
}
static void load_ini(void)
{
    char b[128]; const char *p; int n = 0, i;
    cfg_log = (int)GetPrivateProfileIntA("Settings", "Log", 1, g_ini);
    C.enabled = (int)GetPrivateProfileIntA("Settings", "Enabled", 1, g_ini);
    C.exc_calm = read_f("ExcitementCalm", 0.62f);
    C.exc_big = read_f("ExcitementBig", 0.85f);
    C.stand_idle = read_f("StandingIdle", 0.0f);
    C.clap_calm = read_f("ApplauseNormalPlay", 0.0f);
    C.clap_good = read_f("ApplauseScore", 0.45f);
    C.cheer_good = read_f("HandsUpScore", 0.08f);
    C.stand_good = read_f("StandGood", 0.05f);
    C.stand_big_min = read_f("StandBigMin", 0.40f);
    C.stand_big_max = read_f("StandBig", 0.50f);
    C.seat_cheer_big = read_f("HandsUpBig", 0.20f);
    C.props = read_f("PropsBig", 0.06f);
    C.sad = read_f("Disappointed", 0.03f);
    C.log_every = read_int("LogEverySeconds", 2);
    C.gate = read_f("ExcitedApplauseFrom", 0.80f);
    g_gate = C.gate;
    C.calm_outside = read_int("CalmOutsidePlay", 1);
    C.tipoff_calm = read_int("TipOffCalmSeconds", 45);
    C.resume_calm = read_int("ResumeCalmSeconds", 10);
    read_list("BigSituations", "7,8,11", C.big_sit);
    read_list("ScoreSituations", "3", C.score_sit);
    C.use_exc = read_int("UseExcitement", 0);
    C.allow_stand = read_int("StandOnBigMoments", 1);
    C.attendance = read_f("Attendance", 0.85f);
    C.attendance_po = read_f("PlayoffAttendance", 1.0f);
    C.start_seated = read_int("StartSeated", 1);
    C.calm_hold = read_int("CalmHoldSeconds", 12);
    C.buzzer_secs = read_int("BuzzerSeconds", 7);
    C.buzzer_stand = read_f("BuzzerStand", 0.35f);
    C.playoffs = read_int("Playoffs", 0);
    C.gate_in_memory = read_int("BigMomentApplauseOnly", 1);
    C.events = read_int("ScoreboardReactions", 1);
    C.close_margin = read_int("CloseGameMargin", 5);
    C.close_secs = read_int("CloseGameSeconds", 120);
    C.close_stand = read_f("CloseGameStand", 0.80f);
    C.leave_lead = read_int("LeaveWhenTrailingBy", 15);
    C.leave_per_point = read_f("LeavePerPoint", 0.02f);
    C.leave_max = read_f("LeaveMax", 0.30f);
    C.home = read_int("HomeTeam", 0);
    C.q3_start = read_f("ThirdQuarterStart", 0.55f);
    C.q3_filled = read_f("ThirdQuarterFullAfter", 0.50f);
    if (C.q3_start < 0.3f) C.q3_start = 0.3f;
    if (C.q3_start > 1.0f) C.q3_start = 1.0f;
    if (C.q3_filled < 0.05f) C.q3_filled = 0.05f;
    if (C.attendance < 0.3f) C.attendance = 0.3f;
    if (C.attendance > 1.0f) C.attendance = 1.0f;
    if (C.attendance_po < 0.3f) C.attendance_po = 0.3f;
    if (C.attendance_po > 1.0f) C.attendance_po = 1.0f;
    if (C.exc_big <= C.exc_calm) C.exc_big = C.exc_calm + 0.01f;
}

/* ---------------- mood classes ----------------
 * Group numbers (clip field >> 27), the same for men and women (men's clip names: st_ = seated,
 * sd_ = standing - sit2std_fast/std2sit1 connect the two halves of the graph; 1.1.0 had men swapped):
 *            idle sit/stand  applause  cheer sit/stand  sad sit/stand  props  chants
 *   both       1 / 6           2          3 / 7            4 / 5        8-11  15-17   */
enum { K_OTHER, K_IDLE, K_CLAP, K_CHEER, K_SAD, K_PROP, K_CHANT };
static const int k_class[2][18] = {
    /* 0      1       2       3        4      5      6       7        8       9       10      11      12       13       14       15       16       17 */
    { K_OTHER, K_IDLE, K_CLAP, K_CHEER, K_SAD, K_SAD, K_IDLE, K_CHEER, K_PROP, K_PROP, K_PROP, K_PROP, K_OTHER, K_OTHER, K_OTHER, K_CHANT, K_CHANT, K_CHANT },
    { K_OTHER, K_IDLE, K_CLAP, K_CHEER, K_SAD, K_SAD, K_IDLE, K_CHEER, K_PROP, K_PROP, K_PROP, K_PROP, K_OTHER, K_OTHER, K_OTHER, K_CHANT, K_CHANT, K_CHANT },
};
/*                               idle_sit idle_stand clap cheer_sit cheer_stand sad_sit */
static const int k_grp[2][6] = { { 1, 6, 2, 3, 7, 4 }, { 1, 6, 2, 3, 7, 4 } };
enum { O_IDLE_SD, O_IDLE_ST, O_CLAP, O_CHEER_SD, O_CHEER_ST, O_SAD_SD };

static int set_of(const u8 *ch)      /* 0 men, 1 women, -1 unknown set (special modes: leave alone) */
{
    u32 s = *(const u32 *)(ch + 0x10), p, n;
    if (s > 1) return -1;
    p = *(const u32 *)(G_SETS + 4 * s);
    if (!p || !readable((void *)p, 8)) return -1;
    n = *(const u32 *)(p + 4);
    if (s == 0 && n == 601) return 0;          /* stock crowdren1 */
    if (s == 1 && n == 93) return 1;           /* stock crowdfemren1 */
    return -1;
}

/* ---------------- statistics for the log ---------------- */
static volatile u32 g_sit, g_sit_hist[32], g_in_hist[2][18], g_out_hist[2][18], g_calls;
static float g_exc_min = 9, g_exc_max = -9;

static float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

/* calm phases: everything that is not live play (intros, introductions, tip-off ceremony, breaks),
 * the first TipOffCalmSeconds of live play in a game and ResumeCalmSeconds after every break.
 * A new game = the crowd behaviour sets were (re)loaded, or no live play for 3+ minutes. */
static u32 g_live_since, g_last_live, g_set_ptr, g_game_started, g_calm_reason;
static int calm_phase(void)
{
    u32 now = GetTickCount(), live = *(const u32 *)G_LIVE, sp = *(const u32 *)G_SETS;
    if (sp != g_set_ptr) { g_set_ptr = sp; g_game_started = 0; g_live_since = 0; }
    if (!live) {
        if (g_live_since) g_last_live = now;
        g_live_since = 0;
        if (g_last_live && now - g_last_live > 180000u) g_game_started = 0;
        g_calm_reason = 1;
        return C.calm_outside;
    }
    if (!g_live_since) g_live_since = now;
    if (!g_game_started) {
        if (now - g_live_since < (u32)C.tipoff_calm * 1000u) { g_calm_reason = 2; return 1; }
        g_game_started = 1;
    }
    if (now - g_live_since < (u32)C.resume_calm * 1000u) { g_calm_reason = 3; return 1; }
    g_calm_reason = 0;
    return 0;
}

static int in_list(const int *l, u32 v) { int i; for (i = 0; i < 8; i++) if (l[i] >= 0 && (u32)l[i] == v) return 1; return 0; }
static volatile u32 g_level_seen;

/* ---------------- playoffs ----------------
 * The game's own "is this a series game" test (inside 0xA2CA40): game type 1-3 -> the scheduled
 * game's playoff series; game type 0 only with the season checks first. Evaluated once per arena
 * load on the game thread (inside the crowd update), never from our own thread. */
static u32 g_f_gamemode = F_GAMEMODE, g_f_sched = F_SCHED_GAME, g_f_series = F_SERIES;
static u32 g_f_m0a = F_M0_A, g_f_m0b = F_M0_B, g_f_m0c = F_M0_C;
__declspec(naked) static int game_is_playoff(void)
{
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        call dword ptr [g_f_gamemode]
        test eax, eax
        jnz have_mode
        call dword ptr [g_f_m0a]
        test eax, eax
        jz no
        call dword ptr [g_f_m0b]
        test eax, eax
        jnz no
        xor ecx, ecx
        call dword ptr [g_f_m0c]
        test eax, eax
        jz no
    have_mode:
        call dword ptr [g_f_sched]
        test eax, eax
        jz no
        mov ecx, eax
        call dword ptr [g_f_series]
        test eax, eax
        jz no
        mov eax, 1
        jmp done
    no:
        xor eax, eax
    done:
        pop ebp
        pop edi
        pop esi
        pop ebx
        ret
    }
}
static u32 g_po_key = 1; static int g_po;          /* cached per arena load (behaviour-set pointer) */
static int playoffs(void)
{
    u32 k = *(const u32 *)G_SETS;
    if (C.playoffs == 1) return 1;
    if (C.playoffs == 2) return 0;
    if (k != g_po_key) { g_po_key = k; g_po = game_is_playoff(); }
    return g_po;
}


/* ---------------- the game: scoreboard, clock, period ----------------
 * Everything is read on the game thread (from the crowd update), at most every 100 ms. */
static u32 g_f_gameinfo = F_GAMEINFO;
__declspec(naked) static u32 __cdecl gcall0(u32 f)
{
    __asm {
        push ebx
        push esi
        push edi
        push ebp
        mov eax, [esp+20]
        call eax
        pop ebp
        pop edi
        pop esi
        pop ebx
        ret
    }
}
enum { EV_NONE, EV_AWAY_FG, EV_HOME_FT, EV_HOME_2, EV_HOME_3, EV_AWAY_BIG, EV_UPSET, EV_HOME_BIG, EV_AND1,
       EV_AWAY_WOW, EV_WOW, EV_WOW3, EV_LOSS, EV_WIN, EV_STUNNED, EV_BEATER_TIE, EV_BEATER_WIN, EV_N };
static const char *const k_ev_name[EV_N] = { "-", "away basket", "home free throw", "home basket", "home three",
    "away dunk/and-one (disbelief)", "UPSET: opponent catching up", "home dunk (amazed)", "home AND-ONE (amazed)", "away shot after the horn (disbelief)",
    "WOW: home shot after the horn", "WOW: home three after the horn", "home loss", "HOME WIN", "stunned",
    "BUZZER BEATER TIE", "BUZZER BEATER WIN" };
/* shares of the arena: standing cheer, seated hands-on-head (disbelief), seated hands up / fist pump
   (amazed), applause; big = the energetic clips (high claps, hands up, standing up while clapping) allowed */
typedef struct { float stand, head, hup, clap; u32 ms; int big; float upset; } MIX;   /* upset: every disappointed pose (shaking heads, arms crossed, yelling) */
static MIX k_mix[EV_N] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0.00f, 0.04f, 0.00f, 0.00f,  5000, 0 },   /* away basket: quiet                                   */
    { 0.00f, 0.00f, 0.00f, 0.25f,  3000, 0 },   /* home free throw                                       */
    { 0.00f, 0.00f, 0.06f, 0.45f,  5000, 0 },   /* home basket: calm seated claps                        */
    { 0.00f, 0.00f, 0.12f, 0.55f,  5000, 0 },   /* home three                                            */
    { 0.00f, 0.50f, 0.00f, 0.00f,  7000, 0 },   /* away dunk / and-one: disbelief, hands on / behind heads */
    { 0.00f, 0.00f, 0.00f, 0.00f,  7000, 0 },   /* opponent catching up in the 4th (shares set in fire) */
    { 0.30f, 0.00f, 0.20f, 0.40f,  6000, 1 },   /* home dunk: amazed - up on their feet, hands up, claps */
    { 0.35f, 0.00f, 0.25f, 0.30f,  6000, 1 },   /* home and-one: amazed                                  */
    { 0.00f, 0.55f, 0.00f, 0.00f,  8000, 0 },   /* away shot in after the quarter / shot-clock horn: disbelief */
    { 0.40f, 0.00f, 0.25f, 0.30f,  8000, 1 },   /* home shot goes in after the horn: wow                 */
    { 0.55f, 0.00f, 0.25f, 0.20f,  9000, 1 },   /* home three after the horn: bigger wow                 */
    { 0.00f, 0.20f, 0.00f, 0.00f, 40000, 0, 0.70f }, /* home loss: everybody upset                        */
    { 0.55f, 0.00f, 0.15f, 0.25f, 40000, 1 },   /* home win (shares set by the margin)                   */
    { 0.00f, 0.30f, 0.00f, 0.00f, 20000, 0, 0.55f }, /* opponent's buzzer beater wins / ties it          */
    { 0.80f, 0.00f, 0.10f, 0.10f, 20000, 1 },   /* home buzzer beater ties the game                      */
    { 0.90f, 0.00f, 0.08f, 0.02f, 40000, 1 },   /* home buzzer beater wins the game                      */
};
static struct {
    u32 key, next, home_ok;
    int home;                       /* 0: team A (0x1F1F730) is home, 1: team B */
    int hs, as, valid, scores_ok;   /* home / away points */
    int period;
    float clock, shot, qlen;
    int clock_running;
    u32 last_home_fg, last_away_fg, fg_check_at, afg_check_at; float fg_clock, afg_clock; int fg_pts;
    u32 end_at, shot0_at; int end_period, beater_pending, beater_pts;
    u32 salt; int away_run, hs_before, as_before, was_close;
    int ev; u32 ev_until; MIX mix;
    float tense;                    /* 0..1: how tense a close finish is */
    float q4_fill; int q4_init;
    u32 events_seen;
} G;
static void fire(int ev, u32 now)
{
    if (ev <= EV_NONE || ev >= EV_N) return;
    if (G.ev && (int)(G.ev_until - now) > 0 && ev < G.ev) return;   /* a bigger reaction is running */
    G.ev = ev; G.mix = k_mix[ev]; G.ev_until = now + k_mix[ev].ms; G.events_seen++;
    G.salt = (G.salt + 17u + (now % 41u)) % 60u;       /* a different part of the arena reacts each time */
    if (ev == EV_WIN) {
        int m = G.hs - G.as;
        if (m <= 3 || G.was_close) { G.mix.stand = 0.85f; G.mix.hup = 0.10f; G.mix.clap = 0.05f; }   /* close win: everybody up */
        else if (m >= 10) { G.mix.stand = 0.40f; G.mix.hup = 0.10f; G.mix.clap = 0.45f; }           /* comfortable win */
    }
    if (ev == EV_UPSET) {   /* the opponent closes in during the 4th: the bigger the run, the more upset */
        float u = 0.15f + 0.03f * (float)G.away_run;
        if (G.as > G.hs && G.hs_before >= G.as_before) u += 0.25f;     /* they just took the lead */
        else if (G.as == G.hs) u += 0.12f;                              /* they tied it */
        if (u > 0.6f) u = 0.6f;
        G.mix.upset = u; G.mix.head = 0.10f;
    }
    if ((ev == EV_HOME_2 || ev == EV_HOME_3) && G.period >= 4) {      /* 4th quarter: louder, more so when it's close */
        int m = G.hs - G.as; if (m < 0) m = -m;
        if (m <= 8) { G.mix.stand = ev == EV_HOME_3 ? 0.25f : 0.15f; G.mix.hup += 0.06f; G.mix.clap = 0.45f; }
        else { G.mix.hup += 0.04f; G.mix.clap += 0.10f; }
    }
}
static int home_team(void)
{
    if (C.home == 1) return 0;
    if (C.home == 2) return 1;
    if (!G.home_ok) {
        u32 gi = gcall0(g_f_gameinfo); int arena;
        if (gi && readable((void *)gi, 0x38)) {
            arena = (int)((*(const u32 *)(gi + 0x34)) << 21) >> 21;
            if (arena == (int)*(const unsigned short *)(A_TEAM1 + 0x74)) { G.home = 1; G.home_ok = 1; }
            else if (arena == (int)*(const unsigned short *)(A_TEAM0 + 0x74)) { G.home = 0; G.home_ok = 1; }
            else { G.home = 0; G.home_ok = 2; }    /* neutral floor / unknown: team A */
        }
    }
    return G.home;
}
static void game_update(u32 now)
{
    u32 k = *(const u32 *)G_SETS; int t0, t1, hs, as, dh, da; float c, sc;
    if (!C.events) return;
    if (k != G.key) {   /* new arena load = new game */
        int i; u8 *z = (u8 *)&G; for (i = 0; i < (int)sizeof G; i++) z[i] = 0;
        G.key = k;
    }
    if (G.next && (int)(G.next - now) > 0) return;
    G.next = now + 100;
    c = *(const float *)G_CLOCK; sc = *(const float *)G_SHOT;
    G.period = *(const int *)G_PERIOD; G.qlen = *(const float *)G_QLEN;
    if (G.qlen < 60.0f || G.qlen > 720.0f) G.qlen = 720.0f;
    G.clock_running = c < G.clock - 0.001f;
    t0 = (int)gcall0(F_SCORE_T0); t1 = (int)gcall0(F_SCORE_T1);
    if (home_team()) { hs = t1; as = t0; } else { hs = t0; as = t1; }
    if (hs < 0 || as < 0 || hs > 400 || as > 400) { G.valid = 0; return; }
    if (hs || as) G.scores_ok = 1;
    dh = G.valid ? hs - G.hs : 0; da = G.valid ? as - G.as : 0;
    G.hs_before = G.hs; G.as_before = G.as;
    if (dh < 0 || dh > 4 || da < 0 || da > 4) dh = da = 0;          /* resync (new game, replay, edits) */
    G.hs = hs; G.as = as; G.valid = 1;

    /* horns: the end of a period, the shot clock running out. The horn itself gets no reaction - a
       shot released before it that goes in after it does (the score changes up to 3 s later). */
    if (G.clock > 0.0f && c <= 0.0f && c > -1.0f) {
        G.end_at = now; G.end_period = G.period;
        G.beater_pending = (G.last_home_fg && now - G.last_home_fg < 400) ? 1 : (G.last_away_fg && now - G.last_away_fg < 400) ? -1 : 0;
        if (G.beater_pending == 1) G.beater_pts = G.fg_pts;
        if (G.period >= 4 && hs != as) fire(hs > as ? EV_WIN : EV_LOSS, now);   /* the final horn: no sitting down first */
    } else if (G.shot > 0.0f && sc <= 0.0f && sc > -1.0f && c > 0.5f) G.shot0_at = now;

    /* baskets */
    if (dh >= 2) {
        G.last_home_fg = now; G.last_away_fg = 0; G.fg_clock = c; G.fg_pts = dh; G.fg_check_at = now + 1200;
        if (G.end_at && now - G.end_at < 3000) {                        /* in after the period horn */
            G.beater_pending = 1; G.beater_pts = dh;
            fire(dh == 3 ? EV_WOW3 : EV_WOW, now);                   /* 4th / OT: upgraded below if it wins or ties */
        } else if (G.shot0_at && now - G.shot0_at < 3000) {             /* in after the shot clock ran out */
            fire(dh == 3 ? EV_WOW3 : EV_WOW, now); G.shot0_at = 0; G.fg_check_at = 0;
        } else fire(dh == 3 ? EV_HOME_3 : EV_HOME_2, now);
    } else if (dh == 1) fire(EV_HOME_FT, now);
    if (dh > 0) G.away_run = 0;
    if (da > 0) G.away_run += da;
    if (da >= 2) {
        G.last_home_fg = 0; G.last_away_fg = now; G.afg_clock = c; G.afg_check_at = now + 1200;
        if (G.end_at && now - G.end_at < 3000) { G.beater_pending = -1; fire(EV_AWAY_WOW, now); }
        else if (G.shot0_at && now - G.shot0_at < 3000) { fire(EV_AWAY_WOW, now); G.shot0_at = 0; G.afg_check_at = 0; }
        else if (G.period >= 4 && G.hs_before > G.as_before && hs - as <= 8 && G.away_run >= 4) fire(EV_UPSET, now);   /* lead shrinking */
        else if (G.period >= 4 && G.hs_before >= G.as_before && as >= hs) fire(EV_UPSET, now);   /* tie / lead change */
        else fire(EV_AWAY_FG, now);
    }
    if (G.shot0_at && now - G.shot0_at >= 3000) G.shot0_at = 0;
    /* and-one: the clock stops right after the basket (it keeps running after a plain make, except
       in the last two minutes of the 4th / overtime) */
    if (G.fg_check_at && (int)(now - G.fg_check_at) >= 0) {
        int late = G.period >= 4 && G.fg_clock <= 120.0f;
        if (!late && c > 0.5f && c >= G.fg_clock - 0.02f && G.fg_clock > 0.5f) fire(EV_AND1, now);
        G.fg_check_at = 0;
    }
    if (G.afg_check_at && (int)(now - G.afg_check_at) >= 0) {
        int late = G.period >= 4 && G.afg_clock <= 120.0f;
        if (!late && c > 0.5f && c >= G.afg_clock - 0.02f && G.afg_clock > 0.5f) fire(EV_AWAY_BIG, now);
        G.afg_check_at = 0;
    }
    /* 3 s after the period horn: 4th quarter / overtime buzzer beaters and the final result */
    if (G.end_at && now - G.end_at >= 3000) {
        int fin = G.end_period >= 4 && hs != as, beat = G.beater_pending;
        if (G.end_period >= 4) {
            if (beat == 1 && hs > as) fire(EV_BEATER_WIN, now);
            else if (beat == 1 && hs == as) fire(EV_BEATER_TIE, now);
            else if (beat == -1 && hs <= as) fire(EV_STUNNED, now);
            else if (beat == 1) fire(G.beater_pts == 3 ? EV_WOW3 : EV_WOW, now);
            else if (fin) fire(hs > as ? EV_WIN : EV_LOSS, now);
        }
        G.end_at = 0; G.beater_pending = 0;
    }
    /* close finish: last CloseGameSeconds (scaled to the quarter length) of the 4th / overtime */
    {
        float win = (float)C.close_secs * G.qlen / 720.0f; int m = hs - as; if (m < 0) m = -m;
        if (win < 25.0f) win = 25.0f;
        if (G.period >= 4 && c > 0.0f && c <= win && m <= C.close_margin) { G.tense = 1.0f - c / win * 0.5f; G.was_close = 1; }
        else if (G.period >= 4 && c <= 0.0f && m == 0 && G.was_close) G.tense = 1.0f;        /* tied at the horn: overtime */
        else G.tense = 0.0f;
    }
    G.clock = c; G.shot = sc;
}

/* halftime and the 3rd quarter: fans head out at halftime and are still finding their seats when
   play resumes; the arena fills back up over the first ThirdQuarterFullAfter of the quarter */
static float g_ht_fill; static u32 g_ht_last; static int g_ht_init;
static float q3_fill(float game_level, u32 now)
{
    float full = *(const float *)G_FULL, target, dt;
    if (full < 0.3f || full > 1.5f) full = game_level;
    if (G.period == 3 && G.clock > 0.0f) {
        float done = 1.0f - G.clock / G.qlen, k = done / C.q3_filled;
        if (k > 1.0f) k = 1.0f;
        if (k < 0.0f) k = 0.0f;
        target = full * (C.q3_start + (1.0f - C.q3_start) * k);

    } else target = full * C.q3_start;                                                   /* halftime */
    if (!g_ht_init) { g_ht_fill = game_level; g_ht_last = now; g_ht_init = 1; }
    dt = (float)(now - g_ht_last) * 0.001f; g_ht_last = now;
    if (dt > 1.0f) dt = 1.0f;
    if (G.period == 3) g_ht_fill = target;                     /* 3rd quarter: seats fill back up from ThirdQuarterStart */
    else if (g_ht_fill > target) { g_ht_fill -= 0.03f * dt; if (g_ht_fill < target) g_ht_fill = target; }   /* halftime: heading out */
    else g_ht_fill = target;
    return g_ht_fill;
}

/* 4th quarter / overtime attendance: stays full unless the opponent pulls away; fans then trickle out */
static float q4_fill(float game_level, u32 now)
{
    float full = *(const float *)G_FULL, target, lead;
    static u32 last;
    if (full < 0.3f || full > 1.5f) full = 1.0f;
    if (!G.q4_init) { G.q4_fill = full; G.q4_init = 1; last = now; }   /* the 4th starts with everybody back */
    lead = (float)(G.as - G.hs);
    target = full;
    if (lead >= (float)C.leave_lead) {
        float gone = (lead - (float)C.leave_lead + 1.0f) * C.leave_per_point;
        if (gone > C.leave_max) gone = C.leave_max;
        target = full * (1.0f - gone);
    }
    {   /* leave slowly (about 1% of the arena per second), never come back during the 4th */
        float dt = (float)(now - last) * 0.001f; last = now;
        if (dt > 1.0f) dt = 1.0f;
        if (G.q4_fill > target) { G.q4_fill -= 0.01f * dt; if (G.q4_fill < target) G.q4_fill = target; }
    }
    return G.q4_fill;
}

/* visible crowd: the renderer shows a seat when its random byte < fill * 255 (0x9234F0), fill being
 * the game's crowd level (0x925850, 0.75 .. 1.0 from the home team's popularity - 1.0 at Lakers
 * home games). Two call sites read it; both now get it scaled by Attendance. */
float __cdecl ct_fill(void)
{
    float f = *(const float *)G_EXCITE;
    if (C.events && G.key && G.period >= 4 && G.valid) f = q4_fill(f, GetTickCount());
    else {
        G.q4_init = 0;
        if (C.events && G.key && G.valid && (G.period == 3 || (G.period == 2 && G.clock <= 0.0f))) f = q3_fill(f, GetTickCount());
        else g_ht_init = 0;
    }
    return f * (playoffs() ? C.attendance_po : C.attendance);
}
/* replaces "call 0x925850" (result in st(0), touches nothing else): keep every register as it was */
__declspec(naked) void ct_fill_stub(void)
{
    __asm {
        sub esp, 32
        movups [esp], xmm0
        movups [esp+16], xmm1
        pushad
        call ct_fill
        popad
        movups xmm0, [esp]
        movups xmm1, [esp+16]
        add esp, 32
        ret
    }
}

/* ---------------- 'big moments only' applause, applied to the loaded men's graph ----------------
 * Same marks as the 1.1 crowd.iff patch, written into the graph in memory whenever an arena loads,
 * so the stock crowd.iff works (and a patched one is set to the same state). */
#include "graph_gate.inc"
static u32 g_gate_key; static int g_gate_done;
static void apply_gate(void)
{
    u32 p = *(const u32 *)G_SETS, recs, n; int i, ok = 0, bad = 0, cnt = (int)(sizeof k_g2 / sizeof k_g2[0]);
    if (!C.gate_in_memory || !p || p == g_gate_key) return;
    g_gate_key = p;
    if (!readable((void *)p, 8)) return;
    recs = *(const u32 *)p; n = *(const u32 *)(p + 4);
    if (n != 601 || !readable((void *)recs, 601 * 24)) return;
    for (i = 0; i < cnt; i++) {
        u8 *r = (u8 *)(recs + 24u * k_g2[i].i);
        if (*(const u32 *)r != k_g2[i].anim || (*(const u32 *)(r + 0x10) >> 27) != k_g2[i].grp) { bad++; continue; }
    }
    if (bad) { g_gate_done = -bad; return; }                    /* not the stock graph: leave it alone */
    for (i = 0; i < cnt; i++) {
        u32 *y = (u32 *)(recs + 24u * k_g2[i].i + 0x14);
        if (k_g2[i].gated) *y &= ~0x10000000u; else *y |= 0x10000000u;
        ok++;
    }
    g_gate_done = ok;
}

/* ---------------- per-channel memory (calm hold) ---------------- */
static int g_last_out[NCH]; static u32 g_last_change[NCH]; static int g_mem_init;
static u32 g_buzz_count;

/* a reaction mix -> this channel's mood group (r = the channel's place in the arena, 0..1) */
static int mix_group(int set, float r, const MIX *m, float tense, int cls, int g, int po, int *gate_open)
{
    float st = m->stand > tense ? m->stand : tense;
    if (!C.allow_stand) st = 0;
    *gate_open = m->big;
    if (r < st) {
        if (po && m->big && (cls == K_PROP || cls == K_CHANT) && r < C.props) return g;   /* playoffs: props */
        return k_grp[set][m->stand > 0 ? O_CHEER_ST : O_IDLE_ST];
    }
    r -= st;
    if (r < m->head) { *gate_open = 0; return k_grp[set][O_SAD_SD]; }   /* hands on head: head / pain poses only */
    r -= m->head;
    if (r < m->upset) { *gate_open = 1; return k_grp[set][O_SAD_SD]; }  /* upset: every disappointed pose */
    r -= m->upset;
    if (r < m->hup) return k_grp[set][O_CHEER_SD];
    r -= m->hup;
    if (r < m->clap) return k_grp[set][O_CLAP];
    return k_grp[set][O_IDLE_SD];
}

/* 1.3 policy:
 *   scoreboard events (ScoreboardReactions=1): home basket -> calm seated claps; home three -> a bit more;
 *   home big play (the game's own "big play" code right after a home basket, i.e. dunks) -> claps, some
 *   stand; and-one -> disbelief (hands on head) + standing; buzzers -> a reaction; buzzer beaters, wins
 *   and close finishes -> the big ones; the opponent's baskets -> quiet.
 *   no event: everybody seated and quiet (CalmHoldSeconds), standing in a close finish. */
int __cdecl ct_policy(u8 *ch, int g)
{
    int set, cls, lvl, po, open = 0, ev = 0;
    float e, r, tense;
    u32 idx, now;
    g_calls++;
    if (!C.enabled || g < 0 || g > 17) return g;
    set = set_of(ch);
    if (set < 0) return g;
    if (!g_mem_init) { int k; for (k = 0; k < NCH; k++) { g_last_out[k] = -1; g_last_change[k] = 0; } g_mem_init = 1; }
    apply_gate();
    now = GetTickCount();
    game_update(now);
    po = playoffs();
    g_in_hist[set][g]++;
    idx = *(const u32 *)(ch + 8);
    r = idx < NCH ? (float)(((const int *)G_PERM)[idx] % NCH) / (float)NCH : 0.5f;
    e = *(const float *)G_EXCITE;
    if (e < g_exc_min) g_exc_min = e;
    if (e > g_exc_max) g_exc_max = e;
    cls = k_class[set][g];
    if (C.events && G.ev && (int)(G.ev_until - now) > 0) ev = G.ev;
    /* the game's own "big play" code right after a home basket: a dunk / highlight */
    if (C.events && in_list(C.big_sit, g_sit)) {
        if (G.last_home_fg && now - G.last_home_fg < 4000 && ev < EV_HOME_BIG) { fire(EV_HOME_BIG, now); ev = G.ev; }
        else if (G.last_away_fg && now - G.last_away_fg < 4000 && ev < EV_AWAY_BIG) { fire(EV_AWAY_BIG, now); ev = G.ev; }
    }
    tense = C.events ? G.tense * C.close_stand : 0.0f;
    if (ev && cls != K_OTHER) {
        float re = idx < NCH ? (float)((((const int *)G_PERM)[idx] % NCH + (int)G.salt) % NCH) / (float)NCH : r;
        int out = mix_group(set, re, &G.mix, tense, cls, g, po, &open);
        g_level_seen |= G.mix.big ? 4 : 2;
        g = out; lvl = G.mix.big ? 2 : 1;
    } else {
        int scores = C.events && G.scores_ok;
        if (!scores && in_list(C.big_sit, g_sit)) lvl = 2;          /* no scoreboard: the 1.2 rules */
        else if (!scores && (in_list(C.score_sit, g_sit) || cls == K_CHEER || cls == K_PROP)) lvl = 1;
        else lvl = 0;
        if (C.use_exc && lvl < 2 && e >= C.exc_big) lvl = 2;
        if (calm_phase()) lvl = 0;
        g_level_seen |= 1u << lvl;
        open = lvl >= 2;
        switch (cls) {
        case K_IDLE:
            g = k_grp[set][(r < C.stand_idle || r < tense) ? O_IDLE_ST : O_IDLE_SD];
            break;
        case K_SAD:
            g = k_grp[set][r < tense ? O_IDLE_ST : r < tense + C.sad ? O_SAD_SD : O_IDLE_SD];
            break;
        case K_CLAP: case K_CHEER: case K_PROP: case K_CHANT:
            if (lvl == 0) {
                g = k_grp[set][r < tense ? O_IDLE_ST : r < tense + C.clap_calm ? O_CLAP : O_IDLE_SD];
            } else if (lvl == 1) {
                float a = C.cheer_good, b = a + C.clap_good;
                g = k_grp[set][r < a ? O_CHEER_SD : r < b ? O_CLAP : O_IDLE_SD];
            } else {
                float st = C.allow_stand ? C.stand_big_max : 0.0f;
                if (r < st) {
                    if (po && (cls == K_PROP || cls == K_CHANT) && r < C.props) { }
                    else g = k_grp[set][O_CHEER_ST];
                } else if (r < st + C.seat_cheer_big) g = k_grp[set][O_CHEER_SD];
                else g = k_grp[set][O_CLAP];
            }
            break;
        default: break;
        }
    }
    if (po) g_level_seen |= 16;
    g_gate = open ? 0.0f : 9.0f;              /* energetic applause / other "sad" poses: big moments only */
    /* calm hold: a seated-idle fan stays seated-idle for CalmHoldSeconds when nothing happens */
    if (idx < NCH) {
        int last = g_last_out[idx];
        if (!ev && lvl == 0 && last == k_grp[set][O_IDLE_SD] && g != last && C.calm_hold > 0 &&
            now - g_last_change[idx] < (u32)C.calm_hold * 1000u && !(tense > 0 && g == k_grp[set][O_IDLE_ST]))
            g = last;
        if (g != last) { g_last_out[idx] = g; g_last_change[idx] = now; }
    }
    g_out_hist[set][g]++;
    return g;
}

/* ---------------- call stubs ---------------- */
static u32 g_f_mood = F_MOOD, g_f_sit = F_SIT;

/* replaces "call 0x9251B0" (eax = arg, stack: channel, arg2; callee pops 8) */
__declspec(naked) void ct_mood_stub(void)
{
    __asm {
        push dword ptr [esp+8]
        push dword ptr [esp+8]
        call dword ptr [g_f_mood]
        pushfd
        pushad
        push eax
        push dword ptr [esp+44]
        call ct_policy
        add esp, 8
        mov [esp+28], eax
        popad
        popfd
        ret 8
    }
}

/* replaces "call 0x924BE0" (3 stack args, callee pops 12): remember the situation code */
__declspec(naked) void ct_sit_stub(void)
{
    __asm {
        push dword ptr [esp+12]
        push dword ptr [esp+12]
        push dword ptr [esp+12]
        call dword ptr [g_f_sit]
        mov dword ptr [g_sit], eax
        ret 12
    }
}

/* "call <game function>" -> "call <our stub>", through SLAM (registered, conflict-checked) */
static int patch_call(u32 at, const u8 *orig, void *to)
{
    u8 b[5]; u8 *p = (u8 *)at; int r;
    if (!readable(p, 5) || !same(p, orig, 5)) return 0;
    b[0] = 0xE8; *(u32 *)(b + 1) = (u32)to - (at + 5);
    r = g_slam->patch(g_slamMe, p, b, 5);
    if (r == SLAM_ERR_CONFLICT) logd("WARNING another mod already changed the game code at 0x", at, " (see SLAM\\SLAM.log) - left alone");
    return r == SLAM_OK;
}

static void log_stats(void)
{
    char line[512], t[16]; int s, g, i;
    float e = *(const float *)G_EXCITE;
    line[0] = 0;
    s_cat(line, "excitement ", 512); f2s(e, t); s_cat(line, t, 512);
    s_cat(line, " (", 512); f2s(g_exc_min < 9 ? g_exc_min : e, t); s_cat(line, t, 512); s_cat(line, "..", 512);
    f2s(g_exc_max > -9 ? g_exc_max : e, t); s_cat(line, t, 512); s_cat(line, ")  situation ", 512);
    u2d(g_sit, t); s_cat(line, t, 512);
    s_cat(line, "  level", 512); if (g_level_seen & 1) s_cat(line, " calm", 512); if (g_level_seen & 2) s_cat(line, " score", 512); if (g_level_seen & 4) s_cat(line, " BIG", 512);
    if (g_level_seen & 8) s_cat(line, " (BUZZER)", 512);
    s_cat(line, (g_level_seen & 16) ? "  PLAYOFFS" : "", 512); g_level_seen = 0;
    s_cat(line, "  clock ", 512); f2s(*(const float *)G_CLOCK, t); s_cat(line, t, 512);
    s_cat(line, " shot ", 512); f2s(*(const float *)G_SHOT, t); s_cat(line, t, 512);
    s_cat(line, "  Q", 512); u2d((u32)G.period, t); s_cat(line, t, 512);
    s_cat(line, " home ", 512); u2d((u32)G.hs, t); s_cat(line, t, 512); s_cat(line, "-", 512); u2d((u32)G.as, t); s_cat(line, t, 512);
    s_cat(line, G.home_ok == 1 ? (G.home ? " (home=B)" : " (home=A)") : G.home_ok == 2 ? " (home=A?)" : "", 512);
    if (G.ev && (int)(G.ev_until - GetTickCount()) > 0) { s_cat(line, "  EVENT ", 512); s_cat(line, k_ev_name[G.ev], 512); }
    if (G.tense > 0) { s_cat(line, "  close finish ", 512); f2s(G.tense, t); s_cat(line, t, 512); }
    s_cat(line, "  shown ", 512); f2s(*(const float *)0x1E20320u, t); s_cat(line, t, 512);
    s_cat(line, "  gate ", 512); if (g_gate_done < 0) { s_cat(line, "not-stock", 512); } else { u2d((u32)g_gate_done, t); s_cat(line, t, 512); }
    s_cat(line, "  phase ", 512); u2d(*(const u32 *)G_MODE, t); s_cat(line, t, 512);
    s_cat(line, g_calm_reason == 1 ? " (not live: calm)" : g_calm_reason == 2 ? " (tip-off: calm)" : g_calm_reason == 3 ? " (resume: calm)" : " (live)", 512);
    s_cat(line, "  asked/given per group (men|women):", 512);
    for (s = 0; s < 2; s++) {
        s_cat(line, s ? " |" : "", 512);
        for (g = 0; g < 18; g++) if (g_in_hist[s][g] || g_out_hist[s][g]) {
            s_cat(line, " ", 512); u2d((u32)g, t); s_cat(line, t, 512); s_cat(line, ":", 512);
            u2d(g_in_hist[s][g], t); s_cat(line, t, 512); s_cat(line, "/", 512); u2d(g_out_hist[s][g], t); s_cat(line, t, 512);
        }
    }
    s_cat(line, "  now:", 512);
    {   /* what the 60 channels are doing right now (group of the current clip) */
        u32 cnt[2][18]; int k;
        for (s = 0; s < 2; s++) for (g = 0; g < 18; g++) cnt[s][g] = 0;
        for (k = 0; k < NCH; k++) {
            const u8 *ch = (const u8 *)(0x12FCB28u + 0x34u * (u32)k);
            int st = (int)*(const u32 *)(ch + 0x10), gg = (int)*(const u32 *)(ch + 0x0C);
            if (st >= 0 && st < 2 && gg >= 0 && gg < 18) cnt[st][gg]++;
        }
        for (s = 0; s < 2; s++) {
            s_cat(line, s ? " |" : "", 512);
            for (g = 0; g < 18; g++) if (cnt[s][g]) { s_cat(line, " ", 512); u2d((u32)g, t); s_cat(line, t, 512); s_cat(line, "x", 512); u2d(cnt[s][g], t); s_cat(line, t, 512); }
        }
    }
    logs(line, 0, 0);
    for (s = 0; s < 2; s++) for (g = 0; g < 18; g++) g_in_hist[s][g] = g_out_hist[s][g] = 0;
    for (i = 0; i < 32; i++) g_sit_hist[i] = 0;
    g_exc_min = 9; g_exc_max = -9;
}

static DWORD WINAPI init_thread(LPVOID unused)
{
    DWORD t0 = GetTickCount(); int a = 0, b = 0;
    (void)unused;
    for (;;) {
        if (readable((void *)A_MOOD_CALL, 5) && same((const u8 *)A_MOOD_CALL, k_mood_call, 5) &&
            readable((void *)A_SIT_CALL, 5) && same((const u8 *)A_SIT_CALL, k_sit_call, 5) &&
            readable((void *)A_GATE, 8) && same((const u8 *)A_GATE, k_gate, 8) &&
            readable((void *)A_FILL_CALL1, 5) && same((const u8 *)A_FILL_CALL1, k_fill1, 5) &&
            readable((void *)A_FILL_CALL2, 5) && same((const u8 *)A_FILL_CALL2, k_fill2, 5)) break;
        if (GetTickCount() - t0 > 90000) {
            logs("game code not recognised after 90 s - not patching. CrowdTune needs the RLD nba2k14.exe (TimeDateStamp 52202C7F).", 0, 0);
            return 0;
        }
        Sleep(20);
    }
    Sleep(300);
    a = patch_call(A_SIT_CALL, k_sit_call, (void *)ct_sit_stub);
    b = patch_call(A_MOOD_CALL, k_mood_call, (void *)ct_mood_stub);
    {   /* point the 'excited only' threshold at our own value */
        u8 *p = (u8 *)A_GATE, b[8]; int k;
        if (readable(p, 8) && same(p, k_gate, 8)) {
            for (k = 0; k < 4; k++) b[k] = k_gate[k];
            *(u32 *)(b + 4) = (u32)&g_gate;
            if (g_slam->patch(g_slamMe, p, b, 8) == SLAM_OK) a++;
        }
    }
    a += patch_call(A_FILL_CALL1, k_fill1, (void *)ct_fill_stub);
    a += patch_call(A_FILL_CALL2, k_fill2, (void *)ct_fill_stub);
    if (C.start_seated) {   /* the mood group every channel starts in when an arena loads: seated idle */
        u32 *sg = (u32 *)G_START_GRP; static const u32 seated[2] = { 1, 1 };
        if (readable(sg, 8) && sg[0] == 7 && sg[1] == 6 && g_slam->patch(g_slamMe, sg, seated, 8) == SLAM_OK) {
            logs("arena loads with everybody seated (start group 7/6 -> 1)", 0, 0);
        } else logs("WARNING start group not recognised - fans may stand up when the arena loads", 0, 0);
    }
    if (a + b == 5) logs("patched 5 of 5 spots - crowd moods re-mapped, attendance scaled (settings: CrowdTune.ini)", 0, 0);
    else { logd("WARNING patched only ", (u32)(a + b), " of 5 spots"); }
    for (;;) {
        u32 n = g_calls;
        Sleep(C.log_every > 0 ? (DWORD)C.log_every * 1000 : 60000);
        if (C.log_every > 0 && g_calls != n) log_stats();
    }
}

/* SLAM calls this once all mods are loaded (slam_glue.c -> SlamModStart) */
int mod_start(void)
{
    s_cpy(g_ini, g_slam->mod_ini(g_slamMe), MAX_PATH);
    load_ini();
    if (!C.enabled) { cfg_log = 1; logs("CrowdTune " VERSION " - Enabled=0 in CrowdTune.ini: off", 0, 0); return 0; }
    logs("CrowdTune " VERSION " loaded (SLAM ", g_slam->slamVersion, ") - calmer regular-season crowd");
    CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
    return 0;
}
