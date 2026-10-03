# JupiterBoot

A graphical, touch-driven boot menu injected into the `boot` ramdisk for the Samsung Galaxy A22 4G (`SM-A225M` / `a22`).

JupiterBoot replaces the Android `/init` entry point with a freestanding AArch64 boot menu. The original Android first-stage init is preserved as `/init.system` and can be executed from the menu to continue the normal Android boot process.

> **⚠️ Warning:** JupiterBoot is experimental boot-chain software. Flashing a modified `boot.img` can prevent your device from booting if built incorrectly. Always keep a known-good stock `boot.img` and a reliable recovery/unbrick method (such as Odin) before testing.

> **Note:** This repository does **not** flash your device. It only produces `out/boot-jbm.img`, which must be reviewed and flashed manually.

## Features

- **Graphical framebuffer UI** – Clean, minimal boot menu rendered directly on the framebuffer
- **Touchscreen navigation** – Tap to select actions with configurable hold durations
- **Automatic boot countdown** – Falls back to System boot if left unattended
- **Boot mode selection** – System, Recovery, Fastboot, Download, Reboot, and Power Off
- **Safe fallback** – If the framebuffer cannot be initialized, JupiterBoot chains directly to `/init.system` without showing the UI
- **Host UI preview** – Preview the menu on your host using the same drawing code
- **Non-destructive by design** – No partition modification, flashing, or backups performed at runtime

## Boot Flow

1. Bootloader loads `boot.img`
2. Kernel starts `/init` (replaced by JupiterBoot)
3. JupiterBoot initializes `/proc`, `/sys`, `/dev/fb0`, and touchscreen input
4. Graphical boot menu is displayed
5. Automatic boot countdown starts (cancellable by any touch input)
6. User selects an action or allows countdown to expire
7. If **System** is selected, JupiterBoot executes `/init.system`
8. `/init.system` continues normal Android boot

## Menu Actions

| Action | Hold Time | Behavior |
|---|---|---|
| **System** | Tap | Boots the installed Android OS via `/init.system` |
| **Recovery** | 900 ms | Reboots with the `recovery` reboot reason |
| **Fastboot** | 700 ms | Requests reboot to bootloader (Fastboot mode) |
| **Download** | 900 ms | Reboots with the `download` reboot reason (ODIN/Download Mode) |
| **Reboot** | 700 ms | Restarts the device |
| **Power Off** | 900 ms | Powers off the device |

Any touchscreen interaction cancels the automatic boot countdown.

## Compatibility & Testing

**Tested Device:** Samsung Galaxy A22 4G — `SM-A225M` (`a22`)

| Action | Status |
|---|---|
| System | ✅ Working |
| Recovery | ✅ Working |
| Download (ODIN) | ✅ Working |
| Reboot | ✅ Working |
| Power Off | ✅ Working |
| Fastboot | ⏳ Untested (may vary by bootloader/SoC behavior) |

## Safety & Security

JupiterBoot is **read-only at runtime**. It does not perform partition flashing, copying, overwriting, or modification while running. The `boot` partition remains unmodified during menu operation.

- **Recovery:** Performs `sync()` and calls `reboot(LINUX_REBOOT_CMD_RESTART2, "recovery")`. Whether the bootloader honors this reason is device/platform-dependent.
- **Download:** Requests reboot with the `download` reason. No userspace flashing is performed.
- **No runtime backups:** The legacy `/cache/jbmenu/boot.bak` backup mechanism has been removed in the current version. See [Restoring Stock](#restoring-the-stock-boot-image) if you need to recover.

## Restoring the Stock Boot Image

If you need to restore your original boot image:

- **If you have an old backup** (from a previous JupiterBoot build): From root shell or recovery, run:

  ```sh
  dd if=/cache/jbmenu/boot.bak of=/dev/block/by-name/boot
  sync
  reboot
  ```

- **If no backup is available:** Restore the stock `boot.img` using Samsung's official flashing method (Odin). This is the recommended and safest approach.

## Build Requirements

- `clang` (with AArch64 target support, e.g. `--target=aarch64-linux-gnu`)
- `lld` (linker)
- `nm`
- `file`
- `stat`
- [`magiskboot`](https://github.com/topjohnwu/Magisk/releases) (available in PATH)
- Python 3
- [Pillow](https://pillow.readthedocs.io/) (only required when regenerating the font: `tools/gen_font.py`)

The original stock `boot.img` for `SM-A225M` must be placed in the repository root before building.

## Building

Build with the stock boot image in the repo root:

```sh
./build.sh
```

Or specify a different boot image path:

```sh
./build.sh path/to/boot.img
```

### Build Steps

1. Builds the host UI preview (`ui-preview.ppm`, `ui-preview-intro.ppm`)
2. Compiles the freestanding AArch64 JupiterBoot binary (`jbm_init`)
3. Verifies the binary has no undefined symbols
4. Unpacks the original `boot.img`
5. Renames the original `/init` to `/init.system`
6. Installs JupiterBoot as `/init`
7. Repacks the modified boot image
8. Verifies the resulting ramdisk and generated `/init`
9. Verifies the final image size matches the original (size-preserving)

The build process **does not flash the device**.

## Flashing

> **Critical:** Flashing a modified boot image carries risk. Double-check your build output, ensure you have a stock backup/Odin ready, and proceed at your own risk.

1. Build successfully and verify `out/boot-jbm.img` exists
2. Copy `out/boot-jbm.img` to your PC
3. Flash using [Odin](https://samfw.com/odin) (or your preferred Samsung flashing tool) to the `BOOT` partition only
4. Reboot and test the menu

## Build Output

```text
out/
├── jbm_init
├── boot-jbm.img
├── ui-preview.ppm
└── ui-preview-intro.ppm
```

- `boot-jbm.img` – Modified boot image to flash manually
- `jbm_init` – Compiled freestanding AArch64 binary
- `ui-preview*.ppm` – Host-side UI previews rendered with the same framebuffer code

## Configuration

Build-time constants can be overridden via environment variables:

| Variable | Default | Description |
|---|---|---|
| `JBM_AUTOBOOT_SEC` | As defined in build | Automatic boot timeout in seconds |
| `JBM_VERSION` | As defined in build | Version string embedded in build |

Example:

```sh
JBM_AUTOBOOT_SEC=10 JBM_VERSION=0.2.1 ./build.sh
```

## Regenerating the Font

The bitmap font is generated from a source bitmap via `tools/gen_font.py`. Regenerate only if modifying font assets:

```sh
python3 tools/gen_font.py
```

This regenerates `src/jbm_font.h`. Pillow is required.

## Project Structure

```text
JupiterBoot/
├── src/
│   ├── jbm_menu.c
│   ├── start.S
│   └── jbm_font.h
├── tools/
│   └── gen_font.py
├── build.sh
├── boot.img        # Stock boot image (must be provided)
└── out/            # Build artifacts
```

## Device Info

| Property | Value |
|---|---|
| Device | Samsung Galaxy A22 4G |
| Model | SM-A225M |
| Codename | `a22` |
| SoC | MediaTek MT6768 |
| Architecture | AArch64 |
| Android Target | Android 15 GSI |

## Current Status

**JupiterBoot 0.3 — Experimental**

The graphical boot menu runs successfully on real SM-A225M hardware. System, Recovery, Download, Reboot, and Power Off are confirmed working on-device. **Fastboot** remains untested. 

## Disclaimer

This project is intended for development, testing, and experimentation only. Modifying and flashing boot images can result in a device that fails to boot. **Always** keep a known-good stock boot image and a reliable recovery method available before testing. Use at your own risk.

## Acknowledgements

Thanks to everyone testing, reporting issues, and contributing to JupiterBoot.

**JupiterBoot · Graphical Boot Menu for SM-A225M**
