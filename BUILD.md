# BUILD GUIDE — PaaPhotoshopPlugin 2026

This guide provides detailed, step-by-step instructions for building the PaaPhotoshopPlugin from source with the Adobe Photoshop SDK 2026.

**Quick reference:** See [README.md](README.md) for an overview. Use this document for expanded explanations and troubleshooting.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Pre-Build Checklist](#pre-build-checklist)
3. [Step 1: Download Adobe Photoshop SDK 2026](#step-1-download-adobe-photoshop-sdk-2026)
4. [Step 2: Clone Repository in Correct Location](#step-2-clone-repository-in-correct-location)
5. [Step 3: Build and Install grad_aff](#step-3-build-and-install-grad_aff)
6. [Step 4: Set Environment Variables](#step-4-set-environment-variables)
7. [Step 5: Build the Plugin in Visual Studio](#step-5-build-the-plugin-in-visual-studio)
8. [Step 6: Install and Test](#step-6-install-and-test)
9. [Troubleshooting](#troubleshooting)

---

## Prerequisites

Before you begin, ensure you have:

- **Windows 10 or 11** (64-bit)
- **Visual Studio 2022** with:
  - Workload: "Desktop development with C++"
  - Windows 10 SDK (10.0.xxxxx.x) — select latest version
- **Git** (for cloning repositories)
- **CMake** 3.20+ (either from VS2022 installer or standalone)
- **vcpkg** (for installing grad_aff dependencies)
- **Adobe Photoshop 2026** installed (for testing)
- **Python** (optional, for some CMake configurations)

### Verify Visual Studio Installation

Open **Visual Studio Installer** → **Modify** your VS2022 installation:

1. **Workloads** tab:
   - ✅ Desktop development with C++
2. **Individual components** tab:
   - ✅ Windows 10 SDK (10.0.xxxxx.x)
   - ✅ C++ CMake tools for Windows

### Install vcpkg (if not already installed)

vcpkg is C++ package manager needed for grad_aff's dependency `lzokay`.

```bat
:: Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg

:: Bootstrap vcpkg (run in Command Prompt, not PowerShell)
cd C:\vcpkg
bootstrap-vcpkg.bat

:: Optional: integrate with Visual Studio (makes CMake auto-find packages)
C:\vcpkg\vcpkg.exe integrate install
```

**Important notes:**

- Use **Command Prompt (cmd.exe)** for these commands, not PowerShell. PowerShell requires `.\` prefix for local executables.
- After installing vcpkg, **restart Visual Studio** so it picks up the integration.
- If you must use PowerShell, use `.\bootstrap-vcpkg.bat` and `.\vcpkg.exe integrate install`.

### Install lzokay with vcpkg

grad_aff requires the `lzokay` library (LZO compression). Install it via vcpkg:

```bat
C:\vcpkg\vcpkg.exe install lzokay:x64-windows-static
```

This installs lzokay to `C:\vcpkg\installed\x64-windows-static`.

---

## Pre-Build Checklist

- [ ] Visual Studio 2022 installed with Windows SDK 10.0
- [ ] Git installed and in PATH
- [ ] CMake installed (run `cmake --version` to verify)
- [ ] vcpkg installed and `lzokay:x64-windows-static` package installed
- [ ] At least 5 GB free disk space
- [ ] Admin rights (for copying to Program Files, setting system env vars)

---

## Step 1: Download Adobe Photoshop SDK 2026

### 1.1 Access Adobe Developer Console

1. Open browser → [https://developer.adobe.com/console/downloads/ps](https://developer.adobe.com/console/downloads/ps)
2. Sign in with your Adobe ID (create one free if needed)
3. Accept terms of service

### 1.2 Download the Correct SDK

**Important:** There are two different Photoshop SDKs:

| SDK                                            | Use for                                         | Do NOT use for          |
| ---------------------------------------------- | ----------------------------------------------- | ----------------------- |
| **Photoshop Plug-in and Connection SDK** | **This plugin** (C++ file format plugins) | JavaScript UXP panels   |
| **UXP Hybrid Plugin SDK**                | JavaScript-based panels                         | C++ file format plugins |

**Download:**

1. Select **"Photoshop Plug-in and Connection SDK"**
2. Platform: **Windows**
3. App Version: **2026** (not 2025 or earlier)
4. Click **Download**

The file will be named something like:

```
adobe_photoshop_sdk_2026_win_xxx.zip
```

### 1.3 Extract the SDK

**Choose a permanent location** (you'll need the SDK headers during builds):

```bat
:: Example: extract to D:\SDKs\
mkdir D:\SDKs
:: Right-click ZIP → Extract All... → D:\SDKs\
```

**Verify the extracted structure:**

```
D:\SDKs\adobe_photoshop_sdk_2026_win\
├─ pluginsdk\
│  ├─ PhotoshopAPI\          ← Header files (PIDefines.h, PIFormat.h, etc.)
│  ├─ PICA_SP\               ← PICA API headers
│  └─ samplecode\
│     ├─ common\             ← Shared source utilities
│     ├─ Resources\          ← cnvtpipl.exe tool
│     └─ format\             ← Where your plugin will be cloned
```

**Do NOT move or rename** the SDK folder — the `.vcxproj` uses relative paths expecting exactly this structure.

---

## Step 2: Clone Repository in Correct Location

### 2.1 Why Location Matters

The `PaaFormat.vcxproj` file contains include paths like:

```
..\..\..\..\PhotoshopAPI\Photoshop
..\..\..\..\common\sources
```

This means it expects the SDK to be **3 directory levels above** the plugin's `win/` folder.

### 2.2 Clone to the Correct Path

Navigate to the SDK's `samplecode/format/` directory and clone THIS repository there:

```bat
:: Change to your SDK location
cd D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format

:: Clone this repository
git clone https://github.com/gruppe-adler/PaaPhotoshopPlugin.git
```

**Resulting directory tree:**

```
D:\SDKs\adobe_photoshop_sdk_2026_win\
├─ pluginsdk\
│  ├─ PhotoshopAPI\
│  ├─ PICA_SP\
│  └─ samplecode\
│     ├─ common\              ← SDK common code
│     ├─ Resources\cnvtpipl.exe
│     └─ format\
│        └─ PaaPhotoshopPlugin\   ← ← THIS REPO
│           ├─ README.md
│           ├─ common\
│           └─ win\PaaFormat.vcxproj
```

### 2.3 Verify the Structure

Check that these files exist:

**Required:**

- `.../pluginsdk/PhotoshopAPI/PIDefines.h` ← Core Photoshop API
- `.../pluginsdk/PhotoshopAPI/PIFormat.h` ← File format interface
- `.../pluginsdk/samplecode/Resources/cnvtpipl.exe` ← PiPL compiler

**Your plugin:**

- `PaaPhotoshopPlugin/common/PaaFormat.cpp`
- `PaaPhotoshopPlugin/win/PaaFormat.vcxproj`

If any are missing, the SDK extraction was incomplete — re-extract the ZIP.

---

## Step 3: Build and Install grad_aff

grad_aff is the C++ library that handles PAA encoding/decoding. You must build it **before** building the plugin.

### 3.1 Clone grad_aff

grad_aff is a separate dependency and should be cloned **outside** both:

- The Adobe Photoshop SDK folder
- This PaaPhotoshopPlugin repository

Choose a location where you have write permissions (e.g., `C:\grad_aff` or `D:\deps\grad_aff`):

```bat
:: Recommended: install to C:\grad_aff\ (source + build)
cd C:\
git clone https://github.com/gruppe-adler/grad_aff.git
```

### 3.2 Build grad_aff with CMake

**Important:** Build **both** Debug and Release configurations, and install to separate directories.

```bat
cd C:\grad_aff
git checkout dev

:: ---- RELEASE ----
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build --config Release
cmake --install build --prefix C:\grad_aff\release

:: ---- DEBUG ----
cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF
cmake --build build_debug --config Debug
cmake --install build_debug --prefix C:\grad_aff\debug
```

**Flags explained:**

- `-DBUILD_SHARED_LIBS=OFF` → Builds static library `grad_aff.lib` (recommended)
  - No DLL needed at runtime
  - Avoids "module not found" errors
- `--prefix` → Install location (used by the plugin's `.vcxproj` via env vars)

**Note:** grad_aff requires the `lzokay` library. If you installed it via vcpkg (see Prerequisites), CMake should find it automatically. If you get an error "Could not find a package configuration file provided by 'lzokay'", verify:

- vcpkg is installed and integrated (`C:\vcpkg\vcpkg.exe integrate install`)
- lzokay is installed: `C:\vcpkg\vcpkg.exe install lzokay:x64-windows-static`
- You may need to pass `-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake` to CMake

### 3.3 Verify grad_aff Installation

After building, check these files exist:

**Release:**

- `C:\grad_aff\release\include\grad_aff\paa\paa.h`
- `C:\grad_aff\release\lib\grad_aff.lib` (or `lib\x64\grad_aff.lib` on some CMake versions)

**Debug:**

- `C:\grad_aff\debug\include\grad_aff\paa\paa.h`
- `C:\grad_aff\debug\lib\grad_aff.lib` (or `lib\x64\grad_aff.lib`)

If `.lib` files are in `lib/x64/`, adjust your environment variable to `C:\grad_aff\release\lib\x64`.

### 3.4 grad_aff Build Troubleshooting

| Symptom                                      | Likely cause                        | Solution                                                                                           |
| -------------------------------------------- | ----------------------------------- | -------------------------------------------------------------------------------------------------- |
| CMake can't find generator                   | VS2022 not in PATH                  | Use**Developer Command Prompt** for VS2022, or specify `-G "Visual Studio 17 2022" -A x64` |
| `Could not find package "lzokay"`          | lzokay not installed via vcpkg      | Install:`C:\vcpkg\vcpkg.exe install lzokay:x64-windows-static`                                   |
| `grad_aff.lib` not found after install     | CMake used different install layout | Search:`dir /s grad_aff.lib C:\grad_aff`; update env var accordingly                             |
| Header not found in `include/grad_aff/paa` | grad_aff source structure changed   | Verify you're on `dev` branch: `git checkout dev`                                              |
| Link errors about missing symbols            | Built as shared but no DLL          | Rebuild with `-DBUILD_SHARED_LIBS=OFF`                                                           |

---

## Step 4: Set Environment Variables

The plugin's `.vcxproj` uses these environment variables to locate dependencies:

| Variable           | Value                                     | Example                                         |
| ------------------ | ----------------------------------------- | ----------------------------------------------- |
| `GRAD_AFF`       | Path to**release** grad_aff install | `C:\grad_aff\release`                         |
| `GRAD_AFF_DEBUG` | Path to**debug** grad_aff install   | `C:\grad_aff\debug`                           |
| `PS_DIR`         | Photoshop 2026 installation folder        | `C:\Program Files\Adobe\Adobe Photoshop 2026` |

### 4.1 Set System Environment Variables

1. Press `Win+R` → `sysdm.cpl` → **Advanced** tab → **Environment Variables**
2. Under **System variables**, click **New**:
   - **Variable name:** `GRAD_AFF`
   - **Variable value:** `C:\grad_aff\release`
3. Click **New** again:
   - **Variable name:** `GRAD_AFF_DEBUG`
   - **Variable value:** `C:\grad_aff\debug`
4. Click **New** again:
   - **Variable name:** `PS_DIR`
   - **Variable value:** `C:\Program Files\Adobe\Adobe Photoshop 2026`
5. Click **OK** → **OK** → **OK**

### 4.2 Restart Visual Studio

**Critical:** Close and re-open Visual Studio so it inherits the new environment variables. If you had `PaaFormat.sln` open, close it and reopen.

### 4.3 Verify Environment Variables in Visual Studio

1. Open `PaaPhotoshopPlugin\win\PaaFormat.sln`
2. **Build** → **Configuration Manager** → ensure **Active solution configuration** is `Release` and **Active solution platform** is `x64`
3. Right-click project → **Properties**
4. **Configuration Properties** → **VC++ Directories**
   - **Include Directories** should contain: `$(GRAD_AFF)\include`
   - **Library Directories** should contain: `$(GRAD_AFF)\lib`
5. Click **Apply** → **OK**

**Test:** In the **Output** window (show **Build** output), you should see lines like:

```
Using include path: C:\grad_aff\release\include
Using library path: C:\grad_aff\release\lib
```

---

## Step 5: Build the Plugin in Visual Studio

### 5.1 Open the Solution

Navigate to your SDK location and open the solution:

```
D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format\PaaPhotoshopPlugin\win\PaaFormat.sln
```

Double-click `PaaFormat.sln` or open from Visual Studio.

### 5.2 Select Build Configuration

1. Toolbar → **Solution Configurations**: `Debug` or `Release` (start with Release)
2. Toolbar → **Solution Platforms**: `x64` (not Win32!)
   - If `x64` not available: **Build** → **Configuration Manager** → Active solution platform → `<New...>` → select `x64`

### 5.3 Build

**Build Solution:**

- Menu: **Build** → **Build Solution**
- Shortcut: `Ctrl+Shift+B`
- Right-click project → **Build**

**Expected output (Release):**

```
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========
```

**Output file location:**

```
D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format\PaaPhotoshopPlugin\..\..\..\..\Output\Win\Release64\PaaFormat.8bi
```

That's 4 directories up from `win/`, into the SDK's `Output\Win\Release64\` folder.

### 5.4 Post-Build Event — Automatic Copy

If `PS_DIR` is set correctly, Visual Studio runs this post-build command automatically:

```bat
copy /Y "..\..\..\..\Output\Win\Release64\PaaFormat.8bi" "C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\PaaFormat.8bi"
```

**Check the Output window** for:

```
1>  Copying file to "C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\PaaFormat.8bi".
```

If the copy fails:

- Permission error → Run Visual Studio as **Administrator**
- Path not found → Verify `PS_DIR` points to correct Photoshop install
- Access denied → Close Photoshop before building

### 5.5 Build Outputs

**Debug build (`Debug|x64`):**

- Output: `...\Output\Win\Debug64\PaaFormat.8bi`
- Also copies `PaaFormat.pdb` (debug symbols) to Photoshop Plug-ins folder

**Release build (`Release|x64`):**

- Output: `...\Output\Win\Release64\PaaFormat.8bi`
- No `.pdb` file (optimized)

---

## Step 6: Install and Test

### 6.1 Verify Plugin is Installed

Check that the file exists:

```
C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\PaaFormat.8bi
```

If not, manually copy from the SDK Output folder:

```bat
copy "D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format\PaaPhotoshopPlugin\..\..\..\..\Output\Win\Release64\PaaFormat.8bi" "C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\"
```

### 6.2 Load Test Image

1. **Restart Photoshop** (must restart to detect new plugin)
2. **File → Open** → browse to an Arma 3 `.paa` file
   - You may need to extract PBOs first (use **PBO Manager** or **Arma 3 Tools**)
   - Example locations:
     ```
     C:\Program Files (x86)\Steam\steamapps\common\Arma 3\addons\
     C:\Program Files (x86)\Steam\steamapps\workshop\content\107410\
     ```
3. Select a `.paa` file → **Open**

**Expected results:**

| File type                      | Photoshop mode | Alpha channel | Appearance                             |
| ------------------------------ | -------------- | ------------- | -------------------------------------- |
| `*_co.paa` (color)           | RGB Color      | No            | Normal image                           |
| `*_ca.paa` (camo/alpha)      | RGB Color      | Yes           | Checkerboard transparency              |
| `*_nohq.paa` (normal)        | RGB Color      | Yes           | Blue-purple tint (correct for normals) |
| `*_smdi.paa` (smooth damage) | Grayscale      | Yes           | Grayscale with alpha                   |
| `*_as.paa` (addon surface)   | RGB Color      | No            | Normal image                           |

If the file opens without error, the plugin is working.

### 6.3 Test Write Functionality

1. Open a `.paa` file in Photoshop
2. Make a small edit (e.g., brightness/contrast adjustment)
3. **File → Save As**
4. Choose **PAA** from format dropdown
5. Save to a test folder
6. Verify the new `.paa` opens in **TexView 2** (Arma 3 tool) or loads in-game

**Write mode rules:**

- **RGB Color** → DXT1 (no alpha) or DXT5 (with alpha) depending on layers
- **Grayscale** → AI88

---

## Troubleshooting

### Build Errors

#### Error: `Cannot open include file: 'PIDefines.h'`

**Cause:** Photoshop SDK path is wrong or incomplete.

**Fix:**

1. Verify SDK extraction: Check `D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\PhotoshopAPI\PIDefines.h`
2. Verify repo location: `PaaPhotoshopPlugin` must be in `samplecode/format/`
3. If you moved the SDK, you must **re-clone** the plugin in the correct relative location

---

#### Error: `Could not find package "lzokay"` during grad_aff CMake

**Cause:** lzokay library not installed via vcpkg.

**Fix:**

1. Install vcpkg (if not already): see Prerequisites
2. Install lzokay: `C:\vcpkg\vcpkg.exe install lzokay:x64-windows-static`
3. Re-run grad_aff CMake with vcpkg toolchain:`cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake`
4. Or set `CMAKE_PREFIX_PATH=C:\vcpkg\installed\x64-windows-static`

---

#### Error: `Cannot open include file: 'grad_aff/paa/paa.h'`

**Cause:** grad_aff not built, or `GRAD_AFF` environment variable points to wrong location.

**Fix:**

1. Verify grad_aff header exists: `C:\grad_aff\release\include\grad_aff\paa\paa.h`
2. Open **Command Prompt** → `echo %GRAD_AFF%`
   - Should output `C:\grad_aff\release`
3. If wrong, set env var correctly and **restart Visual Studio**

---

#### Error: `LINK: fatal error LNK1181: grad_aff.lib not found`

**Cause:** grad_aff library not in expected location.

**Fix:**

1. Check: `C:\grad_aff\release\lib\grad_aff.lib`
2. If not there, search: `dir /s grad_aff.lib C:\grad_aff`
3. If found in `lib\x64\`, set `GRAD_AFF` to `C:\grad_aff\release\lib\x64`
4. If not built with `-DBUILD_SHARED_LIBS=OFF`, rebuild (you need `.lib`, not `.dll`)

---

#### Error: `'cnvtpipl' is not recognized as an internal or external command`

**Cause:** The Adobe SDK's `Resources/cnvtpipl.exe` is missing.

**Fix:**

1. Verify: `D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\Resources\cnvtpipl.exe`
2. If missing, the SDK ZIP was incompletely extracted — re-extract
3. Ensure no antivirus quarantined `cnvtpipl.exe` (it's a legitimate tool)

---

#### Post-build copy fails with "Access denied"

**Cause:** Cannot write to `C:\Program Files\...` without admin rights.

**Fix:**

1. **Option A:** Close Photoshop, then **Run as Administrator** when building
2. **Option B:** Manually copy `.8bi` after build (no admin needed if you own the files)
3. **Option C:** Disable UAC (not recommended)

---

#### Error: `The specified module could not be found` when Photoshop loads plugin

**Cause:** If you built grad_aff as a **shared library** (`.dll`), the DLL is not in PATH.

**Fix:**

1. **Recommended:** Rebuild grad_aff with `-DBUILD_SHARED_LIBS=OFF` (static library)
2. **Alternative:** Copy `grad_aff.dll` to `C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\`
3. **Alternative:** Add grad_aff `bin/` directory to system PATH

---

### Photoshop Plugin Errors

#### Plugin doesn't appear in File → Open dialog

**Cause:** Plugin not loaded, or `PaaFormat.8bi` not in correct Plug-ins folder.

**Fix:**

1. Verify file exists: `C:\Program Files\Adobe\Adobe Photoshop 2026\Plug-ins\PaaFormat.8bi`
2. Restart Photoshop (completely exit, not just close windows)
3. Check **File → Open → Format dropdown** — should see "PAA (*.paa)"
4. If still missing, check **Plug-ins** panel in **Help → System Info** → **Plug-ins** section

---

#### PAA opens but colors are wrong / pink/green artifacts

**Cause:** Possible mipmap or DXT compression mismatch.

**Fix:** This is usually a grad_aff version mismatch:

- Ensure you're using `dev` branch, not `master`
- Rebuild grad_aff cleanly: delete `build/` folders, rebuild from scratch
- Try different build options: `-DCMAKE_BUILD_TYPE=Release` (no `RelWithDebInfo`)

---

#### Plugin crashes on certain PAA files

**Cause:** Unhandled edge case in PAA format (corrupt header, unknown type, etc.)

**Fix:**

1. Enable debug build: Build `Debug|x64` configuration
2. Run Photoshop from **Visual Studio** (Debug → Start Debugging)
3. Open problematic file → VS should break on exception
4. Check `Output` window for error message from `DoMessageUI`

**Report the issue** with:

- PAA file (if possible)
- Stack trace from VS
- SDK version, grad_aff commit hash

---

#### Save fails with "Disk is full!"

**Cause:** Plugin detected insufficient disk space during write.

**Fix:**

1. Check destination drive free space
2. Verify you have write permissions to target folder
3. If saving to network drive, try local drive instead

---

### Environment Variable Issues

#### `$(GRAD_AFF)` not resolved in VS

**Cause:** VS opened **before** setting env vars.

**Fix:** Close **all** Visual Studio instances, reopen solution. Verify in Property Manager (**View → Other Windows → Property Manager**) that `$(GRAD_AFF)` expands correctly.

---

#### Multiple SDK versions conflict

**Cause:** Old SDK folder still in include path from previous builds.

**Fix:**

1. In project properties → **VC++ Directories** → **Include Directories**
2. Remove any references to old SDK paths (e.g., `...\Photoshop CS6\...`)
3. Clean rebuild: **Build → Clean Solution**, then **Build → Rebuild Solution**

---

## Advanced Topics

### Building Without Environment Variables

If you prefer not to use system-wide environment variables, you can set them in the project:

1. Project → Properties → **Configuration Properties** → **General**
2. **VC++ Project Settings** → **Include Paths**: Add `C:\grad_aff\release\include`
3. **VC++ Project Settings** → **Library Paths**: Add `C:\grad_aff\release\lib`
4. Remove `$(GRAD_AFF)` from include/library paths (or override locally)

---

### Building with grad_aff as Shared Library (DLL)

While not recommended, you can build grad_aff as a shared library:

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
cmake --build build --config Release
cmake --install build --prefix C:\grad_aff\release
```

Then copy `grad_aff.dll` to Photoshop Plug-ins folder.

---

## Clean Rebuild

If builds behave strangely, do a full clean:

```bat
:: Close Visual Studio first

:: Delete grad_aff builds
rmdir /s /q C:\grad_aff\build
rmdir /s /q C:\grad_aff\build_debug
rmdir /s /q C:\grad_aff\release
rmdir /s /q C:\grad_aff\debug

:: Rebuild grad_aff
cd C:\grad_aff
git checkout dev
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
cmake --install build --prefix C:\grad_aff\release

:: Delete plugin output
rmdir /s /q "D:\SDKs\adobe_photoshop_sdk_2026_win\pluginsdk\samplecode\format\PaaPhotoshopPlugin\..\..\..\..\Output"

:: Reopen VS, rebuild plugin
```

---

## Verification Checklist

After successful build:

- [ ] `PaaFormat.8bi` exists in Photoshop Plug-ins folder
- [ ] Photoshop can open `.paa` files (File → Open → select PAA)
- [ ] Color textures display correctly (no pink/green corruption)
- [ ] Alpha channels show transparency (checkerboard)
- [ ] Saved `.paa` files can be re-opened in Photoshop
- [ ] Saved `.paa` files work in Arma 3 (TexView 2 or in-game)

If all checks pass, the plugin is successfully built and ready to use.

---

## Getting Help

- **Issues:** https://github.com/gruppe-adler/PaaPhotoshopPlugin/issues
- **Arma 3 modding:** https://discord.gg/gruppe-adler (Gruppe Adler Discord)
- **grad_aff library:** https://github.com/gruppe-adler/grad_aff
- **lzokay/vcpkg:** https://github.com/Microsoft/vcpkg

When reporting issues, include:

1. Photoshop version (2026)
2. SDK version (v2-Dec-2025)
3. grad_aff commit/branch
4. Full error message
5. Build log (from Visual Studio Output window)

---

**Last updated:** 2026-04-02
**Target SDK:** Adobe Photoshop 2026 (v2-Dec-2025)
