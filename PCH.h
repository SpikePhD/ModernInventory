#pragma once

// Ensure STL/CRT settings match CommonLibSSE (avoid Debug iterator/CRT mismatches)
#if defined(_MSC_VER)
#  if !defined(_ITERATOR_DEBUG_LEVEL)
#    define _ITERATOR_DEBUG_LEVEL 0
#  endif
#  if !defined(_SECURE_SCL)
#    define _SECURE_SCL 0
#  endif
#endif

// CommonLibSSE/NG main headers
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Convenience
using namespace std::literals;
