#ifndef GUARD_AMATEURLY_TSARIST_DEEKSHA_SUBCULTIVATES_2234
#define GUARD_AMATEURLY_TSARIST_DEEKSHA_SUBCULTIVATES_2234
#pragma once

// frames
#define FrameMark (void) 0
#define FrameMarkNamed(name) (void) 0
#define FrameMarkStart(name) (void) 0
#define FrameMarkEnd(name) (void) 0
#define FrameImage(image, width, height, offset, flip) (void) 0

// zones
#define ZoneScoped (void) 0
#define ZoneScopedC(color) (void) 0
#define ZoneScopedN(name) (void) 0
#define ZoneScopedNC(name, color) (void) 0
#define ZoneScopedS(depth) (void) 0
#define ZoneScopedCS(color, depth) (void) 0
#define ZoneScopedNS(name, depth) (void) 0
#define ZoneScopedNCS(name, color, depth) (void) 0
#define ZoneText(text, size) (void) 0
#define ZoneValue(val) (void) 0
#define ZoneName(text, size) (void) 0

#define ZoneNamed(var, active) (void) 0
#define ZoneNamedC(var, color, active) (void) 0
#define ZoneNamedN(var, name, active) (void) 0
#define ZoneNamedNC(var, name, color, active) (void) 0
#define ZoneTextV(var, text, size) (void) 0
#define ZoneValueV(var, val) (void) 0
#define ZoneNameV(var, text, size) (void) 0

// locks
#define TracyLockable(type, var) type var
#define TracyLockableN(type, var, desc) type var
#define LockableName(var, name, size) (void) 0
#define LockableBase(type) type
#define LockMark(var) (void) 0

#define TracySharedLockable(type, var) type var
#define TracySharedLockableN(type, var, desc) type var
#define SharedLockableBase(type) type

// plot
#define TracyPlot(name, value) (void) 0
#define TracyPlotConfig(name, format) (void) 0

// msg log
#define TracyMessage(text, size) (void) 0
#define TracyMessageL(text) (void) 0
#define TracyMessageC(text, size, color) (void) 0
#define TracyMessageLC(text, color) (void) 0
#define TracyMessageS(text, size, depth) (void) 0
#define TracyMessageLS(text, depth) (void) 0
#define TracyMessageCS(text, size, color, depth) (void) 0
#define TracyMessageLCS(text, color, depth) (void) 0

// app info
#define TracyAppInfo(text, size) (void) 0

// memory
#define TracyAlloc(ptr, size) (void) 0
#define TracyAllocN(ptr, size, name) (void) 0
#define TracyAllocS(ptr, size, depth) (void) 0
#define TracyAllocNS(ptr, size, depth, name) (void) 0
#define TracyFree(ptr) (void) 0
#define TracyFreeN(ptr, name) (void) 0
#define TracyFreeS(ptr, depth) (void) 0
#define TracyFreeNS(ptr, depth, name) (void) 0

// thread
namespace tracy
{
  inline void SetThreadName(const char* name) noexcept {}
}

#endif
