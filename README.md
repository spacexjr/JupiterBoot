
## JupiterBoot

A graphical, touch-driven boot menu injected into the `boot` ramdisk of the
Samsung Galaxy A22 4G (`SM-A225M` / `a22`).

JupiterBoot replaces the Android `/init` entry point with a freestanding
AArch64 boot menu. The original Android first-stage init is preserved as
`/init.system` and can be executed by the menu to continue the normal Android
boot process.

> **Warning:** JupiterBoot is experimental boot-chain software. Flashing a
> modified `boot.img` can prevent the device from booting if the image is
> incorrect. Always keep a known-good stock `boot.img` available.

The repository does **not** flash the device. The build system only produces
`out/boot-jbm.img`, which must be reviewed and flashed manually.

---

### Features

- Graphical framebuffer interface
- Touchscreen navigation
- Automatic boot countdown
- System boot
- Recovery reboot
- Download Mode reboot
- Device reboot
- Power off
- JupiterBoot graphical interface
- Host-side UI preview
- Freestanding AArch64 `/init`
- Original Android `/init` preserved as `/init.system`

---

### Boot Flow

1. The bootloader loads `boot.img`.
2. The kernel starts `/init`.
3. JupiterBoot initializes `/proc`, `/sys`, `/dev/fb0`, and touchscreen input.
4. The graphical boot menu is displayed.
5. The automatic boot countdown starts.
6. The user can select an action or allow the countdown to expire.
7. When **System** is selected, JupiterBoot executes `/init.system`.
8. `/init.system` continues the normal Android boot process.

If the framebuffer cannot be initialized, JupiterBoot immediately chains to
`/init.system` without displaying the graphical interface.

---

### Menu Actions

| Action | Hold | Behaviour |
|---|---:|---|
| **System** | Tap | Starts the installed Android system through `/init.system` |
| **Recovery** | 900 ms | Reboots with the `recovery` reason |
| **Fastboot** | 700 ms | Requests a reboot to the bootloader |
| **Download** | 900 ms | Reboots with the `download` reason |
| **Reboot** | 700 ms | Restarts the device |
| **Power Off** | 900 ms | Powers off the device |

Any touchscreen interaction cancels the automatic boot countdown.

---

### Tested on Real Hardware

**Samsung Galaxy A22 4G — SM-A225M**

### Confirmed working

- ✅ System
- ✅ Recovery
- ✅ Download
- ✅ Reboot
- ✅ Power Off
- ✅ Fastboot

The graphical **System starting screen** has been fixed and tested on real hardware.

### Charger / Offline Charging

When the device is powered off and connected to a charger, the JupiterBoot
menu can also appear during the device charging path. This suggests that the
boot image is being entered through the device charger/off-mode boot path.

The battery percentage displayed during this state appears to be obtained at
runtime from the device battery/charger state rather than being a fixed value
stored in the boot image. The exact bootloader/charger flag responsible for
entering this path has not yet been fully identified.

---

### Safety Model

JupiterBoot does not perform partition flashing or modification during normal
menu operation.

### Recovery

The Recovery action does not write to flash. It synchronizes pending writes
and requests a reboot with the `recovery` reason:

```c
reboot(
    LINUX_REBOOT_MAGIC1,
    LINUX_REBOOT_MAGIC2,
    LINUX_REBOOT_CMD_RESTART2,
    "recovery"
);
```

Whether the bootloader honors this reboot reason depends on the device
platform.

### Download

The Download action does not write to flash. It requests a reboot with the
`download` reason. There is no userspace flashing operation performed by
JupiterBoot.

### Other Actions

No menu action backs up, copies, overwrites, or modifies a partition.

The `boot` partition is not modified while JupiterBoot is running.

---

### Restoring the Stock Boot Image

Earlier JupiterBoot builds could create a backup at:

```text
/cache/jbmenu/boot.bak
```

That backup mechanism has been removed from the current version.

If you still have a backup from an older build, it can be restored from a
root shell or recovery:

```sh
dd if=/cache/jbmenu/boot.bak of=/dev/block/by-name/boot
sync
```

If no backup is available, restore the stock `boot.img` using your normal
Samsung flashing method, such as Odin.

---

### Build Requirements

- `clang`
- AArch64 target support (`--target=aarch64-linux-gnu`)
- `lld`
- `nm`
- `file`
- `stat`
- `magiskboot`
- Python 3
- Pillow (only required when regenerating the font)

The original `boot.img` must be available in the repository root for packaging.
Host-only builds and the self-test do not need it.

---

### Building

```sh
./build.sh
```

Or specify another boot image:

```sh
./build.sh path/to/boot.img
```

Compile/selftest without a boot image:

```sh
./build.sh --host-only
```

The full build process:

1. Compiles the freestanding AArch64 JupiterBoot binary.
2. Verifies that the binary has no undefined symbols.
3. Builds the host UI preview and renders the preview images.
4. Runs the host self-test (config/entries/themes/BMP).
5. Unpacks the original `boot.img`.
6. Renames the original `/init` to `/init.system`.
7. Installs JupiterBoot as `/init`.
8. Repacks the boot image.
9. Verifies the resulting ramdisk and generated `/init`.
10. Verifies that the final image size is unchanged.

The build process **does not flash the device**.

---

### Build Output

```text
out/
├── jbm_init
├── boot-jbm.img
├── ui-preview.ppm
└── ui-preview-intro.ppm
```

`out/boot-jbm.img` is the modified boot image that must be manually reviewed
before flashing.

---

### UI Preview

The build system generates previews using the same drawing code used by the
JupiterBoot framebuffer interface. This allows the menu to be tested on the
host before deploying it to the device.

---

### Configuration

Build-time constants can be overridden through environment variables:

```sh
JBM_AUTOBOOT_SEC=10 JBM_VERSION=1.1 ./build.sh
```

### Runtime configuration

JupiterBoot reads `config.ini`, `entries.ini`, `themes/*.ini` and an optional
`background.bmp` from its config root:

* `/cache/Jupiter` — when `/cache` is available and holds Jupiter files.
* `/Jupiter` (ramdisk) — fallback used when `/cache` is unavailable.

On the first boot the menu creates the whole tree when it is missing:
`config.ini`, `entries.ini`, `background.bmp`, `themes/default.ini`,
`scripts/` and `bin/`. Existing files are never overwritten and writes are
atomic (temp file + rename). If the config root is not writable — for example
`/cache` did not mount — the failure is logged and the built-in defaults are
used. Delete one file to regenerate only that default on the next boot.

The generated `background.bmp` is a valid 24-bpp image matching the default
dark/warm backdrop; deleting it brings back the procedural space scene (which
also draws the planet).

Sample files live under `config/`; copy them into the config root and edit.
For host testing, point the menu at a scratch directory — it only writes there
and never to `/cache` on the PC:

```sh
JUPITER_CONFIG_ROOT=./testdata/Jupiter ./out/jbm_host
```

**`config.ini`** — colours are `#RRGGBB` or `0xRRGGBB` (opaque alpha is added
automatically). `orientation` accepts `0/90/180/270` or `auto` (follows the
framebuffer's reported rotation, and quarter turns like `1/2/3`).
`config_version` is currently `2`; older files are still accepted.

```ini
theme = default
config_version = 2
title = Jupiter
subtitle = Boot
show_title = 1
show_description = 1
show_icons = 1
icon_scale = 100
background = /cache/Jupiter/background.bmp   ; optional, BMP 24-bit only
color_bg = #020202
color_accent = #C87335
color_selected_bg = #B86837
color_pressed_bg = #D88A4E
color_disabled_text = #525862
button_style = 0     ; 0 solid, 1 outline, 2 minimal, 3 glass
button_width = 430
button_height = 94
button_spacing = 12
button_radius = 7
button_border_width = 1
button_alpha = 255
text_size = 19
text_align = 0       ; 0 left, 1 center, 2 right
layout_margin = 7    ; percent of screen width, 0..35
menu_timeout_sec = 15
default_entry = -1   ; -1 = honour the entry's `default = 1` flag
show_countdown = 1
show_brand = 1
orientation = auto
prefer_drm = 0
fallback_to_fb = 1
input_device =       ; empty = auto-detect the best touchscreen
touch_cal_x0 = -1    ; optional raw-axis overrides (-1 = unused)
log_file = 1
```

Only keys explicitly present in `config.ini` override the chosen theme; unset
keys fall back to the theme file, then to built-in defaults. This includes the
appearance keys above, so a theme can define them without being overridden.

**`entries.ini`** — one `[id]` section per menu entry. Each entry can override
the visual style of its row; omitted values inherit the theme:

```ini
[system]
name = System
description = Start Android
icon = system        ; system | recovery | fastboot | download | reboot | power | script
type = system        ; system | recovery | fastboot | download | reboot | poweroff | script | binary
order = 0
enabled = 1
default = 1
behavior_after = 0   ; 0 = return to menu, 1 = chain to real init, 2 = reboot
# button_style = 3
# button_width = 600
# button_height = 110
# button_radius = 24
# button_border_width = 2
# text_align = 1
# icon_scale = 120
# show_icon = 1
# color_bg = #101820
# color_border = #D88A4E
# color_text = #B0B0B0
# color_text_sel = #FFFFFF
```

`script`/`binary` entries run with `fork()`+`execve()` (no shell, no flashing,
no kexec). They must use absolute paths without `..`; the menu `chmod +x` them
before running. After they finish the menu returns unless `behavior_after` says
otherwise.

**`themes/<name>.ini`** — same colour/metric keys as `config.ini`, applied first
and then overridden by matching `config.ini` keys. `extends = <theme>` inherits
another theme (up to two levels). The firmware generates `default`, `jupiter`,
`amoled` and `minimal` when the theme tree is first created.

### Logging

Diagnostics go through `/dev/kmsg` and, when `log_file = 1`, are appended to
`<config root>/boot.log` (best effort). On the device, read them after boot
with `adb shell cat /cache/Jupiter/boot.log` (or `/Jupiter/boot.log`).

---

### Regenerating the Font

```sh
python3 tools/gen_font.py
```

This requires Pillow and regenerates:

```text
src/jbm_font.h
```

---

### Project Structure

```text
JupiterBoot/
├── src/
│   ├── jbm_menu.c
│   ├── start.S
│   └── jbm_font.h
├── tools/
│   └── gen_font.py
├── build.sh
├── boot.img
└── out/
```

---

### Device

| Property | Value |
|---|---|
| Device | Samsung Galaxy A22 4G |
| Model | SM-A225M |
| Codename | `a22` |
| SoC | MediaTek MT6768 |
| Architecture | AArch64 |
| Android target | Android 15 GSI |

---

### Current Status

**JupiterBoot 0.3 — Experimental**

The graphical boot menu is successfully running on real SM-A225M hardware.

System, Recovery, Download, Reboot, and Power Off have been confirmed on the
device. The graphical System starting screen is also working on real hardware.
Fastboot remains untested. The device can also enter the JupiterBoot UI while
powered off and charging, which is currently being investigated as a charger/off-mode
boot path.

### Known issue: `/cache` is not mounted

On the current SM-A225M build the menu does **not** mount `/cache`, so
`/cache/Jupiter` is never created and the runtime configuration is not loaded
from there. The boot log shows:

```
jbm: could not mount /cache, using defaults
jbm: config root /cache/Jupiter not writable, using built-in defaults
```

The cache partition itself is valid (`/dev/block/mmcblk0p46`, ext4,
`PARTNAME=cache`) and Android mounts it normally afterwards. The failure is in
the menu's early probe: `/sys/class/block` exposes 150 entries on this device
and `mmcblk0p46` is entry 124, while the scan was capped at 64. A fix (larger
scan buffer plus identification by `by-name/cache`, `PARTNAME=cache` and the
ext4 label) is implemented but **not yet validated on hardware**. Until it is
confirmed, the menu falls back to the built-in defaults and no files are
written to `/cache/Jupiter`.

---

### Disclaimer

This project is intended for development and experimentation.

Modifying and flashing boot images can result in a device that does not boot.
Always keep a known-good stock boot image and a reliable recovery method
available before testing.

---

# Jupiter — Android Boot Manager

`com.spacexjr.jupiter` · version 1.0.0 · minSdk 26 (Android 8) · targetSdk 35 · arm64-v8a

---

## What the app is

**Jupiter** is the Android app that manages the JupiterBoot boot menu straight
from the phone — no PC, no fastboot, no typing commands in a terminal. It is a
Kotlin + Jetpack Compose app built with Material You 3, designed to be operated
entirely by touch.

From the Android device itself you can:

- **Install** JupiterBoot into the boot image (a real, verified patch).
- **Edit** the menu entries (`entries.ini`) and visual settings (`config.ini`)
  through form screens.
- **Browse** system files with a built-in file manager.
- **Track** everything through an operation log.
- **Check for updates** to JupiterBoot via the GitHub API.

All wrapped in a Material You look (dynamic color) with a burgundy-rose planet
icon.

> **Important:** the app **never** writes directly to physical partitions. The
> final image stays in the app's private storage until you explicitly export it
> and flash it yourself (fastboot / recovery).

---

## How to use the app (step by step)

### First launch
1. Grant **root access** when the app asks (Magisk/KernelSU/APatch).
2. On the **Home** tab, check that the status shows *root available* and that the
   bundled binaries were installed (size + hash shown).
3. Optionally set a **local test root** path to explore without touching the real
   system.

### Applying the boot patch
1. Go to the **Patch** tab and tap **Scan**.
2. The app lists the partitions and picks the target automatically:
   - only `init_boot` → **HIGH** confidence;
   - only `boot` → **MEDIUM** confidence;
   - both → prefers `init_boot` and **asks for your confirmation**;
   - only `vendor_boot`/unknown → **aborts** with an explanation.
3. Review the stated reason and confirm.
4. The app extracts, unpacks, injects `jbm_init`, repacks and **verifies** the
   image, showing every step.
5. Use **Export** to save the final image and flash it yourself.

### Editing the boot menu
- **Entries** tab: add/remove/reorder entries, set the type (`system`,
  `recovery`, `fastboot`, `script`, `binary`, …), path, arguments, order and
  which entry is the default. The app validates before writing.
- Each entry can also override its own description, icon, button style, size,
  radius, alignment, icon scale and colors. Omitted values inherit the theme.

### Adjusting the appearance
- **Appearance** tab: live preview of the real menu layout (drawing only),
  title/subtitle, button style (solid/outline/minimal/glass), metrics, colors,
  text alignment, icons and descriptions.
- The preview uses the same effective theme + `config.ini` merge rules and the
  same row geometry as `src/jbm_menu.c`; no boot action can be triggered.
- **Theme** section lists `themes/*.ini`, supports `extends`, creates and saves
  themes with an automatic backup.
- **Background**: pick an image, crop/zoom/rotate it and export a 24-bit BMP as
  `background.bmp`. The firmware does not load PNG/JPEG, so the conversion is
  explicit and visible.
- **Config** tab: theme, colors (ARGB picker), button size, menu timeout,
  orientation, display (DRM/FB), logging, touch calibration and device profiles.
- Every write creates an **automatic backup**; backups can be restored.

### Managing files
- **Files** tab: browse folders, view size/permissions/owner, edit text files,
  create folders, apply `chmod +x`, back up/delete.
- The app does **not** run scripts silently.

### Tracking and updating
- **Logs** tab: full history of commands, output and exit codes.
- **Home → Updates**: queries the latest release from
  `https://api.github.com/repos/spacexjr/JupiterBoot/releases/latest`.
- Downloading and installing `jbm_init` is a separate, confirmed action. The
  app verifies the asset name, size, that it is an AArch64 ELF and, when the
  release publishes `jbm_init.sha256`, its SHA-256. It keeps the previous file
  as a backup and **never executes it** or touches a boot partition.
- A locally installed update is remembered independently from the bundled
  asset version, so update checks reflect the version actually installed.

---

## Features in detail

### Home
- Root status, local JupiterBoot version, model/device/kernel.
- State of the bundled binaries (installed, size, SHA-256 hash).
- Summary of the last executed patch.

### Patch (boot image)
Staged flow, always with user confirmation:

1. **Scan** of partitions (`/dev/block/by-name`, `bootdevice`, `mapper`).
2. **Target selection** (`PatchEngine.chooseTarget`):
   - only `init_boot` → **HIGH** confidence (GKI Android 13+);
   - only `boot` → **MEDIUM** confidence;
   - both → prefers `init_boot`, MEDIUM confidence (requires confirmation);
   - only `vendor_boot` or unknown architecture → **aborts** with an explanation.
3. **Extraction** with `dd` (10 min timeout) + size and SHA-256 verification.
4. **Unpack** with `magiskboot unpack -h`.
5. **Ramdisk inspection** (`init` and `init.system`).
6. **Integration**: original `/init` → `/init.system`; `jbm_init` → `/init` (0750).
7. **Repack** + full verification (size, hashes, presence of init).
8. Optional **export** of the final image to shared storage.

### Entries (entries.ini)
- Editing of boot menu entries (name, type, path, arguments, order, enabled,
  default, `behavior_after`).
- Supported types: `system`, `recovery`, `fastboot`, `download`, `reboot`,
  `poweroff`, `script`, `binary`.
- Validation before writing; unknown fields preserved.

### Appearance
- Preview + editors for the menu title/subtitle, button styles and per-entry
  overrides, colors, alignment, icons and descriptions.
- Theme inheritance (`extends`) resolved up to two levels.
- Background editor: crop, zoom, rotate and export to 24-bit BMP.
- **Config** tab: visual editing of theme, colors (ARGB), button metrics,
  timeout, orientation, display (DRM/FB), logging, touch calibration and
  device profiles.
- Parser faithful to the firmware (colors `#`/`0x`, bools `1/t/y/o`, `auto`→-1).
- Automatic backup before every write; backup restore.

### Files
- Filesystem browsing (private test root or system path via root).
- Text viewing/editing, size/permissions/owner info.
- `chmod +x`, folder creation, deletion, backup/restore.
- **Never** runs scripts silently.

### Logs
- History of every operation (commands, output, exit codes).
- Persisted to disk; capped at 2000 entries.

---

## Visual identity

The app icon is a **Jupiter-style planet** in burgundy-rose tones with an
orbital band, matching the app's Material You theme.

| Resource | File |
|----------|------|
| Adaptive icon | `res/mipmap-anydpi-v26/ic_launcher.xml` |
| Foreground layer (planet vector) | `res/drawable/ic_launcher_foreground.xml` |
| Background color | `res/values/ic_launcher_background.xml` (`#4A1226`) |

The icon uses the *adaptive icon* format (Android 8+): on devices with dynamic
theming, the system applies the mask and parallax automatically.

---

## Requirements

| Item | Requirement |
|------|-------------|
| Android | 8.0 (API 26) or newer |
| Architecture | arm64-v8a (AArch64) |
| Root | `su` (Magisk/KernelSU/APatch) for system operations |
| Space | ~40 MB free on internal storage |

Without root, the app opens and shows diagnostics, but does not patch or edit
system files.

---

## Code architecture

```
app/src/main/java/com/spacexjr/jupiter/
├── core/
│   ├── shell/       Sh (quoting/validation), ProcessRunner, RootService
│   ├── log/         OperationLog (StateFlow + persistence)
│   ├── util/        Hashes, Elf, Versions
│   ├── config/      IniLines, ConfigIni, EntriesIni, ThemeIni, MenuModel
│   ├── store/       ConfigRoot (device via su / local for tests)
│   ├── binaries/    BinaryInstaller (verify without overwrite)
│   ├── device/      DeviceProfile (orientation/touch profiles)
│   ├── image/       BmpWriter (crop/zoom/rotate, BMP 24-bit)
│   ├── patch/       PatchEngine, PatchModels, PatchStateStore
│   ├── update/      GitHubReleaseClient, UpdateInstaller
│   └── prefs/       AppPrefs
└── ui/
    ├── theme/       JupiterLight/JupiterDark + dynamic color
    ├── components/  StatusCard, InfoRow
    ├── screens/     Home, Patch, Entries, Config, Files, Logs
    ├── JupiterViewModel.kt
    └── JupiterRoot.kt (NavigationBar)
```

---

## Bundled binaries

Located at `app/src/main/assets/bin/arm64-v8a/` and described in
`assets/binaries.json`. The hashes are verified by the app before installation.

| Binary | Size | SHA-256 |
|--------|-----:|---------|
| `jbm_init` | 892968 | `49ec4df0f2e9ef8d0fc5d7907193d51ea4c1e45affba7f11829b6f779502809a` |
| `magiskboot` | 788840 | `d7440e2cd89899426e809554bf793baef9804ccbe5a52ce34a8b6242725d3c77` |

- `jbm_init` was compiled from `src/jbm_menu.c` + `src/start.S` of the JupiterBoot
  repository with `clang --target=aarch64-linux-gnu -ffreestanding -nostdlib
  -static -fno-builtin -fno-stack-protector -fuse-ld=lld -Os`, then stripped with
  the NDK's `llvm-strip`.
- `magiskboot` is a copy of `magiskbootarm64` from the JupiterBoot repository.
- Licenses: JupiterBoot (jbm_init) and Magisk GPLv3 (magiskboot).

---

## Build

```bash
# Requirements: JDK 21, Android SDK (platform 36), bundled Gradle Wrapper
export JAVA_HOME=/path/to/jdk-21
./gradlew :app:assembleDebug     # debug APK
./gradlew :app:testDebugUnitTest # unit tests (JVM)
```

- Gradle 9.3.0 · AGP 9.0.1 (built-in Kotlin) · Kotlin Compose plugin 2.2.10
- Compose BOM 2025.05.01 · Material 3 1.3.2
- `compileSdk 36` · `targetSdk 35` · `minSdk 26`

The resulting APK is at `app/build/outputs/apk/debug/app-debug.apk`.

---

## Unit tests

JVM tests covering:

- `Versions` — segment-wise numeric comparison (1.10 > 1.9), pre-releases.
- `Hashes` / `Elf` — known SHA-256, ELF64 AArch64/x86_64 detection.
- `Sh` — POSIX quoting, `'` escaping, rejection of relative paths/`..`/metacharacters.
- `ConfigIni` — parser, serializer (in-place + append), validator, JBM values.
- `EntriesIni` — parser, model, per-button overrides, unknown-field preservation,
  validation.
- `ThemeIni` / `MenuModel` — `extends` resolution, config mask precedence,
  effective per-entry styles and row geometry.
- `PatchEngine` — target-selection policy with fake executors.
- `GitHubReleaseClient` — release JSON parsing.

---

## Security

- All privileged commands are built as `argv` and turned into a single
  POSIX single-quoted string (`Sh.join`), preventing injection.
- System paths are validated (`Sh.validateSystemPath`: absolute, no `..`,
  conservative character set).
- Binary file transfer device ↔ app via **base64** (read) and **stdin** (atomic
  write with `tmp` + `mv`).
- The app **never executes** files downloaded from the internet. Updates are
  downloaded only from HTTPS GitHub hosts, checked (ELF + optional SHA-256) and
  installed as a stored file; boot patching remains an explicit separate step.
- **Never** writes to physical partitions automatically.

---

## Known limitations

- Requires root; without it, only the local test root is readable.
- Patch support limited to architectures with recognized `boot`/`init_boot`;
  standalone `vendor_boot` is not supported.
- `arm64-v8a` only (AArch64 binaries).
- Final flashing of the image to the partition is the user's responsibility
  (fastboot/recovery) — the app builds and verifies the image but does not apply
  it.

---

## License

The app code follows the JupiterBoot project's license. The bundled binaries keep
their original licenses (JupiterBoot / Magisk GPLv3).

---