#ifndef GUARD_AMATEURLY_TSARIST_DEEKSHA_SUBCULTIVATES_2234
#define GUARD_AMATEURLY_TSARIST_DEEKSHA_SUBCULTIVATES_2234
#pragma once

// frames
#define FrameMark
#define FrameMarkNamed(name)
#define FrameMarkStart(name)
#define FrameMarkEnd(name)
#define FrameImage(image, width, height, offset, flip)

// zones
#define ZoneScoped
#define ZoneScopedC(color)
#define ZoneScopedN(name)
#define ZoneScopedNC(name, color)
#define ZoneScopedS(depth)
#define ZoneScopedCS(color, depth)
#define ZoneScopedNS(name, depth)
#define ZoneScopedNCS(name, color, depth)
#define ZoneText(text, size)
#define ZoneValue(val)
#define ZoneName(text, size)

#define ZoneNamed(var, active)
#define ZoneNamedC(var, color, active)
#define ZoneNamedN(var, name, active)
#define ZoneNamedNC(var, name, color, active)
#define ZoneTextV(var, text, size)
#define ZoneValueV(var, val)
#define ZoneNameV(var, text, size)

// locks
#define TracyLockable(type, var) type var
#define TracyLockableN(type, var, desc) type var
#define LockableName(var, name, size)
#define LockableBase(type) type
#define LockMark(var)

#define TracySharedLockable(type, var) type var
#define TracySharedLockableN(type, var, desc) type var
#define SharedLockableBase(type) type

// plot
#define TracyPlot(name, value)
#define TracyPlotConfig(name, format)

// msg log
#define TracyMessage(text, size)
#define TracyMessageL(text)
#define TracyMessageC(text, size, color)
#define TracyMessageLC(text, color)
#define TracyMessageS(text, size, depth)
#define TracyMessageLS(text, depth)
#define TracyMessageCS(text, size, color, depth)
#define TracyMessageLCS(text, color, depth)

// app info
#define TracyAppInfo(text, size)

// memory
#define TracyAlloc(ptr, size)
#define TracyFree(ptr)
#define TracyAllocS(ptr, size, depth)
#define TracyFreeS(ptr, depth)

// thread
namespace tracy
{
  inline void SetThreadName(const char* name) noexcept {}
}

#endif
