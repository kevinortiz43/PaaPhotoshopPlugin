
# Gruppe Adler PAA Photoshop Plugin — 2026 Edition

This is my fork of Gruppe Adlee PAA Photoshop Plugin and update for 2026 Photoshop. PaahotoshopPlugin is a native C++ Photoshop File Format Plugin (`.8bi`) for opening and saving Arma 3 PAA texture files directly in Photoshop.

For the PAA file format specification see the [Bohemia Interactive Wiki](https://community.bistudio.com/wiki/PAA_File_Format).

---

### For End Users (No Building Required)

1. Skip to **[Installation (Pre-built)](#installation-pre-built)**
2. Download the latest `PaaFormat.8bi` from [Releases](https://github.com/kevinortiz43/PaaPhotoshopPlugin/releases)
3. Copy to Photoshop Plug-ins folder
4. Restart Photoshop

##If you want to build it yourself look below

## Quick Start

**First time building?** Follow these steps:

1. **Install prerequisites** (Visual Studio 2022, Windows SDK 10.0)
2. **Download Adobe Photoshop SDK 2026** from Adobe Developer Console
3. **Build grad_aff** library from source (see Step 3)
4. **Set environment variables** (`GRAD_AFF`, `GRAD_AFF_DEBUG`, `PS_DIR`)
5. **Build the plugin** using `PaaFormat.sln` in Visual Studio
6. **Copy the output** `.8bi` file to your Photoshop Plug-ins folder

For detailed instructions, see the **[BUILD GUIDE](BUILD.md)**.

---

## Supported PAA Types

| PAA Type | Read | Write | Notes |
|----------|------|-------|-------|
| DXT1 | ✅ | ✅ | RGB (no alpha) — most `_co.paa` color textures |
| DXT3 | ✅ | ❌ | RGBA, 4-bit alpha (auto-detected on read) |
| DXT5 | ✅ | ✅ | RGBA, smooth alpha — skins, decals, `_smdi.paa` |
| AI88  | ✅ | ✅ | Grayscale + alpha — open image in Grayscale mode to write |
| ARGB4444 | ✅ | ❌ | Legacy format (read-only) |
| ARGB1555 | ✅ | ❌ | Legacy format (read-only) |
| ARGB8888 | ✅ | ❌ | Uncompressed (read-only) |

**Write type is chosen automatically:**
- RGB image in Photoshop → DXT1
- RGBA image (with alpha channel) → DXT5
- Grayscale image → AI88

**Normal maps** (`_nohq.paa`): Open and edit normally — they are DXT5 files. The R/G channels contain X/Y normal data. Photoshop will display them with a blue-purple cast, which is correct.

---

## Limitations

- Image dimensions must be a power of 2 (e.g. 512×512, 1024×2048, 2048×2048). This is a PAA format requirement.
- Only RGB Color and Grayscale image modes are supported for writing.

---

## Installation (Pre-built)

1. Download the latest `PaaFormat.8bi` from [Releases](https://github.com/kevinortiz43/PaaPhotoshopPlugin/releases).
2. Copy `PaaFormat.8bi` into your Photoshop plug-ins folder:
   ```
   C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\
   ```
3. Restart Photoshop.
4. Test: **File → Open** → select any `.paa` file.

---

## Building from Source

### Requirements

| Tool | Version | Notes |
|------|---------|-------|
| Visual Studio | 2022 (any edition) | Must include **"Desktop development with C++"** workload |
| Windows SDK | 10.0 (latest) | Installed via VS2022 installer |
| Adobe Photoshop SDK | 2026 (v2-Dec-2025) | Download below |
| grad_aff | `dev` branch | Build from source |
| CMake | 3.20+ | For building grad_aff |

---

### Step 1 — Download the Adobe Photoshop SDK

1. Go to [https://developer.adobe.com/console/downloads/ps](https://developer.adobe.com/console/downloads/ps)
2. Sign in with your Adobe ID (free)
3. Download **"Photoshop Plug-in and Connection SDK" → Windows → App Version: 2026**
   - Do **not** download the UXP Hybrid Plugin SDK — that is for JavaScript UI panels
4. Unpack the ZIP. You will get a folder like: `adobe_photoshop_sdk_2026_win/`

**Verify SDK structure:**
```
adobe_photoshop_sdk_2026_win/
  pluginsdk/
    PhotoshopAPI/           ← C++ headers
    samplecode/
      common/               ← Shared utilities
        Includes/
        sources/
      Resources/            ← cnvtpipl.exe tool
      format/               ← Clone this repo here
```

---

### Step 2 — Set up the directory structure

The `.vcxproj` expects to find the Photoshop SDK exactly **3 levels above** the `PaaPhotoshopPlugin/` folder:

```
adobe_photoshop_sdk_2026_win/
  pluginsdk/
    PhotoshopAPI/           ← SDK C++ headers
    samplecode/
      common/               ← SDK shared utilities
        Includes/
        sources/
      Resources/            ← cnvtpipl.exe tool lives here
      format/               ← Clone this repo here
        PaaPhotoshopPlugin/ ← <— this repository
          win/
            PaaFormat.vcxproj
          common/
            PaaFormat.cpp
```

**Clone into the correct location:**
```bat
cd adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format
git clone https://github.com/kevinortiz43/PaaPhotoshopPlugin.git
```

**After cloning, your directory structure should look like:**
```
adobe_photoshop_sdk_2026_win\
├─ pluginsdk\
│  ├─ PhotoshopAPI\
│  ├─ PICA_SP\
│  └─ samplecode\
│     ├─ common\...
│     ├─ Resources\cnvtpipl.exe
│     └─ format\
│        └─ PaaPhotoshopPlugin\
│           ├─ README.md
│           ├─ common\...
│           └─ win\PaaFormat.vcxproj
```

---

### Step 3 — Build grad_aff

grad_aff is the C++ library that handles PAA decode/encode. Build it from source:

```bat
git clone https://github.com/kevinortiz43/grad_aff.git
cd grad_aff
git checkout dev

:: Build Release
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build --config Release
cmake --install build --prefix C:\grad_aff\release

:: Build Debug
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF
cmake --build build_debug --config Debug
cmake --install build_debug --prefix C:\grad_aff\debug
```

**Verify grad_aff build:** After installation, check that these files exist:
- `C:\grad_aff\release\include\grad_aff\paa\paa.h`
- `C:\grad_aff\release\lib\grad_aff.lib`
- `C:\grad_aff\debug\include\grad_aff\paa\paa.h`
- `C:\grad_aff\debug\lib\grad_aff.lib`

---

### Step 4 — Set Environment Variables

Open **System Properties → Advanced → Environment Variables** and add these **System** variables:

| Variable | Value | Purpose |
|----------|-------|---------|
| `GRAD_AFF` | `C:\grad_aff\release` | Release build of grad_aff |
| `GRAD_AFF_DEBUG` | `C:\grad_aff\debug` | Debug build of grad_aff |
| `PS_DIR` | `C:\Program Files\Adobe\Adobe Photoshop 2026` | Photoshop install dir |

**Important:** After setting environment variables, **restart Visual Studio** so it picks them up.

**Verify environment variables in Visual Studio:**
1. Open `PaaFormat.sln`
2. Open **Project → Properties**
3. Look at **VC++ Directories → Include Directories** and **Library Directories**
4. You should see `$(GRAD_AFF)\include` and `$(GRAD_AFF)\lib` resolved to your paths

---

### Step 5 — Build the Plugin

1. Open `PaaPhotoshopPlugin\win\PaaFormat.sln` in **Visual Studio 2022**
2. Set configuration to **Release | x64** (do not use Win32 — Photoshop is 64-bit only)
3. **Build → Build Solution** (`Ctrl+Shift+B`)

**Expected output:**
- Build succeeds with no errors
- Output file: `..\..\..\..\Output\Win\Release64\PaaFormat.8bi`

If `PS_DIR` is set correctly, the post-build event **automatically copies** `PaaFormat.8bi` to your Photoshop Plug-ins folder:
```
C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\PaaFormat.8bi
```

**Build configurations available:**
- `Release|x64` — Optimized release build (recommended for deployment)
- `Debug|x64` — Debug build with symbols (for development)

---

### Step 6 — Test the Plugin

1. **Restart Photoshop** (or launch it fresh if it was already running)
2. **File → Open** → navigate to any `.paa` file from your Arma 3 install
   - Example path (after extracting a PBO): `C:\Program Files (x86)\Steam\steamapps\common\Arma 3\addons\`
3. The image should open. Verify:
   - **Color textures** (`_co.paa`) → RGB image, no alpha channel
   - **Transparent textures** (`_ca.paa`) → RGBA with alpha channel visible
   - **Normal maps** (`_nohq.paa`) → purple/blue DXT5 image (correct normal map appearance)
   - **Grayscale/detail** (`_smdi.paa`, `_as.paa`) → grayscale or RGB
4. Make an edit, then **File → Save As** → select **PAA** from format dropdown
5. Verify the saved `.paa` works in Arma 3's **TexView 2** or in-game

---

## Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `Cannot open include file: 'PIDefines.h'` | SDK path incorrect | Verify you cloned the repo into `adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format\` |
| `Cannot open include file: 'grad_aff/paa/paa.h'` | grad_aff not built or env vars wrong | Build grad_aff first; verify `GRAD_AFF` points to release folder |
| `LINK: fatal error LNK1181: grad_aff.lib not found` | grad_aff library not found | Ensure you built with `-DBUILD_SHARED_LIBS=OFF` and installed to `C:\grad_aff\release\lib` |
| `'cnvtpipl' is not recognized` | SDK `Resources/cnvtpipl.exe` missing | Re-extract the Adobe SDK ZIP; ensure `Resources/` folder exists |
| Post-build copy fails | `PS_DIR` not set or wrong | Set `PS_DIR` to your Photoshop 2026 install path; or manually copy `.8bi` to Plug-ins folder |
| `The specified module could not be found` when loading plugin | grad_aff.dll not in PATH (if built as shared) | Rebuild grad_aff with `-DBUILD_SHARED_LIBS=OFF` or copy `grad_aff.dll` to Photoshop Plug-ins folder |

---

## How to Use This README

This README serves as the **primary documentation** for:

1. **Understanding the plugin** — What it does, supported formats, limitations
2. **Pre-built installation** — Copy `.8bi` to Plug-ins folder (end users)
3. **Building from source** — Step-by-step guide for developers (see below)

### For Developers (Building From Source)

Follow the complete build process:

1. **[Quick Start](#quick-start)** — Overview of all steps
2. **[Requirements](#requirements)** — Install Visual Studio 2022 with Windows SDK 10.0
3. **[Step 1 — Download Adobe Photoshop SDK 2026](#step-1---download-the-adobe-photoshop-sdk)** — Get the official SDK from Adobe
4. **[Step 2 — Set up directory structure](#step-2---set-up-the-directory-structure)** — Clone into the correct location
5. **[Step 3 — Build grad_aff](#step-3---build-grad_aff)** — Compile the PAA codec library
6. **[Step 4 — Set environment variables](#step-4---set-environment-variables)** — Configure paths for VS
7. **[Step 5 — Build the plugin](#step-5---build-the-plugin)** — Compile in Visual Studio
8. **[Step 6 — Test](#step-6---test)** — Verify it works

**Need more detail?** See **[BUILD.md](BUILD.md)** for an expanded, annotated build guide with additional troubleshooting tips.

---

## Project Structure

```
PaaPhotoshopPlugin/               (this repository)
├─ README.md                      (this file)
├─ BUILD.md                       (detailed build guide — read this for help)
├─ common/
│  ├─ PaaFormat.cpp              (plugin implementation)
│  ├─ PaaFormat.h                (declarations)
│  ├─ PaaFormat.r                (PiPL resource script)
│  └─ PaaFormatTerminology.h     (Photoshop terminology)
├─ win/
│  ├─ PaaFormat.sln              (Visual Studio solution)
│  ├─ PaaFormat.vcxproj          (project file)
│  ├─ AboutDialog.cpp/.h         (About UI)
│  ├─ PaaFormatUIWin.cpp         (Windows UI implementation)
│  └─ paaformat-sym.h            (Symbol exports)
└─ mac/                          (macOS support — not used for 2026 Windows build)
```

---

## Dependencies

- [Adobe Photoshop SDK 2026](https://developer.adobe.com/console/downloads/ps) — required to build
- [grad_aff](https://github.com/kevinortiz43/grad_aff/tree/dev) — PAA codec library (dev branch)

---

## Technical Notes

### Photoshop Plugin Architecture

This plugin implements the Photoshop File Format Plug-in API (PICA/Pre-CC 2015). It handles:

- **Read selectors**: `formatSelectorReadPrepare`, `formatSelectorReadStart`, `formatSelectorReadContinue`, `formatSelectorReadFinish`
- **Write selectors**: `formatSelectorWritePrepare`, `formatSelectorWriteStart`, `formatSelectorWriteContinue`, `formatSelectorWriteFinish`
- **Options/Estimate selectors**: Dialog-less operation

### PAA Format Handling

The plugin delegates all PAA encoding/decoding to the `grad_aff` library:

- **Reading**: Parse PAA header → determine type (DXT1/3/5, AI88, etc.) → decode → convert to Photoshop pixel layout
- **Writing**: Receive pixel data from Photoshop → convert to RGBA → pick type based on image mode → encode with mips → write

### Mipmap Generation

When writing, `paa.calculateMipmapsAndTaggs()` automatically generates the full mipchain. The output PAA will include all mip levels down to 1×1.

---

## License

See [LICENSE](LICENSE) file.

---

## Credits

- Plugin template: [Adobe Photoshop SDK](https://developer.adobe.com/console/downloads/ps)
- PAA codec: [grad_aff](https://github.com/kevinortiz43/grad_aff) by Gruppe Adler
- Icon: AdlerPS.png (Gruppe Adler logo)
