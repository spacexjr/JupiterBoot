# JupiterBoot

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

## Features

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

## Boot Flow

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

## Menu Actions

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

## Tested on Real Hardware

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

## Safety Model

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

## Restoring the Stock Boot Image

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

## Build Requirements

- `clang`
- AArch64 target support (`--target=aarch64-linux-gnu`)
- `lld`
- `nm`
- `file`
- `stat`
- `magiskboot`
- Python 3
- Pillow (only required when regenerating the font)

The original `boot.img` must be available in the repository root.

---

## Building

```sh
./build.sh
```

Or specify another boot image:

```sh
./build.sh path/to/boot.img
```

The build process:

1. Builds the host UI preview.
2. Compiles the freestanding AArch64 JupiterBoot binary.
3. Verifies that the binary has no undefined symbols.
4. Unpacks the original `boot.img`.
5. Renames the original `/init` to `/init.system`.
6. Installs JupiterBoot as `/init`.
7. Repacks the boot image.
8. Verifies the resulting ramdisk and generated `/init`.
9. Verifies that the final image size is unchanged.

The build process **does not flash the device**.

---

## Build Output

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

## UI Preview

The build system generates previews using the same drawing code used by the
JupiterBoot framebuffer interface. This allows the menu to be tested on the
host before deploying it to the device.

---

## Configuration

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

```ini
theme = default
color_bg = #020202
color_accent = #C87335
button_width = 430
button_height = 94
button_spacing = 12
button_radius = 7
text_size = 19
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
keys fall back to the theme file, then to built-in defaults.

**`entries.ini`** — one `[id]` section per menu entry:

```ini
[system]
name = System
type = system          ; system | recovery | fastboot | download | reboot | poweroff | script | binary
order = 0
enabled = 1
is_default = 1
behavior_after = 0   ; 0 = return to menu, 1 = chain to real init, 2 = reboot

[custom]
name = Meu Script
type = script
path = /data/local/tmp/test.sh
args = --verbose run
order = 1
enabled = 1
default = 1
behavior_after = 0
```

`script`/`binary` entries run with `fork()`+`execve()` (no shell, no flashing,
no kexec). They must use absolute paths without `..`; the menu `chmod +x` them
before running. After they finish the menu returns unless `behavior_after` says
otherwise.

**`themes/<name>.ini`** — same colour/metric keys as `config.ini`, applied first
and then overridden by matching `config.ini` keys.

### Logging

Diagnostics go through `/dev/kmsg` and, when `log_file = 1`, are appended to
`<config root>/boot.log` (best effort). On the device, read them after boot
with `adb shell cat /cache/Jupiter/boot.log` (or `/Jupiter/boot.log`).

---

## Regenerating the Font

```sh
python3 tools/gen_font.py
```

This requires Pillow and regenerates:

```text
src/jbm_font.h
```

---

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
├── boot.img
└── out/
```

---

## Device

| Property | Value |
|---|---|
| Device | Samsung Galaxy A22 4G |
| Model | SM-A225M |
| Codename | `a22` |
| SoC | MediaTek MT6768 |
| Architecture | AArch64 |
| Android target | Android 15 GSI |

---

## Current Status

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

## Disclaimer

This project is intended for development and experimentation.

Modifying and flashing boot images can result in a device that does not boot.
Always keep a known-good stock boot image and a reliable recovery method
available before testing.

---

## Thank You

Thanks to everyone testing and contributing to JupiterBoot.

**JupiterBoot · Boot Menu Project**
