// Tipos y offsets de FTS15 v2.09 (libFTS15.so).
// Origen: desensamblado de la versión x86. VERIFICAR en la versión ARM (Ghidra).
#pragma once

// 3 enteros (12 bytes), POD: se pasa por valor igual que lo hace el juego.
struct TPoint3D { int x, y, z; };

struct CBall;     // opacos: solo se acceden por offset
struct CPlayer;

namespace off {
  // CBall::SetVel escribe la velocidad aquí y llama a CalcSpeed (verificado en x86).
  // CBall::ClampSpeed normaliza solo x,y (XMATH_Normalize2D): z es la vertical.
  constexpr int BALL_VX = 0x0c, BALL_VY = 0x10, BALL_VZ = 0x14;
  constexpr int BALL_SPEED = 0x24;   // lo calcula CalcSpeed (ClampSpeed lo lee)
}

template <int O> static inline int& field(void* base) {
  return *reinterpret_cast<int*>(reinterpret_cast<char*>(base) + O);
}
static inline int& ballVX(CBall* b)    { return field<off::BALL_VX>(b); }
static inline int& ballVY(CBall* b)    { return field<off::BALL_VY>(b); }
static inline int& ballVZ(CBall* b)    { return field<off::BALL_VZ>(b); }
static inline int& ballSpeed(CBall* b) { return field<off::BALL_SPEED>(b); }
