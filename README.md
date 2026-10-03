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

### Not tested yet

- ⏳ Fastboot

---

## Safety Model

JupiterBoot does not perform partition flashing or modification during normal
menu operation.

### Recovery
It synchronizes pending writes
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

It requests a reboot with the
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
|   
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

**JupiterBoot 0.2 — Experimental**

The graphical boot menu is successfully running on real SM-A225M hardware.

System, Recovery, Download, Reboot, and Power Off have been confirmed on the
device. Fastboot remains untested, and the graphical System starting screen is
still under development.

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
