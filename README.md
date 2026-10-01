# JupiterBoot

A graphical, touch-driven boot menu injected into the `boot` ramdisk of a
Samsung Galaxy A22 4G (`a22` / SM-A225M, MediaTek MT6768, Android 15 GSI).

`/init` is replaced by a freestanding aarch64 binary; the original Android
first-stage init is preserved as `/init.system` and is what the menu eventually
executes. The repository never flashes anything: the build only produces
`out/boot-jbm.img`, which must be reviewed and flashed manually.

## Boot flow

1. The bootloader loads `boot.img` and the kernel starts `/init` (the menu).
2. The menu mounts `proc`/`sys`, opens `/dev/fb0` and the touchscreen, and draws
   the UI.
3. After the auto-start countdown (default 15 s) it chains to `/init.system`,
   which continues the normal second-stage boot.
4. If the framebuffer is unavailable, the menu chains to `/init.system`
   immediately without drawing anything.

## Menu actions

| Action    | Hold  | Behaviour                                              |
|-----------|-------|--------------------------------------------------------|
| System    | tap   | Boot the installed Android (runs `/init.system`)       |
| Recovery  | 900ms | Reboot with reason `recovery` (no flash write)         |
| Fastboot  | 700ms | Reboot to bootloader                                   |
| Download  | 900ms | Reboot with reason `download`                           |
| Reboot    | 700ms | Restart the device                                     |
| Power off | 900ms | Shut the device down                                   |

Any touch cancels the countdown so the device waits for a decision.

## Safety model

- **Recovery** does not write to flash. It only logs the intent, syncs, and asks
  the kernel to reboot with the reason `recovery`
  (`reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2,
  "recovery")`). Whether the bootloader honours that reason is up to the
  platform and is not guaranteed.
- **Download** likewise only reboots with the reason `download`; there is no
  userspace flash write.
- No menu action backs up, copies, or overwrites any partition. `boot` is left
  untouched by the menu.

## Restoring `boot` (legacy)

Earlier builds could back up `boot` to `/cache/jbmenu/boot.bak` before a
recovery swap. That code has been removed and no backup is created anymore.
If you still have a backup from an old build, it is a raw copy of the whole
32 MiB `boot` partition and can be restored from a root shell or recovery:

```sh
dd if=/cache/jbmenu/boot.bak of=/dev/block/by-name/boot
sync
```

Without a backup, reflash the stock `boot` image with Odin.

## Build

Requirements:

- Linux host with `clang` capable of `--target=aarch64-linux-gnu` (uses `lld`).
- `nm`, `file`, `stat`.
- `magiskboot` next to this README (already present).
- The original `boot.img` in the repository root.
- Python 3 + Pillow only if regenerating the font.

```sh
./build.sh            # uses ./boot.img
./build.sh path/to/boot.img
```

The script:

1. Builds the host UI preview (`out/ui-preview.ppm`, after the intro, and
   `out/ui-preview-intro.ppm`, mid-animation) from the real drawing path.
2. Compiles the freestanding aarch64 binary to `out/jbm_init` and rejects any
   undefined symbol.
3. Unpacks `boot.img`, renames `init` to `init.system`, adds the menu as `init`,
   and repacks to `out/boot-jbm.img`.
4. Re-reads the output and verifies the ramdisk, the extracted `/init`, and that
   the image size is unchanged.

Constants such as the countdown and version can be overridden:

```sh
JBM_AUTOBOOT_SEC=10 JBM_VERSION=1.1 ./build.sh
```

## Regenerating fonts

```sh
python3 tools/gen_font.py     # rewrites src/jbm_font.h, needs Pillow
```

## Layout

```
src/jbm_menu.c   menu, UI, framebuffer, touch and reboot
src/start.S      aarch64 entry point that normalises argc/argv/envp
src/jbm_font.h   generated 8-bit alpha font atlases (committed)
tools/gen_font.py
build.sh         build + inject + verify, never flashes
out/             build artifacts (jbm_init, boot-jbm.img, previews, staging)
```

## Known limitations

- **Download/ODIN**: there is no known userspace trigger for MTK download mode;
  the menu only passes the `download` reason through `reboot` and logs it.
- **AVB**: the image is built for an unlocked device (`verifiedbootstate=orange`,
  `avb_version=0`). It does not update `vbmeta`; do not flash it on a locked
  device.
- The actual Android boot through `/init.system` after the menu chains to it has
  not been validated on hardware from this repository.
