#include "PaaFormat.h"

#define GRAD_AFF_STATIC_DEFINE
#include <grad_aff/paa/paa.h>

DLLExport MACPASCAL void PluginMain(const int16 selector, FormatRecordPtr formatParamBlock, intptr_t* data, int16* result);

// -----------------------------------------------------------------------
// Globals (must be extern "C" to match Photoshop SDK declarations)
// -----------------------------------------------------------------------
extern "C" {
SPBasicSuite*    sSPBasic      = nullptr;
SPPluginRef      gPluginRef    = nullptr;
FormatRecordPtr  gFormatRecord = nullptr;
int16*           gResult       = nullptr;
}

// -----------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------

static bool isPowerOfTwo(uint32_t x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

static std::string GetWin32ErrorString(DWORD code) {
#ifdef _WIN32
    char buf[512] = {};
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, static_cast<DWORD>(sizeof(buf)), nullptr);
    // trim trailing newlines
    std::string s(buf);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    if (s.empty()) s = "(error code " + std::to_string(code) + ")";
    return s;
#else
    return "(error code " + std::to_string(code) + ")";
#endif
}

// -----------------------------------------------------------------------
// Determine output PAA TypeClass from image properties.
// Called at write time to pick the right DXT encoding.
// -----------------------------------------------------------------------
//
// Logic:
//   Grayscale (1 plane)            -> AI88  (greyscale + alpha pad)
//   RGB  (3 planes, no alpha)      -> DXT1  (no alpha, best compression)
//   RGBA (4 planes, with alpha)    -> DXT5  (smooth alpha: skins, decals)
//
// Normal maps (_nohq) in Arma use DXT5nm convention (R=X, G=Y channels).
// grad_aff writes standard DXT5; the channels are already correct from PS.
//
static grad_aff::TypeOfPaX PickPaaTypeClass(bool hasAlpha, bool isGrayscale) {
    if (isGrayscale)  return grad_aff::TypeOfPaX::GRAYwAlpha;
    if (hasAlpha)     return grad_aff::TypeOfPaX::DXT5;
    return grad_aff::TypeOfPaX::DXT1;
}

// -----------------------------------------------------------------------
// File I/O helpers
// -----------------------------------------------------------------------

static int64_t GetFileSizeFFR() {
    int64_t filesize = 0;

#ifdef _WIN32
    LARGE_INTEGER fs;
    if (!GetFileSizeEx(reinterpret_cast<HANDLE>(gFormatRecord->dataFork), &fs)) {
        DWORD err = GetLastError();
        DoMessageUI("PAA read error!", "Could not determine file size: " + GetWin32ErrorString(err));
        *gResult = readErr;
        return 0;
    }
    filesize = fs.QuadPart;
#endif

#if __PIMac__
    struct stat st;
    if (fstat(gFormatRecord->posixFileDescriptor, &st) != 0) {
        DoMessageUI("PAA read error!", "Could not determine file size (fstat failed).");
        *gResult = readErr;
        return 0;
    }
    filesize = st.st_size;
#endif

    return filesize;
}

static void FromStart() {
    auto result = PSSDKSetFPos(
        gFormatRecord->dataFork,
        gFormatRecord->posixFileDescriptor,
        gFormatRecord->pluginUsingPOSIXIO,
        fsFromStart, 0);
    *gResult = result;
#ifdef _WIN32
    if (result != noErr) {
        DoMessageUI("PAA Error!", "Seek-to-start failed: " + GetWin32ErrorString(GetLastError()));
    }
#endif
}

static void Read(int64_t count, void* buffer) {
    // PSSDKRead takes int32_t counts — chunk if > 2 GB (rare for PAA but correct)
    int64_t remaining = count;
    uint8_t* dst = reinterpret_cast<uint8_t*>(buffer);

    while (remaining > 0) {
        int32_t chunk = static_cast<int32_t>(std::min(remaining, static_cast<int64_t>(0x7FFF'0000)));
        int32_t readCount = chunk;
        auto result = PSSDKRead(
            gFormatRecord->dataFork,
            gFormatRecord->posixFileDescriptor,
            gFormatRecord->pluginUsingPOSIXIO,
            &readCount, dst);
        *gResult = result;
        if (result != noErr) {
#ifdef _WIN32
            DoMessageUI("PAA read error!", "Read failed: " + GetWin32ErrorString(GetLastError()));
#endif
            return;
        }
        dst       += readCount;
        remaining -= readCount;
    }
}

static void Write(int64_t count, const void* buffer) {
    int64_t       remaining = count;
    const uint8_t* src      = reinterpret_cast<const uint8_t*>(buffer);

    while (remaining > 0) {
        int32_t chunk      = static_cast<int32_t>(std::min(remaining, static_cast<int64_t>(0x7FFF'0000)));
        int32_t writeCount = chunk;
        auto result = PSSDKWrite(
            gFormatRecord->dataFork,
            gFormatRecord->posixFileDescriptor,
            gFormatRecord->pluginUsingPOSIXIO,
            &writeCount, const_cast<void*>(reinterpret_cast<const void*>(src)));
        *gResult = result;
        if (result == noErr && writeCount != chunk) {
            DoMessageUI("PAA save error!", "Disk is full!");
            *gResult = eofErr;
            return;
        }
        if (result != noErr) {
#ifdef _WIN32
            DoMessageUI("PAA write error!", "Write failed: " + GetWin32ErrorString(GetLastError()));
#endif
            return;
        }
        src       += writeCount;
        remaining -= writeCount;
    }
}

// -----------------------------------------------------------------------
// PluginMain entry point
// -----------------------------------------------------------------------

DLLExport MACPASCAL void PluginMain(
    const int16      selector,
    FormatRecordPtr  formatParamBlock,
    intptr_t*        data,
    int16*           result)
{
    try {
        gFormatRecord = formatParamBlock;
        gPluginRef    = reinterpret_cast<SPPluginRef>(gFormatRecord->plugInRef);
        gResult       = result;

        if (selector == formatSelectorAbout) {
            DoAboutUI(gFormatRecord);
        } else {
            sSPBasic = formatParamBlock->sSPBasic;
            if (gFormatRecord->advanceState == nullptr) {
                *gResult = errPlugInHostInsufficient;
                return;
            }

            if (gFormatRecord->HostSupports32BitCoordinates)
                gFormatRecord->PluginUsing32BitCoordinates = true;

            switch (selector) {
            case formatSelectorReadPrepare:    DoReadPrepare();    break;
            case formatSelectorReadStart:      DoReadStart();      break;
            case formatSelectorReadContinue:   DoReadContinue();   break;
            case formatSelectorReadFinish:     DoReadFinish();     break;

            case formatSelectorOptionsPrepare: DoOptionsPrepare(); break;
            case formatSelectorOptionsStart:   DoOptionsStart();   break;
            case formatSelectorOptionsContinue:                    break;
            case formatSelectorOptionsFinish:                      break;

            case formatSelectorEstimatePrepare: DoEstimatePrepare(); break;
            case formatSelectorEstimateStart:   DoEstimateStart();   break;
            case formatSelectorEstimateContinue:DoEstimateContinue();break;
            case formatSelectorEstimateFinish:  DoEstimateFinish();  break;

            case formatSelectorWritePrepare:   DoWritePrepare();   break;
            case formatSelectorWriteStart:     DoWriteStart();     break;
            case formatSelectorWriteContinue:  DoWriteContinue();  break;
            case formatSelectorWriteFinish:    DoWriteFinish();    break;

            case formatSelectorFilterFile:     DoFilterFile();     break;
            }
        }

        if (selector == formatSelectorAbout         ||
            selector == formatSelectorWriteFinish   ||
            selector == formatSelectorReadFinish    ||
            selector == formatSelectorOptionsFinish ||
            selector == formatSelectorEstimateFinish||
            selector == formatSelectorFilterFile    ||
            *gResult != noErr)
        {
#if __PIMac__
            UnLoadRuntimeFunctions();
#endif
            PIUSuitesRelease();
        }
    }
    catch (const std::exception& ex) {
#if __PIMac__
        UnLoadRuntimeFunctions();
#endif
        DoMessageUI("PAA Plugin — Unhandled Exception", std::string(ex.what()));
        if (result) *result = -1;
    }
    catch (...) {
#if __PIMac__
        UnLoadRuntimeFunctions();
#endif
        DoMessageUI("PAA Plugin — Unknown Error", "An unknown exception was thrown.");
        if (result) *result = -1;
    }
}

// -----------------------------------------------------------------------
// Filter
// -----------------------------------------------------------------------

static void DoFilterFile() {
    // Accept all — Photoshop already filters by extension from PiPL
}

// -----------------------------------------------------------------------
// Read — Prepare
// -----------------------------------------------------------------------

static void DoReadPrepare() {
    gFormatRecord->maxData = 0;
#if __PIMac__
    if (gFormatRecord->hostSupportsPOSIXIO)
        gFormatRecord->pluginUsingPOSIXIO = true;
#endif
}

// -----------------------------------------------------------------------
// Read — Start   (header-only pass to report dimensions / mode)
// -----------------------------------------------------------------------

static void DoReadStart() {
    FromStart();
    if (*gResult != noErr) return;

    int64_t filesize = GetFileSizeFFR();
    if (*gResult != noErr || filesize <= 0) return;

    std::vector<uint8_t> fileData(static_cast<size_t>(filesize));
    Read(filesize, fileData.data());
    if (*gResult != noErr) return;

    auto paa = grad_aff::Paa();
    try {
        paa.readPaa(fileData, true /* header only */);
    }
    catch (const std::runtime_error& ex) {
        DoMessageUI("PAA read error!", std::string("Error parsing header: ") + ex.what());
        *gResult = readErr;
        return;
    }

    auto width  = paa.mipMaps[0].width;
    auto height = paa.mipMaps[0].height;

    // Bit 15 set on width signals special Arma encoding — strip flag
    if ((width & 0x8000) != 0)
        width &= 0x7FFF;

    // ----------------------------------------------------------------
    // Determine image mode from PAA type class
    // ----------------------------------------------------------------
    bool hasAlpha     = false;
    bool isGrayscale  = false;

    // Check TAGG markers
    for (const auto& tagg : paa.taggs) {
        if (tagg.signature == "GGATGALF")  // "FLAGTAGG" reversed
            hasAlpha = true;
    }

    // Check type class for grayscale (AI88) detection
    switch (paa.typeOfPax) {
    case grad_aff::TypeOfPaX::GRAYwAlpha:
        isGrayscale = true;
        hasAlpha    = true;  // AI88 has 8-bit greyscale + 8-bit alpha
        break;
    case grad_aff::TypeOfPaX::DXT5:
    case grad_aff::TypeOfPaX::DXT3:
    case grad_aff::TypeOfPaX::RGBA4444:
    case grad_aff::TypeOfPaX::RGBA5551:
    case grad_aff::TypeOfPaX::RGBA8888:
        hasAlpha = true;
        break;
    case grad_aff::TypeOfPaX::DXT1:
    default:
        // keep hasAlpha as detected from TAGG
        break;
    }

    // ----------------------------------------------------------------
    // Report image geometry to Photoshop
    // ----------------------------------------------------------------
    if (isGrayscale) {
        gFormatRecord->imageMode = plugInModeGrayScale;
        gFormatRecord->planes    = hasAlpha ? 2 : 1;
        gFormatRecord->transparencyPlane = hasAlpha ? 1 : -1;
    } else {
        gFormatRecord->imageMode = plugInModeRGBColor;
        gFormatRecord->planes    = hasAlpha ? 4 : 3;
        gFormatRecord->transparencyPlane = hasAlpha ? 3 : -1;
    }

    gFormatRecord->imageSize.h   = gFormatRecord->imageSize32.h   = width;
    gFormatRecord->imageSize.v   = gFormatRecord->imageSize32.v   = height;
    gFormatRecord->depth         = 8;
}

// -----------------------------------------------------------------------
// Read — Continue   (deliver pixel data)
// -----------------------------------------------------------------------

static void DoReadContinue() {
    FromStart();
    if (*gResult != noErr) return;

    int64_t filesize = GetFileSizeFFR();
    if (*gResult != noErr || filesize <= 0) return;

    std::vector<uint8_t> fileData(static_cast<size_t>(filesize));
    Read(filesize, fileData.data());
    if (*gResult != noErr) return;

    auto paa = grad_aff::Paa();
    try {
        paa.readPaa(fileData);
    }
    catch (const std::runtime_error& ex) {
        DoMessageUI("PAA read error!", std::string("Error decoding image data: ") + ex.what());
        *gResult = readErr;
        return;
    }

    auto width  = paa.mipMaps[0].width;
    auto height = paa.mipMaps[0].height;
    auto data   = paa.mipMaps[0].data;  // always RGBA from grad_aff

    // Determine mode (reuse same logic as DoReadStart)
    bool hasAlpha    = false;
    bool isGrayscale = false;
    for (const auto& tagg : paa.taggs)
        if (tagg.signature == "GGATGALF") hasAlpha = true;

    switch (paa.typeOfPax) {
    case grad_aff::TypeOfPaX::GRAYwAlpha:
        isGrayscale = true;
        hasAlpha    = true;
        break;
    case grad_aff::TypeOfPaX::DXT5:
    case grad_aff::TypeOfPaX::DXT3:
    case grad_aff::TypeOfPaX::RGBA4444:
    case grad_aff::TypeOfPaX::RGBA5551:
    case grad_aff::TypeOfPaX::RGBA8888:
        hasAlpha = true;
        break;
    default:
        break;
    }

    // ----------------------------------------------------------------
    // Convert grad_aff's always-RGBA output to the target plane format
    // ----------------------------------------------------------------
    std::vector<uint8_t> converted;

    if (isGrayscale && hasAlpha) {
        // AI88: extract R as grey, keep A
        // grad_aff gives RGBA — take R for luma and A for alpha
        converted.reserve(static_cast<size_t>(width) * height * 2);
        for (size_t i = 0; i < data.size(); i += 4) {
            converted.push_back(data[i]);      // R → Grey
            converted.push_back(data[i + 3]);  // A
        }
        gFormatRecord->planes    = 2;
        gFormatRecord->transparencyPlane = 1;
    } else if (isGrayscale) {
        converted.reserve(static_cast<size_t>(width) * height);
        for (size_t i = 0; i < data.size(); i += 4)
            converted.push_back(data[i]);      // R → Grey
        gFormatRecord->planes    = 1;
        gFormatRecord->transparencyPlane = -1;
    } else if (!hasAlpha) {
        // RGB only — strip alpha channel
        converted.reserve(static_cast<size_t>(width) * height * 3);
        for (size_t i = 0; i < data.size(); i += 4) {
            converted.push_back(data[i]);
            converted.push_back(data[i + 1]);
            converted.push_back(data[i + 2]);
        }
        gFormatRecord->planes    = 3;
        gFormatRecord->transparencyPlane = -1;
    } else {
        // RGBA — pass through as-is
        converted = data;
        gFormatRecord->planes    = 4;
        gFormatRecord->transparencyPlane = 3;
    }

    gFormatRecord->planeBytes = 1;
    gFormatRecord->colBytes   = gFormatRecord->planeBytes * gFormatRecord->planes;
    gFormatRecord->rowBytes   = gFormatRecord->colBytes   * width;

    gFormatRecord->loPlane = 0;
    gFormatRecord->hiPlane = gFormatRecord->planes - 1;

    gFormatRecord->theRect.left    = gFormatRecord->theRect32.left  = 0;
    gFormatRecord->theRect.right   = gFormatRecord->theRect32.right = width;
    gFormatRecord->theRect.top     = gFormatRecord->theRect32.top   = 0;
    gFormatRecord->theRect.bottom  = gFormatRecord->theRect32.bottom= height;

    gFormatRecord->data = converted.data();
    gFormatRecord->advanceState();
    gFormatRecord->data = nullptr;
}

static void DoReadFinish() {}

// -----------------------------------------------------------------------
// Options
// -----------------------------------------------------------------------

static void DoOptionsPrepare() { gFormatRecord->maxData = 0; }
static void DoOptionsStart()   { gFormatRecord->data    = nullptr; }

// -----------------------------------------------------------------------
// Estimate
// -----------------------------------------------------------------------

static void DoEstimatePrepare() { gFormatRecord->maxData = 0; }

static void DoEstimateStart() {
    gFormatRecord->minDataBytes =
        static_cast<int64_t>(gFormatRecord->imageSize32.h) *
        static_cast<int64_t>(gFormatRecord->imageSize32.v) * 8;
    gFormatRecord->maxDataBytes = gFormatRecord->minDataBytes;
    gFormatRecord->data = nullptr;
}

static void DoEstimateContinue() {}
static void DoEstimateFinish()   {}

// -----------------------------------------------------------------------
// Write — Prepare
// -----------------------------------------------------------------------

static void DoWritePrepare() {
    gFormatRecord->maxData = 0;
#if __PIMac__
    if (gFormatRecord->hostSupportsPOSIXIO)
        gFormatRecord->pluginUsingPOSIXIO = true;
#endif
}

// -----------------------------------------------------------------------
// Write — Start   (collect pixel data and encode to PAA)
// -----------------------------------------------------------------------

static void DoWriteStart() {
    if (gFormatRecord->HostSupports32BitCoordinates && 
        gFormatRecord->imageSize32.h &&
        gFormatRecord->imageSize32.v)
        gFormatRecord->PluginUsing32BitCoordinates = TRUE;

    const int32_t width  = gFormatRecord->PluginUsing32BitCoordinates
                         ? gFormatRecord->imageSize32.h
                         : gFormatRecord->imageSize.h;
    const int32_t height = gFormatRecord->PluginUsing32BitCoordinates
                         ? gFormatRecord->imageSize32.v
                         : gFormatRecord->imageSize.v;

    // PAA spec: dimensions must be powers of 2
    if (!isPowerOfTwo(static_cast<uint32_t>(width)) ||
        !isPowerOfTwo(static_cast<uint32_t>(height))) {
        DoMessageUI("PAA save error!",
            "Image dimensions must be powers of 2 (e.g. 512x512, 1024x2048).\n"
            "Current size: " + std::to_string(width) + "x" + std::to_string(height));
        *gResult = noErr;
        return;
    }

    // Supported image modes
    const bool isRGB       = (gFormatRecord->imageMode == plugInModeRGBColor);
    const bool isGrayscale = (gFormatRecord->imageMode == plugInModeGrayScale);

    if (!isRGB && !isGrayscale) {
        DoMessageUI("PAA save error!",
            "Only RGB Color and Grayscale image modes are supported.\n"
            "Convert the image mode before saving as PAA.");
        *gResult = noErr;
        return;
    }

    const bool hasAlpha = (gFormatRecord->planes == 4) ||
                          (isGrayscale && gFormatRecord->planes == 2);

    // ---------------------------------------------------------------
    // Request pixel data from Photoshop
    // ---------------------------------------------------------------
    gFormatRecord->depth      = 8;
    gFormatRecord->loPlane    = 0;
    gFormatRecord->hiPlane    = gFormatRecord->planes - 1;
    gFormatRecord->planeBytes = 1;
    gFormatRecord->colBytes   = gFormatRecord->planeBytes * gFormatRecord->planes;
    gFormatRecord->rowBytes   = gFormatRecord->colBytes   * width;

    gFormatRecord->theRect.left    = gFormatRecord->theRect32.left  = 0;
    gFormatRecord->theRect.right   = gFormatRecord->theRect32.right = width;
    gFormatRecord->theRect.top     = gFormatRecord->theRect32.top   = 0;
    gFormatRecord->theRect.bottom  = gFormatRecord->theRect32.bottom= height;

    const size_t pixelBytes = static_cast<size_t>(width) * height * gFormatRecord->planes;
    std::vector<uint8_t> psData(pixelBytes);
    gFormatRecord->data = psData.data();
    gFormatRecord->advanceState();
    gFormatRecord->data = nullptr;

    // ---------------------------------------------------------------
    // Convert Photoshop pixel layout → grad_aff RGBA
    // ---------------------------------------------------------------
    std::vector<uint8_t> rgbaData;
    rgbaData.reserve(static_cast<size_t>(width) * height * 4);

    if (isGrayscale) {
        // Grayscale (1 or 2 planes) → RGBA by replicating grey into R,G,B
        const size_t planes = static_cast<size_t>(gFormatRecord->planes);
        for (size_t i = 0; i < pixelBytes; i += planes) {
            uint8_t grey  = psData[i];
            uint8_t alpha = (planes == 2) ? psData[i + 1] : 255;
            rgbaData.push_back(grey);
            rgbaData.push_back(grey);
            rgbaData.push_back(grey);
            rgbaData.push_back(alpha);
        }
    } else if (gFormatRecord->planes == 3) {
        // RGB → RGBA
        for (size_t i = 0; i < pixelBytes; i += 3) {
            rgbaData.push_back(psData[i]);
            rgbaData.push_back(psData[i + 1]);
            rgbaData.push_back(psData[i + 2]);
            rgbaData.push_back(255);
        }
    } else {
        // RGBA — pass through
        rgbaData = psData;
    }

    // ---------------------------------------------------------------
    // Build grad_aff Paa and encode
    // ---------------------------------------------------------------
    auto paa = grad_aff::Paa();
    paa.typeOfPax    = PickPaaTypeClass(hasAlpha, isGrayscale);
    paa.hasTransparency = hasAlpha;

    grad_aff::MipMap mipMap;
    mipMap.width      = static_cast<uint32_t>(width);
    mipMap.height     = static_cast<uint32_t>(height);
    mipMap.data       = rgbaData;
    mipMap.dataLength = static_cast<uint32_t>(mipMap.data.size());

    paa.mipMaps.clear();
    paa.mipMaps.push_back(std::move(mipMap));

    std::vector<uint8_t> dataOut;
    try {
        paa.calculateMipmapsAndTaggs();
        dataOut = paa.writePaa();
    }
    catch (const std::runtime_error& ex) {
        DoMessageUI("PAA save error!", std::string("Error encoding image: ") + ex.what());
        *gResult = writErr;
        return;
    }

    Write(static_cast<int64_t>(dataOut.size()), dataOut.data());
}

static void DoWriteContinue() {}
static void DoWriteFinish()   {}
