#pragma once

#include "PIDefines.h"
#include "PIFormat.h"
#include "PIUtilities.h"
#include "PIUI.h"
#include "FileUtilities.h"
#include "PaaFormatTerminology.h"

#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <time.h>

#if __PIMac__
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <stdio.h>
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include "..\win\paaformat-sym.h"
#endif

extern SPPluginRef gPluginRef;
extern FormatRecordPtr gFormatRecord;
extern int16* gResult;

// -----------------------------------------------------------------------
// PAA Type support
// Matches grad_aff TypeClass enum
// -----------------------------------------------------------------------
enum class PaaTypeClass : uint8_t {
    Unknown  = 0x00,
    DXT1     = 0x01,  // RGB, no alpha — common _co textures
    DXT3     = 0x02,  // RGBA, 4-bit alpha
    DXT5     = 0x03,  // RGBA, smooth alpha — _co with transparency, _smdi
    ARGB4444 = 0x04,  // Legacy
    ARGB1555 = 0x05,  // Legacy
    ARGB8888 = 0x08,  // Uncompressed RGBA
    AI88     = 0x09,  // Grayscale + alpha — _nohq detail channels
    Unk0C    = 0x0C,  // Unknown
};

// -----------------------------------------------------------------------
// Filter
// -----------------------------------------------------------------------
static void DoFilterFile();

// -----------------------------------------------------------------------
// File I/O helpers
// -----------------------------------------------------------------------
static void Read(int64_t count, void* buffer);
static void Write(int64_t count, const void* buffer);
static void FromStart();

// -----------------------------------------------------------------------
// Read selectors
// -----------------------------------------------------------------------
static void DoReadPrepare();
static void DoReadStart();
static void DoReadContinue();
static void DoReadFinish();

// -----------------------------------------------------------------------
// Write selectors
// -----------------------------------------------------------------------
static void DoWritePrepare();
static void DoWriteStart();
static void DoWriteContinue();
static void DoWriteFinish();

// -----------------------------------------------------------------------
// Options / Estimate selectors
// -----------------------------------------------------------------------
static void DoOptionsPrepare();
static void DoOptionsStart();
static void DoEstimatePrepare();
static void DoEstimateStart();
static void DoEstimateContinue();
static void DoEstimateFinish();

// -----------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------
[[nodiscard]] static bool      isPowerOfTwo(uint32_t x);
[[nodiscard]] static int64_t   GetFileSizeFFR();
[[nodiscard]] static std::string GetWin32ErrorString(DWORD code);

// -----------------------------------------------------------------------
// UI
// -----------------------------------------------------------------------
bool DoAboutUI(FormatRecordPtr formatRecord);
void DoMessageUI(const std::string& title, const std::string& message);
