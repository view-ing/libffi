#ifdef __arm64__

#if __has_include(<bdffctarget_arm64.h>)
#include <bdffctarget_arm64.h>
#elif __has_include("bdffctarget_arm64.h")
#include "bdffctarget_arm64.h"
#endif

#endif
#ifdef __i386__

#if __has_include(<bdffctarget_i386.h>)
#include <bdffctarget_i386.h>
#elif __has_include("bdffctarget_i386.h")
#include "bdffctarget_i386.h"
#endif

#endif
#ifdef __arm__

#if __has_include(<bdffctarget_armv7.h>)
#include <bdffctarget_armv7.h>
#elif __has_include("bdffctarget_armv7.h")
#include "bdffctarget_armv7.h"
#endif

#endif
#ifdef __x86_64__

#if __has_include(<bdffctarget_x86_64.h>)
#include <bdffctarget_x86_64.h>
#elif __has_include("bdffctarget_x86_64.h")
#include "bdffctarget_x86_64.h"
#endif

#endif
