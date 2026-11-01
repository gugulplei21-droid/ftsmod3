// libftsmod.so — hooks sobre libFTS15.so (FTS15 v2.09, 32 bits). ESQUELETO SIN PROBAR.
#include <jni.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <android/log.h>
#include <atomic>
#include <cstdio>
#include "game.h"
#include "dobby.h"

#define TAG "ftsmod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// ---------- Estado compartido con la UI (el orden debe coincidir con ModMenu.java) ----------
enum Feature { F_SPEED, F_POWER, F_AIM, F_KEEPER, F_JOY, F_DEBUG, F_COUNT };
enum Value   { V_SPEED, V_POWER, V_COUNT };

static std::atomic<bool>  g_on[F_COUNT];
static std::atomic<float> g_val[V_COUNT];
static std::atomic<float> g_joyX, g_joyY;
static std::atomic<int>   g_hooksOk, g_hooksTotal;

static CBall* volatile g_ball = nullptr;       // capturado en CBall::UpdateFrame
static int  g_userTeam = 0;                     // TODO(RE): equipo del usuario
static thread_local bool t_inFinish = false;    // true mientras corre CPlayer::ControlFinish
static void (*p_calcSpeed)(CBall*) = nullptr;   // CBall::CalcSpeed() (solo se llama, no se hookea)

static const float JOY_SPEED = 600.f;           // TODO(RE): calibrar con el log de depuración

static inline bool  on(Feature f) { return g_on[f].load(); }
static inline float val(Value v)  { return g_val[v].load(); }

// TODO(RE): decidir si 'p' es el jugador del usuario. Pista: GC_PlayerIsControlled(int,int)
// (_Z21GC_PlayerIsControlledii) y GL_SetTouch(int,int,...) reciben equipo/índice de jugador.
static bool isUserPlayer(CPlayer*) { return true; }

// ---------- 1) Velocidad (tipo de retorno int confirmado en GetSprintSpeed) ----------
#define SPEED_HOOK(NAME)                                                        \
  static int (*o_##NAME)(CPlayer*);                                             \
  static int h_##NAME(CPlayer* p) {                                             \
    int r = o_##NAME(p);                                                        \
    return (on(F_SPEED) && isUserPlayer(p)) ? (int)(r * val(V_SPEED)) : r;      \
  }
SPEED_HOOK(sprint)   // CPlayer::GetSprintSpeed()
SPEED_HOOK(curRun)   // CPlayer::GetCurrentRunSpeed()
SPEED_HOOK(run)      // CPlayer::GetRunSpeed()

// ---------- 2) Disparo: ControlFinish (rutina de tiro) + escalado de velocidad del balón ----------
// Se declara int de retorno: si la función es void, r0 sobrante se ignora; si es int, se conserva.
static int (*o_finish)(CPlayer*, int, int, int);
static int h_finish(CPlayer* p, int a, int b, int c) {
  bool mine = (on(F_POWER) || on(F_AIM)) && isUserPlayer(p);
  if (mine) t_inFinish = true;
  int r = o_finish(p, a, b, c);
  t_inFinish = false;
  CBall* ball = g_ball;
  if (mine && on(F_POWER) && ball) {
    float m = val(V_POWER);
    ballVX(ball) = (int)(ballVX(ball) * m);
    ballVY(ball) = (int)(ballVY(ball) * m);
    ballVZ(ball) = (int)(ballVZ(ball) * m);
    if (p_calcSpeed) p_calcSpeed(ball);
    // TODO(RE): ControlFinish llama a SetBallMoveToPos(Low); si el balón sigue una
    // trayectoria proyectada y no solo la velocidad, hay que escalar ahí en su lugar.
  }
  return r;
}

// ---------- 3) Precisión: SetBallMoveToPos(TPoint3D*, TPoint3D, TPoint3D, int, int) ----------
// ControlFinish la invoca al tirar (verificado). Semántica de cada argumento: pendiente.
static int (*o_moveToPos)(CBall*, TPoint3D*, TPoint3D, TPoint3D, int, int);
static int h_moveToPos(CBall* b, TPoint3D* p, TPoint3D a, TPoint3D c, int i, int j) {
  if (t_inFinish && (on(F_AIM) || on(F_DEBUG))) {
    LOGI("moveToPos(tiro) p=%p a=(%d,%d,%d) c=(%d,%d,%d) i=%d j=%d",
         (void*)p, a.x, a.y, a.z, c.x, c.y, c.z, i, j);
    // TODO(RE): con los logs, identificar cuál TPoint3D es el destino y sustituirlo
    // por el centro de la portería rival (esquina alta) cuando on(F_AIM).
  }
  return o_moveToPos(b, p, a, c, i, j);
}

// ---------- 4) Balón controlable con joystick: se aplica cada frame ----------
static int (*o_frame)(CBall*);
static int h_frame(CBall* b) {
  g_ball = b;
  static int n = 0;
  if (on(F_DEBUG) && (++n % 60) == 0)
    LOGI("balon v=(%d,%d,%d) speed=%d", ballVX(b), ballVY(b), ballVZ(b), ballSpeed(b));
  float jx = g_joyX.load(), jy = g_joyY.load();
  if (on(F_JOY) && (jx * jx + jy * jy) > 0.01f) {
    // x,y = plano horizontal, z = vertical (no se toca). TODO(RE): mapear ejes a la cámara.
    ballVX(b) = (int)(jx * JOY_SPEED);
    ballVY(b) = (int)(jy * JOY_SPEED);
    if (p_calcSpeed) p_calcSpeed(b);
  }
  return o_frame(b);
}

// ---------- 5) Portero: anular reacciones del portero rival ----------
// TODO(RE): confirmar que el int es el equipo y el tipo de retorno de cada función.
#define GK_HOOK(NAME)                                                           \
  static int (*o_##NAME)(int);                                                  \
  static int h_##NAME(int team) {                                               \
    if (on(F_KEEPER) && team != g_userTeam) return 0;                           \
    return o_##NAME(team);                                                      \
  }
GK_HOOK(gkSave)        // GAI_GKProcessSave(int)
GK_HOOK(gkCatch)       // GAI_GKCatchBall(int)
GK_HOOK(gkParry)       // GAI_GKParryBall(int)
GK_HOOK(gkAnticipate)  // GAI_GKCheckAnticipate(int)

// ---------- Instalación ----------
static void install(void* lib, const char* sym, void* repl, void** orig) {
  g_hooksTotal++;
  void* addr = dlsym(lib, sym);
  if (!addr) { LOGE("simbolo no encontrado: %s", sym); return; }
  
  // Usamos cast genérico a (void*) compatible con Dobby actualizado
  if (DobbyHook(addr, (void*)repl, (void**)orig) == 0) {
    g_hooksOk++; LOGI("hook OK %s @%p", sym, addr);
  } else {
    LOGE("DobbyHook fallo: %s", sym);
  }
}
#define HOOK(SYM, NAME) install(lib, SYM, (void*)h_##NAME, (void**)&o_##NAME)

static void* installThread(void*) {
  void* lib = nullptr;
  for (int i = 0; i < 600 && !lib; i++) {            // espera hasta 60 s a que cargue el juego
    lib = dlopen("libFTS15.so", RTLD_NOW | RTLD_NOLOAD);
    if (!lib) usleep(100000);
  }
  if (!lib) { LOGE("libFTS15.so no esta cargada"); return nullptr; }

  p_calcSpeed = (void (*)(CBall*))dlsym(lib, "_ZN5CBall9CalcSpeedEv");

  HOOK("_ZN7CPlayer14GetSprintSpeedEv",     sprint);
  HOOK("_ZN7CPlayer18GetCurrentRunSpeedEv", curRun);
  HOOK("_ZN7CPlayer11GetRunSpeedEv",        run);
  HOOK("_ZN7CPlayer13ControlFinishEiii",    finish);
  HOOK("_ZN5CBall16SetBallMoveToPosEP8TPoint3DS0_S0_ii", moveToPos);
  HOOK("_ZN5CBall11UpdateFrameEv",          frame);
  HOOK("_Z17GAI_GKProcessSavei",            gkSave);
  HOOK("_Z15GAI_GKCatchBalli",              gkCatch);
  HOOK("_Z15GAI_GKParryBalli",              gkParry);
  HOOK("_Z21GAI_GKCheckAnticipatei",        gkAnticipate);
  LOGI("hooks instalados: %d/%d", g_hooksOk.load(), g_hooksTotal.load());
  return nullptr;
}

// ---------- JNI ----------
extern "C" {
JNIEXPORT void JNICALL Java_com_ftsmod_ModNative_setFeature(JNIEnv*, jclass, jint id, jboolean v) {
  if (id >= 0 && id < F_COUNT) g_on[id].store(v != 0);
}
JNIEXPORT void JNICALL Java_com_ftsmod_ModNative_setValue(JNIEnv*, jclass, jint id, jfloat v) {
  if (id >= 0 && id < V_COUNT) g_val[id].store(v);
}
JNIEXPORT void JNICALL Java_com_ftsmod_ModNative_setJoystick(JNIEnv*, jclass, jfloat x, jfloat y) {
  g_joyX.store(x); g_joyY.store(y);
}
JNIEXPORT jstring JNICALL Java_com_ftsmod_ModNative_getStatus(JNIEnv* env, jclass) {
  char buf[64];
  snprintf(buf, sizeof buf, "Hooks: %d/%d%s", g_hooksOk.load(), g_hooksTotal.load(),
           g_ball ? " - balon OK" : "");
  return env->NewStringUTF(buf);
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
  for (int i = 0; i < F_COUNT; i++) g_on[i].store(false);
  g_val[V_SPEED].store(1.5f);
  g_val[V_POWER].store(2.0f);
  g_joyX.store(0); g_joyY.store(0);
  pthread_t t; pthread_create(&t, nullptr, installThread, nullptr); pthread_detach(t);
  return JNI_VERSION_1_6;
}
}
