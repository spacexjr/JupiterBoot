#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdint.h>

#include "jbm_font.h"
#include "jbm_planet_boot.h"
#include "jbm_planet_menu.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Configuration system: files live under /cache/Jupiter (mounted at runtime),
 * with /Jupiter inside the ramdisk as a fallback root. JBM_ROOT is set once
 * during startup to whichever root actually holds config/entries files. */
#define JBM_CONFIG_DIR "/cache/Jupiter"
#define JBM_RAMDISK_DIR "/Jupiter"
#define JBM_MAX_ENTRIES 32
#define JBM_MAX_ENTRY_NAME 64
#define JBM_MAX_ENTRY_PATH 256
#define JBM_MAX_ENTRY_ARGS 256
#define JBM_MAX_THEME_NAME 32
#define JBM_MAX_CONFIG_VALUE 256

typedef enum {
    JBM_ENTRY_TYPE_SYSTEM = 0,
    JBM_ENTRY_TYPE_RECOVERY,
    JBM_ENTRY_TYPE_FASTBOOT,
    JBM_ENTRY_TYPE_DOWNLOAD,
    JBM_ENTRY_TYPE_REBOOT,
    JBM_ENTRY_TYPE_POWEROFF,
    JBM_ENTRY_TYPE_SCRIPT,
    JBM_ENTRY_TYPE_BINARY,
} jbm_entry_type_t;

typedef struct {
    char id[32];
    char name[JBM_MAX_ENTRY_NAME];
    char description[96];
    char icon[32];
    jbm_entry_type_t type;
    char path[JBM_MAX_ENTRY_PATH];
    char args[JBM_MAX_ENTRY_ARGS];
    int enabled;
    int order;
    int is_default;
    /* 0: return to the menu after a script/binary finishes (default)
     * 1: chain to the real init    2: reboot */
    int behavior_after;
    /* Per-button visual overrides. -1 means "inherit the theme". */
    int button_style;
    int button_width;
    int button_height;
    int button_radius;
    int button_border_width;
    int text_align;
    int icon_scale;
    int show_icon;
    u32 color_bg;
    u32 color_border;
    u32 color_text;
    u32 color_text_sel;
} jbm_entry_t;

/* Bits for jbm_config_t.mask: keys that were explicitly present in
 * config.ini. Only those override the theme when the layers are merged. */
#define JBM_K_COLOR_BG 0x0001u
#define JBM_K_COLOR_FG 0x0002u
#define JBM_K_COLOR_BORDER 0x0004u
#define JBM_K_COLOR_HL 0x0008u
#define JBM_K_COLOR_TEXT 0x0010u
#define JBM_K_COLOR_TEXTSEL 0x0020u
#define JBM_K_COLOR_ACCENT 0x0040u
#define JBM_K_BTN_W 0x0080u
#define JBM_K_BTN_H 0x0100u
#define JBM_K_BTN_GAP 0x0200u
#define JBM_K_BTN_R 0x0400u
#define JBM_K_TEXT_SIZE 0x0800u
#define JBM_K_COLOR_SEL_BG 0x1000u
#define JBM_K_COLOR_PRESSED 0x2000u
#define JBM_K_COLOR_DISABLED 0x4000u
#define JBM_K_BTN_BORDER 0x8000u
#define JBM_K_BTN_ALPHA 0x10000u
#define JBM_K_LAYOUT_MARGIN 0x20000u
#define JBM_K_ICON_SCALE 0x40000u
#define JBM_K_BTN_STYLE 0x80000u
#define JBM_K_TEXT_ALIGN 0x100000u
#define JBM_K_SHOW_TITLE 0x200000u
#define JBM_K_SHOW_DESC 0x400000u
#define JBM_K_SHOW_ICONS 0x800000u

typedef struct {
    u32 mask; /* JBM_K_* bits set by keys found in config.ini */
    int config_version;
    char background_path[JBM_MAX_CONFIG_VALUE];
    char theme_name[JBM_MAX_THEME_NAME];
    char title[64];
    char subtitle[128];
    int button_style;
    u32 color_bg;
    u32 color_fg;
    u32 color_border;
    u32 color_highlight;
    u32 color_selected_bg;
    u32 color_pressed_bg;
    u32 color_disabled_text;
    u32 color_text;
    u32 color_text_sel;
    u32 color_accent;
    int button_width;
    int button_height;
    int button_spacing;
    int button_radius;
    int button_border_width;
    int button_alpha; /* 0..255, background fill opacity */
    int text_size;
    int text_align; /* 0 left, 1 center, 2 right */
    int layout_margin; /* percent of screen width, 0..40 */
    int show_title;
    int show_description;
    int show_icons;
    int icon_scale; /* percent, 50..300 */
    int menu_timeout_sec;
    int default_entry; /* index into the entry list, -1 = use entry flag */
    int show_countdown;
    int show_brand;
    int orientation; /* 0/90/180/270, -1 = auto (fb var.rotate * 90) */
    char input_device[JBM_MAX_CONFIG_VALUE];
    int touch_cal_x0, touch_cal_y0, touch_cal_x1, touch_cal_y1;
    int prefer_drm;
    int fallback_to_fb;
    int log_level;
    int log_file;
} jbm_config_t;

typedef struct {
    char name[JBM_MAX_THEME_NAME];
    char extends[JBM_MAX_THEME_NAME];
    int button_style;
    u32 color_bg;
    u32 color_fg;
    u32 color_border;
    u32 color_highlight;
    u32 color_selected_bg;
    u32 color_pressed_bg;
    u32 color_disabled_text;
    u32 color_text;
    u32 color_text_sel;
    u32 color_accent;
    int button_width;
    int button_height;
    int button_spacing;
    int button_radius;
    int button_border_width;
    int button_alpha;
    int text_size;
    int text_align;
    int layout_margin;
    int show_title;
    int show_description;
    int show_icons;
    int icon_scale;
} jbm_theme_t;

static jbm_config_t JBM_CFG;
static jbm_theme_t JBM_FX; /* theme merged with config overrides: what UI paints with */
static jbm_entry_t JBM_ENTRIES[JBM_MAX_ENTRIES];
static int JBM_NUM_ENTRIES = 0;
static char JBM_ROOT[64] = JBM_CONFIG_DIR;
static int JBM_CACHE_MOUNTED = 0; /* we mounted /cache and must unmount it again */
#ifndef JBM_HOST
static int JBM_CACHE_READY = 0;   /* /cache (or the ramdisk root) is usable for config */
#endif
static int JBM_CFG_WRITABLE = 0;  /* may create <JBM_ROOT>/... on this boot */

#define AT_FDCWD (-100)

struct jbm_timespec {
    i64 tv_sec;
    long tv_nsec;
};

#ifndef JBM_REAL_INIT
#define JBM_REAL_INIT "/init.system"
#endif
#ifndef JBM_AUTOBOOT_SEC
#define JBM_AUTOBOOT_SEC 15
#endif
#ifndef JBM_VERSION
#define JBM_VERSION "1.1"
#endif
#ifndef JBM_TOTAL_TIMEOUT_SEC
#define JBM_TOTAL_TIMEOUT_SEC 240
#endif

/* Current on-disk configuration format. config.ini can carry a
 * `config_version` key; older files are accepted and migrated in memory. */
#define JBM_CONFIG_VERSION 2

/* Duration of the entry slide-in animation, in ms. */
#define JBM_INTRO_MS 480

/* How long the "Booting System" screen stays before chaining to the real
 * init. The frame is left in the framebuffer while Android boots. */
#define JBM_BOOT_MS 2000

/* How long the action screen stays up before a non-System action actually runs
 * (reboot to recovery/bootloader, restart, power off, ...). Without this the
 * reboot syscall fires in the same frame as the tap and nothing is seen. */
#define JBM_ACTION_MS 1500

/* errno of the last failed jbm_mmap(), 0 otherwise (diagnostics only). */
static int jbm_mmap_err = 0;

#define LINUX_REBOOT_MAGIC1 0xfee1dead
#define LINUX_REBOOT_MAGIC2 0x28121969
#define LINUX_REBOOT_CMD_RESTART 0x01234567
#define LINUX_REBOOT_CMD_POWER_OFF 0x4321FEDC
#define LINUX_REBOOT_CMD_RESTART2 0xA1B2C3D4

#define JB_RB_RESTART 0
#define JB_RB_BOOTLOADER 1
#define JB_RB_POWER_OFF 2
#define JB_RB_DOWNLOAD 3
#define JB_RB_RECOVERY 4

/* ------------------------------------------------------------------ */
/* syscalls                                                            */
/* ------------------------------------------------------------------ */

#ifdef JBM_HOST

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>

static i64 hs_read(int fd, void *b, u32 n) { return (i64)read(fd, b, n); }
static i64 hs_write(int fd, const void *b, u32 n) { return (i64)write(fd, b, n); }
static i64 hs_open(const char *p, int fl, int md) { return open(p, fl, md); }
static int hs_close(int fd) { return close(fd); }
static int hs_ioctl(int fd, unsigned long r, void *a) { return ioctl(fd, r, a); }
static void *hs_mmap(u32 len, int prot, int flags, int fd, long off) {
    (void)prot;
    if (flags & MAP_ANONYMOUS) return calloc(1, len ? len : 1);
    (void)fd;
    (void)off;
    return calloc(1, len ? len : 1);
}
static i64 hs_poll(void *fds, u32 n, int t) { return (i64)poll((void *)fds, (nfds_t)n, t); }
static i64 hs_getdents(int fd, void *b, u32 n) { return (i64)syscall(SYS_getdents64, fd, b, n); }
static i64 hs_mkdir(const char *p, int m) { return (i64)mkdir(p, (mode_t)m); }
static i64 hs_mknod(const char *p, int m, int d) { return (i64)mknod(p, (mode_t)m, (dev_t)d); }
static int hs_mount(const char *s, const char *t, const char *ty, u64 fl, const void *d) {
    return mount(s, t, ty, fl, d) == 0 ? 0 : -1;
}
static int hs_umount(const char *t) { return umount(t) == 0 ? 0 : -1; }
static int hs_fsync(int fd) { return fsync(fd); }
static i64 hs_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (i64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static void hs_sleep_ms(u32 ms) { usleep(ms * 1000); }
static int hs_fchmod(const char *p, int m) { return chmod(p, (mode_t)m); }
static int hs_rename(const char *a, const char *b) { return rename(a, b); }
static int jbm_atoi_(const char *s) { return atoi(s); }
static i64 hs_lseek(int fd, i64 off, int whence) { return (i64)lseek(fd, (off_t)off, whence); }
static void hs_release(void *p, u32 len) {
    (void)len;
    free(p);
}
static i64 hs_fork(void) {
    pid_t p = fork();
    return (i64)p;
}
static int hs_wait(i64 pid, int *status) {
    int st = 0;
    pid_t r = (pid_t)waitpid((pid_t)pid, &st, 0);
    if (status) *status = st;
    return r < 0 ? -1 : (st >> 8) & 0xFF;
}
static i64 hs_reboot_cmd(int cmd, int sub) {
    static const char *const names[] = {"restart", "bootloader", "power off", "download", "recovery"};
    if (cmd >= 0 && cmd < 5) fprintf(stderr, "[jbm:host] would reboot: %s\n", names[cmd]);
    else fprintf(stderr, "[jbm:host] would reboot: unknown (%d)\n", cmd);
    (void)sub;
    return 0;
}

#define jbm_open hs_open
#define jbm_read hs_read
#define jbm_write hs_write
#define jbm_close hs_close
#define jbm_ioctl hs_ioctl
#define jbm_mmap hs_mmap
#define jbm_pollfd_ts hs_poll
#define jbm_getdents hs_getdents
#define jbm_mkdir hs_mkdir
#define jbm_mknod hs_mknod
#define jbm_mount hs_mount
#define jbm_umount hs_umount
#define jbm_fsync hs_fsync
#define jbm_now_ms hs_now_ms
#define jbm_sleep_ms hs_sleep_ms
#define jbm_reboot_cmd hs_reboot_cmd
#define jbm_fchmod hs_fchmod
#define jbm_rename hs_rename
#define jbm_fork hs_fork
#define jbm_wait hs_wait
#define jbm_lseek hs_lseek
#define jbm_release hs_release

#else /* freestanding aarch64 */

#define SYS_ioctl 29
#define SYS_renameat 38
#define SYS_mknodat 33
#define SYS_mkdirat 34
#define SYS_umount2 39
#define SYS_mount 40
#define SYS_fchmodat 53
#define SYS_openat 56
#define SYS_close 57
#define SYS_getdents64 61
#define SYS_lseek 62
#define SYS_read 63
#define SYS_write 64
#define SYS_ppoll 73
#define SYS_fsync 82
#define SYS_exit_group 94
#define SYS_nanosleep 101
#define SYS_clock_gettime 113
#define SYS_sched_yield 124
#define SYS_reboot 142
#define SYS_clone 220
#define SYS_munmap 215
#define SYS_execve 221
#define SYS_mmap 222
#define SYS_wait4 260
#define SYS_syncfs 267

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64
#define O_EXCL 128
#define O_TRUNC 512
#define O_APPEND 1024
#define O_NONBLOCK 2048
#define O_DIRECTORY 040000

#define SIGCHLD 17

#define SEEK_SET 0
#define SEEK_END 2

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32

#define POLLIN 0x001

#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_ABS 0x03
#define ABS_X 0x00
#define ABS_Y 0x01
#define ABS_MT_SLOT 0x2F
#define ABS_MT_POSITION_X 0x35
#define ABS_MT_POSITION_Y 0x36
#define ABS_MT_TRACKING_ID 0x39
#define BTN_TOUCH 0x14A

#define FBIOGET_VSCREENINFO 0x4600
#define FBIOPUT_VSCREENINFO 0x4601
#define FBIOBLANK 0x4611
#define FB_ACTIVATE_FORCE 64
#define FB_ACTIVATE_ALL 32
#define FBIOGET_FSCREENINFO 0x4602

#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif

#define _IOC(dir, type, nr, size) (((dir) << 30) | ((size) << 16) | ((type) << 8) | (nr))
#define _IOC_READ 2
#define EVIOCGNAME(len) _IOC(_IOC_READ, 'E', 0x06, len)
#define EVIOCGABS(code) _IOC(_IOC_READ, 'E', 0x40 + (code), sizeof(struct jbm_absinfo))

static long sc6(long n, long a, long b, long c, long d, long e, long f) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    register long x5 __asm__("x5") = f;
    __asm__ volatile("svc 0" : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory", "cc");
    return x0;
}
static long sc5(long n, long a, long b, long c, long d, long e) { return sc6(n, a, b, c, d, e, 0); }
static long sc4(long n, long a, long b, long c, long d) { return sc6(n, a, b, c, d, 0, 0); }
static long sc3(long n, long a, long b, long c) { return sc6(n, a, b, c, 0, 0, 0); }
static long sc2(long n, long a, long b) { return sc6(n, a, b, 0, 0, 0, 0); }
static long sc1(long n, long a) { return sc6(n, a, 0, 0, 0, 0, 0); }

static int jbm_open(const char *p, int fl, int md) { return (int)sc4(SYS_openat, AT_FDCWD, (long)p, fl, md); }
static i64 jbm_read(int fd, void *b, u32 n) { return sc3(SYS_read, fd, (long)b, n); }
static i64 jbm_write(int fd, const void *b, u32 n) { return sc3(SYS_write, fd, (long)b, n); }
static int jbm_close(int fd) { return (int)sc1(SYS_close, fd); }
static int jbm_ioctl(int fd, unsigned long r, void *a) { return (int)sc3(SYS_ioctl, fd, (long)r, (long)a); }
static void *jbm_mmap_raw(u32 len, int prot, int flags, int fd, long off) {
    return (void *)sc6(SYS_mmap, 0, len, prot, flags, fd, off);
}
static void *jbm_mmap(u32 len, int prot, int flags, int fd, long off) {
    void *p = jbm_mmap_raw(len, prot, flags, fd, off);
    long e = (long)p;
    jbm_mmap_err = 0;
    if (e < 0 && e > -4096) {
        jbm_mmap_err = (int)-e; /* kernel returned -errno */
        return 0;
    }
    return p;
}
static i64 jbm_getdents(int fd, void *b, u32 n) { return sc3(SYS_getdents64, fd, (long)b, n); }
static i64 jbm_mkdir(const char *p, int m) { return sc3(SYS_mkdirat, AT_FDCWD, (long)p, m); }
static i64 jbm_mknod(const char *p, int m, int d) { return sc4(SYS_mknodat, AT_FDCWD, (long)p, m, d); }
static int jbm_mount(const char *s, const char *t, const char *ty, u64 fl, const void *d) {
    return (int)sc5(SYS_mount, (long)s, (long)t, (long)ty, fl, (long)d);
}
static int jbm_umount(const char *t) { return (int)sc2(SYS_umount2, (long)t, 0); }
static i64 jbm_pollfd_ts(void *fds, u32 n, int t) {
    /* aarch64 has no poll(2); ppoll(2) takes the timeout as a timespec. */
    struct jbm_timespec ts;
    struct jbm_timespec *tp = 0;
    if (t >= 0) {
        ts.tv_sec = t / 1000;
        ts.tv_nsec = (long)(t % 1000) * 1000000L;
        tp = &ts;
    }
    return sc5(SYS_ppoll, (long)fds, n, (long)tp, 0, 0);
}
static i64 jbm_now_ms(void) {
    struct jbm_timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    sc2(SYS_clock_gettime, 1, (long)&ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static void jbm_sleep_ms(u32 ms) {
    struct jbm_timespec ts;
    ts.tv_sec = (i64)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    sc2(SYS_nanosleep, (long)&ts, 0);
}
static i64 jbm_reboot_cmd(int cmd, int sub) {
    (void)sub;
    switch (cmd) {
        case JB_RB_RESTART:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART, 0);
        case JB_RB_BOOTLOADER:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"bootloader");
        case JB_RB_DOWNLOAD:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"download");
        case JB_RB_RECOVERY:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"recovery");
        default:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_POWER_OFF, 0);
    }
}

/* Best-effort chmod so a freshly pushed script/binary becomes executable. */
static int jbm_fchmod(const char *p, int m) { return (int)sc4(SYS_fchmodat, AT_FDCWD, (long)p, (long)m, 0); }

/* renameat(2): aarch64 has no rename(2); both paths are CWD-relative. Returns 0
 * or a negative errno. Used for atomic temp-file writes. */
static int jbm_rename(const char *a, const char *b) {
    return (int)sc4(SYS_renameat, AT_FDCWD, (long)a, AT_FDCWD, (long)b);
}

/* fork(2) on aarch64 is clone(SIGCHLD, 0, 0, 0, 0). 0 in the child, pid in the
 * parent, negative error otherwise. */
static i64 jbm_fork(void) { return sc5(SYS_clone, SIGCHLD, 0, 0, 0, 0); }

/* Blocking wait4; returns the exit code (0..255) or -1 if the child was
 * signalled / wait failed. */
static int jbm_wait(i64 pid, int *status) {
    int st = 0;
    i64 r = sc4(SYS_wait4, pid, (long)&st, 0, 0);
    if (status) *status = st;
    if (r < 0) return -1;
    if (st & 0x7F) return -1; /* killed by a signal */
    return (st >> 8) & 0xFF;
}

static i64 jbm_lseek(int fd, i64 off, int whence) { return sc3(SYS_lseek, fd, off, whence); }

/* Anonymous mappings (and the fb mapping) are released here. For the fb the
 * caller already tracked the length; anonymous buffers pass their own. */
static void jbm_release(void *p, u32 len) {
    if (p && len) sc2(SYS_munmap, (long)p, len);
}

#endif /* JBM_HOST */

struct jbm_pollfd {
    int fd;
    short events;
    short revents;
};

struct jbm_absinfo {
    i32 value, minimum, maximum, fuzz, flat, resolution;
};

struct jbm_input_event {
    i64 tv_sec;
    i64 tv_usec;
    u16 type;
    u16 code;
    i32 value;
};

struct jbm_dirent64 {
    u64 d_ino;
    i64 d_off;
    u16 d_reclen;
    u8 d_type;
    char d_name[];
};

struct jbm_fb_var {
    /* Field order/size must match the kernel's struct fb_var_screeninfo
     * (160 bytes on 64-bit); offsets are checked by the static asserts below. */
    u32 xres, yres, xres_virtual, yres_virtual;
    u32 xoffset, yoffset, bits_per_pixel, grayscale;
    u32 red_offset, red_length, red_msb_right;
    u32 green_offset, green_length, green_msb_right;
    u32 blue_offset, blue_length, blue_msb_right;
    u32 transp_offset, transp_length, transp_msb_right;
    u32 nonstd, activate, height, width;
    u32 accel_flags, pixclock;
    u32 left_margin, right_margin, upper_margin, lower_margin;
    u32 hsync_len, vsync_len, sync, vmode;
    u32 rotate, colorspace;
    u32 reserved[4];
};

struct jbm_fb_fix {
    char id[16];
    u64 smem_start;
    u32 smem_len;
    u32 type, type_aux, visual;
    u16 xpanstep, ypanstep, ywrapstep;
    u32 line_length;
    u64 mmio_start;
    u32 mmio_len;
    u32 accel;
    u16 capabilities;
    u16 reserved[2];
};

_Static_assert(sizeof(struct jbm_input_event) == 24, "input_event size");
_Static_assert(sizeof(struct jbm_fb_var) == 160, "fb_var size");
_Static_assert(sizeof(struct jbm_fb_fix) == 80, "fb_fix size");
_Static_assert(sizeof(struct jbm_absinfo) == 24, "absinfo size");
_Static_assert(__builtin_offsetof(struct jbm_fb_var, red_offset) == 32, "fb_var red");
_Static_assert(__builtin_offsetof(struct jbm_fb_var, transp_offset) == 68, "fb_var transp");
_Static_assert(__builtin_offsetof(struct jbm_fb_fix, line_length) == 48, "fb_fix line_length");

/* ------------------------------------------------------------------ */
/* libc                                                                */
/* ------------------------------------------------------------------ */

#ifndef JBM_HOST
void *memset(void *d, int c, unsigned long n);
void *memcpy(void *d, const void *s, unsigned long n);
void *memmove(void *d, const void *s, unsigned long n);
int memcmp(const void *a, const void *b, unsigned long n);
unsigned long strlen(const char *s);
int strcmp(const char *a, const char *b);
char *strchr(const char *s, int c);
#endif

void *memset(void *d, int c, unsigned long n) {
    u8 *p = (u8 *)d;
    while (n--) *p++ = (u8)c;
    return d;
}

void *memcpy(void *d, const void *s, unsigned long n) {
    u8 *dp = (u8 *)d;
    const u8 *sp = (const u8 *)s;
    if (!n || dp == sp) return d;
    if (((u64)dp & 7) == 0 && ((u64)sp & 7) == 0) {
        while (n && ((u64)dp & 7)) {
            *dp++ = *sp++;
            n--;
        }
        while (n >= 8) {
            *(u64 *)dp = *(const u64 *)sp;
            dp += 8;
            sp += 8;
            n -= 8;
        }
    }
    while (n--) *dp++ = *sp++;
    return d;
}

void *memmove(void *d, const void *s, unsigned long n) {
    u8 *dp = (u8 *)d;
    const u8 *sp = (const u8 *)s;
    if (dp == sp || !n) return d;
    if (dp < sp) return memcpy(d, s, n);
    dp += n;
    sp += n;
    while (n--) *--dp = *--sp;
    return d;
}

int memcmp(const void *a, const void *b, unsigned long n) {
    const u8 *x = (const u8 *)a, *y = (const u8 *)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++;
        y++;
    }
    return 0;
}

#ifndef JBM_HOST
unsigned long strlen(const char *s) {
    unsigned long n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char *strchr(const char *s, int c) {
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (!*s) return 0;
    }
}
#endif

static int jbm_str_has(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (const char *h = hay; *h; h++) {
        const char *a = h;
        const char *b = needle;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (!*b) return 1;
    }
    return 0;
}

static void jbm_strcpy_(char *d, const char *s, u32 cap) {
    u32 i = 0;
    if (!cap) return;
    while (s[i] && i < cap - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

#ifndef JBM_HOST
static int jbm_atoi_(const char *s) {
    int v = 0, neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}
#endif

static i32 jbm_isqrt(i32 v) {
    if (v <= 0) return 0;
    i32 r = v, x = (v / 2) + 1;
    while (x < r) {
        r = x;
        x = (x + v / x) / 2;
    }
    return r;
}

static char *jbm_fmtd(char *o, i64 v, u32 pad, char fill) {
    char tmp[24];
    int n = 0;
    u64 u = v < 0 ? (u64)(-v) : (u64)v;
    if (!u) tmp[n++] = '0';
    while (u) {
        tmp[n++] = (char)('0' + (u % 10));
        u /= 10;
    }
    while ((u32)n < pad) tmp[n++] = fill;
    if (v < 0) tmp[n++] = '-';
    for (int i = n - 1; i >= 0; i--) *o++ = tmp[i];
    *o = 0;
    return o;
}

static int jbm_vfmt(char *out, const char *fmt, va_list ap) {
    char *o = out;
    while (*fmt) {
        if (*fmt != '%') {
            *o++ = *fmt++;
            continue;
        }
        fmt++;
        char padc = ' ';
        u32 pad = 0;
        if (*fmt == '0') {
            padc = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            pad = pad * 10 + (u32)(*fmt++ - '0');
        }
        int lng = 0; /* 'l'/'ll' asks for a 64-bit argument */
        while (*fmt == 'l' || *fmt == 'z' || *fmt == 'j' || *fmt == 'h') {
            if (*fmt == 'l') lng++;
            fmt++;
        }
        switch (*fmt++) {
            case 'x':
            case 'X': {
                unsigned long long x = lng ? va_arg(ap, unsigned long long)
                                           : (unsigned long long)va_arg(ap, unsigned int);
                char t[24];
                int m = 0;
                if (!x) t[m++] = '0';
                const char *hex = (fmt[-1] == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                while (x) {
                    t[m++] = hex[x & 0xF];
                    x >>= 4;
                }
                for (u32 k = (u32)m; k < pad; k++) *o++ = padc;
                for (int i = m - 1; i >= 0; i--) *o++ = t[i];
                break;
            }
            case 'p': {
                unsigned long pxv = (unsigned long)va_arg(ap, void *);
                char t[32];
                int m = 0;
                if (!pxv) t[m++] = '0';
                const char *hex = "0123456789abcdef";
                while (pxv) {
                    t[m++] = hex[pxv & 0xF];
                    pxv >>= 4;
                }
                for (u32 k = (u32)m; k < pad; k++) *o++ = padc;
                for (int i = m - 1; i >= 0; i--) *o++ = t[i];
                break;
            }
            case 'd':
            case 'i': {
                i64 dv = lng ? va_arg(ap, i64) : (i64)va_arg(ap, int);
                o = jbm_fmtd(o, dv, pad, padc);
                break;
            }
            case 'u': {
                i64 uv = lng ? (i64)va_arg(ap, unsigned long long)
                             : (i64)va_arg(ap, unsigned int);
                o = jbm_fmtd(o, uv, pad, padc);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s) *o++ = *s++;
                break;
            }
            case 'c':
                *o++ = (char)va_arg(ap, int);
                break;
            case '%':
                *o++ = '%';
                break;
            default:
                *o++ = '?';
                break;
        }
    }
    *o = 0;
    return (int)(o - out);
}

static int jbm_sn(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = jbm_vfmt(out, fmt, ap);
    va_end(ap);
    return n;
}

static int jbm_klog_fd = -1;
static int jbm_kmsg_err = 0;
static int jbm_con_err = 0;
/* Optional second sink: <config root>/boot.log, opened once a config root is
 * known. Best effort: writing fails silently when /cache is not writable. */
static int jbm_log_fd = -1;

static void jbm_log(const char *fmt, ...) {
    char b[512];
    char l[600];
    va_list ap;
    va_start(ap, fmt);
    jbm_vfmt(b, fmt, ap);
    va_end(ap);
    if (jbm_klog_fd < 0) {
        /* Prefer /dev/kmsg: those lines land in the kernel log, so they show
         * up in dmesg and survive in pstore console-ramoops after reboot.
         * /dev/console alone is a VT write and is lost. */
        jbm_klog_fd = jbm_open("/dev/kmsg", O_WRONLY, 0);
        if (jbm_klog_fd >= 0) {
            jbm_kmsg_err = 0;
        } else {
            jbm_kmsg_err = -jbm_klog_fd;
            jbm_klog_fd = jbm_open("/dev/console", O_WRONLY, 0);
            jbm_con_err = jbm_klog_fd < 0 ? -jbm_klog_fd : 0;
        }
    }
    int n = 0;
    l[n++] = '<';
    l[n++] = '6';
    l[n++] = '>';
    for (int i = 0; b[i] && n < (int)sizeof(l) - 40; i++) l[n++] = b[i];
    if (jbm_klog_fd < 0 && jbm_kmsg_err) {
        n += jbm_sn(l + n, " [kmsg_err=%d con_err=%d]", jbm_kmsg_err, jbm_con_err);
    }
    l[n++] = '\n';
    if (jbm_klog_fd >= 0)
        jbm_write(jbm_klog_fd, l, (u32)n);
    else
        jbm_write(1, l, (u32)n); /* stdout only when kmsg is unavailable */
    if (jbm_log_fd >= 0) {
        jbm_write(jbm_log_fd, b, strlen(b));
        jbm_write(jbm_log_fd, "\n", 1);
    }
}

#ifndef JBM_HOST
static long jbm_read_file(const char *path, char *buf, u32 cap) {
    int fd = jbm_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    long n = jbm_read(fd, buf, cap - 1);
    jbm_close(fd);
    if (n < 0) return -1;
    if (n > (long)cap - 1) n = (long)cap - 1;
    buf[n] = 0;
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) buf[--n] = 0;
    return n;
}
#endif

static void jbm_sync_all(void) {
    int fd = jbm_open("/", O_RDONLY, 0);
    if (fd >= 0) {
#ifdef JBM_HOST
        jbm_fsync(fd);
#else
        sc1(SYS_syncfs, fd);
#endif
        jbm_close(fd);
    }
}

/* ------------------------------------------------------------------ */
/* INI configuration parser                                           */
/* ------------------------------------------------------------------ */

static void jbm_trim(char *s) {
    char *start = s;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
    char *end = s + strlen(s) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) *end-- = 0;
    if (start != s) memmove(s, start, strlen(start) + 1);
}

/* "#RRGGBB", "0xRRGGBB" or "RRGGBB"; returns opaque ARGB, or 0 when invalid
 * (a parsed colour always carries an alpha of 0xFF, so it is never 0). */
static u32 jbm_parse_color(const char *s) {
    if (!s || !*s) return 0;
    jbm_trim((char *)s);
    if (s[0] == '#') s++;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    unsigned int val = 0;
    int n = 0;
    for (int i = 0; s[i] && i < 8; i++) {
        char c = s[i];
        unsigned int v = 0;
        if (c >= '0' && c <= '9') v = (unsigned int)(c - '0');
        else if (c >= 'a' && c <= 'f') v = (unsigned int)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v = (unsigned int)(c - 'A' + 10);
        else break;
        val = (val << 4) | v;
        n++;
    }
    if (n == 0) return 0;
    if (n <= 6) val |= 0xFF000000u;
    return (u32)val;
}

static int jbm_parse_bool(const char *s) {
    if (!s) return 0;
    jbm_trim((char *)s);
    return (s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y' ||
            s[0] == 'o' || s[0] == 'O');
}

static int jbm_parse_int(const char *s, int def) {
    if (!s || !*s) return def;
    jbm_trim((char *)s);
    return jbm_atoi_(s);
}

/* Portable whole-file slurp. Returns 0/-1. */
static int jbm_read_text(const char *path, char *buf, u32 cap) {
    int fd = jbm_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    long n = jbm_read(fd, buf, cap - 1);
    jbm_close(fd);
    if (n < 0) return -1;
    if (n > (long)cap - 1) n = (long)cap - 1;
    buf[n] = 0;
    return 0;
}

static int jbm_exists(const char *path) {
    int fd = jbm_open(path, O_RDONLY, 0);
    if (fd < 0) return 0;
    jbm_close(fd);
    return 1;
}

static void jbm_set_default_config(jbm_config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->config_version = JBM_CONFIG_VERSION;
    jbm_strcpy_(cfg->theme_name, "default", sizeof(cfg->theme_name));
    jbm_strcpy_(cfg->title, "Jupiter", sizeof(cfg->title));
    jbm_strcpy_(cfg->subtitle, "Boot", sizeof(cfg->subtitle));
    cfg->mask = 0;
    cfg->color_bg = 0xFF020202;
    cfg->color_fg = 0xFF20242C;
    cfg->color_border = 0xFFD88A4E;
    cfg->color_highlight = 0xFFB86837;
    cfg->color_selected_bg = 0xFFB86837;
    cfg->color_pressed_bg = 0xFFD88A4E;
    cfg->color_disabled_text = 0xFF525862;
    cfg->color_text = 0xFF858585;
    cfg->color_text_sel = 0xFFFFFFFF;
    cfg->color_accent = 0xFFC87335;
    cfg->button_style = 0;
    cfg->button_width = 430;
    cfg->button_height = 94;
    cfg->button_spacing = 12;
    cfg->button_radius = 7;
    cfg->button_border_width = 1;
    cfg->button_alpha = 255;
    cfg->text_size = 19;
    cfg->text_align = 0;
    cfg->layout_margin = 7;
    cfg->show_title = 1;
    /* Text-only by default: names only, no descriptions or glyphs. */
    cfg->show_description = 0;
    cfg->show_icons = 0;
    cfg->icon_scale = 100;
    cfg->menu_timeout_sec = JBM_AUTOBOOT_SEC;
    cfg->default_entry = -1;
    cfg->show_countdown = 1;
    cfg->show_brand = 1;
    cfg->orientation = 0;
    cfg->input_device[0] = 0;
    cfg->touch_cal_x0 = cfg->touch_cal_y0 = cfg->touch_cal_x1 = cfg->touch_cal_y1 = -1;
    cfg->prefer_drm = 0;
    cfg->fallback_to_fb = 1;
    cfg->log_level = 6;
    cfg->log_file = 1;
    /* background_path is filled in after the config root is known. */
}

static void jbm_set_default_theme(jbm_theme_t *theme) {
    memset(theme, 0, sizeof(*theme));
    jbm_strcpy_(theme->name, "default", sizeof(theme->name));
    theme->button_style = 0;
    theme->color_bg = 0xFF020202;
    theme->color_fg = 0xFF20242C;
    theme->color_border = 0xFFD88A4E;
    theme->color_highlight = 0xFFB86837;
    theme->color_selected_bg = 0xFFB86837;
    theme->color_pressed_bg = 0xFFD88A4E;
    theme->color_disabled_text = 0xFF525862;
    theme->color_text = 0xFF858585;
    theme->color_text_sel = 0xFFFFFFFF;
    theme->color_accent = 0xFFC87335;
    theme->button_width = 430;
    theme->button_height = 94;
    theme->button_spacing = 12;
    theme->button_radius = 7;
    theme->button_border_width = 1;
    theme->button_alpha = 255;
    theme->text_size = 19;
    theme->text_align = 0;
    theme->layout_margin = 7;
    theme->show_title = 1;
    theme->show_description = 0;
    theme->show_icons = 0;
    theme->icon_scale = 100;
}

/* Splits "key = value" out of a line; returns 0 when a key was found. */
static int jbm_split_kv(const char *line, char *key, u32 kcap, char *val, u32 vcap) {
    const char *eq = strchr(line, '=');
    if (!eq) return -1;
    int keylen = (int)(eq - line);
    if (keylen >= (int)kcap) keylen = (int)kcap - 1;
    memcpy(key, line, (u32)keylen);
    key[keylen] = 0;
    jbm_trim(key);
    jbm_strcpy_(val, eq + 1, vcap);
    jbm_trim(val);
    return 0;
}

static void jbm_parse_config_line(const char *line, jbm_config_t *cfg) {
    char key[128], val[160];
    if (jbm_split_kv(line, key, sizeof(key), val, sizeof(val)) != 0) return;

    if (strcmp(key, "config_version") == 0) cfg->config_version = jbm_parse_int(val, cfg->config_version);
    else if (strcmp(key, "background") == 0) jbm_strcpy_(cfg->background_path, val, sizeof(cfg->background_path));
    else if (strcmp(key, "title") == 0) jbm_strcpy_(cfg->title, val, sizeof(cfg->title));
    else if (strcmp(key, "subtitle") == 0) jbm_strcpy_(cfg->subtitle, val, sizeof(cfg->subtitle));
    else if (strcmp(key, "theme") == 0) jbm_strcpy_(cfg->theme_name, val, sizeof(cfg->theme_name));
    else if (strcmp(key, "button_style") == 0) { cfg->button_style = jbm_parse_int(val, cfg->button_style); cfg->mask |= JBM_K_BTN_STYLE; }
    else if (strcmp(key, "text_align") == 0) { cfg->text_align = jbm_parse_int(val, cfg->text_align); cfg->mask |= JBM_K_TEXT_ALIGN; }
    else if (strcmp(key, "layout_margin") == 0) { cfg->layout_margin = jbm_parse_int(val, cfg->layout_margin); cfg->mask |= JBM_K_LAYOUT_MARGIN; }
    else if (strcmp(key, "button_alpha") == 0) { cfg->button_alpha = jbm_parse_int(val, cfg->button_alpha); cfg->mask |= JBM_K_BTN_ALPHA; }
    else if (strcmp(key, "button_border_width") == 0) { cfg->button_border_width = jbm_parse_int(val, cfg->button_border_width); cfg->mask |= JBM_K_BTN_BORDER; }
    else if (strcmp(key, "icon_scale") == 0) { cfg->icon_scale = jbm_parse_int(val, cfg->icon_scale); cfg->mask |= JBM_K_ICON_SCALE; }
    else if (strcmp(key, "show_title") == 0) { cfg->show_title = jbm_parse_bool(val); cfg->mask |= JBM_K_SHOW_TITLE; }
    else if (strcmp(key, "show_description") == 0) { cfg->show_description = jbm_parse_bool(val); cfg->mask |= JBM_K_SHOW_DESC; }
    else if (strcmp(key, "show_icons") == 0) { cfg->show_icons = jbm_parse_bool(val); cfg->mask |= JBM_K_SHOW_ICONS; }
    else if (strcmp(key, "color_bg") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_bg = (u32)c; cfg->mask |= JBM_K_COLOR_BG; } }
    else if (strcmp(key, "color_fg") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_fg = (u32)c; cfg->mask |= JBM_K_COLOR_FG; } }
    else if (strcmp(key, "color_border") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_border = (u32)c; cfg->mask |= JBM_K_COLOR_BORDER; } }
    else if (strcmp(key, "color_highlight") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_highlight = (u32)c; cfg->mask |= JBM_K_COLOR_HL; } }
    else if (strcmp(key, "color_selected_bg") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_selected_bg = (u32)c; cfg->mask |= JBM_K_COLOR_SEL_BG; } }
    else if (strcmp(key, "color_pressed_bg") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_pressed_bg = (u32)c; cfg->mask |= JBM_K_COLOR_PRESSED; } }
    else if (strcmp(key, "color_disabled_text") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_disabled_text = (u32)c; cfg->mask |= JBM_K_COLOR_DISABLED; } }
    else if (strcmp(key, "color_text") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_text = (u32)c; cfg->mask |= JBM_K_COLOR_TEXT; } }
    else if (strcmp(key, "color_text_sel") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_text_sel = (u32)c; cfg->mask |= JBM_K_COLOR_TEXTSEL; } }
    else if (strcmp(key, "color_accent") == 0) { u32 c = jbm_parse_color(val); if (c) { cfg->color_accent = (u32)c; cfg->mask |= JBM_K_COLOR_ACCENT; } }
    else if (strcmp(key, "button_width") == 0) { cfg->button_width = jbm_parse_int(val, cfg->button_width); cfg->mask |= JBM_K_BTN_W; }
    else if (strcmp(key, "button_height") == 0) { cfg->button_height = jbm_parse_int(val, cfg->button_height); cfg->mask |= JBM_K_BTN_H; }
    else if (strcmp(key, "button_spacing") == 0) { cfg->button_spacing = jbm_parse_int(val, cfg->button_spacing); cfg->mask |= JBM_K_BTN_GAP; }
    else if (strcmp(key, "button_radius") == 0) { cfg->button_radius = jbm_parse_int(val, cfg->button_radius); cfg->mask |= JBM_K_BTN_R; }
    else if (strcmp(key, "text_size") == 0) { cfg->text_size = jbm_parse_int(val, cfg->text_size); cfg->mask |= JBM_K_TEXT_SIZE; }
    else if (strcmp(key, "menu_timeout_sec") == 0) cfg->menu_timeout_sec = jbm_parse_int(val, cfg->menu_timeout_sec);
    else if (strcmp(key, "default_entry") == 0) cfg->default_entry = jbm_parse_int(val, cfg->default_entry);
    else if (strcmp(key, "show_countdown") == 0) cfg->show_countdown = jbm_parse_bool(val);
    else if (strcmp(key, "show_brand") == 0) cfg->show_brand = jbm_parse_bool(val);
    else if (strcmp(key, "orientation") == 0) {
        if (strcmp(val, "auto") == 0) cfg->orientation = -1;
        else {
            int o = jbm_parse_int(val, cfg->orientation);
            if (o == 1 || o == 2 || o == 3) o *= 90; /* quarter turns accepted too */
            cfg->orientation = o;
        }
    }
    else if (strcmp(key, "input_device") == 0) jbm_strcpy_(cfg->input_device, val, sizeof(cfg->input_device));
    else if (strcmp(key, "touch_cal_x0") == 0) cfg->touch_cal_x0 = jbm_parse_int(val, cfg->touch_cal_x0);
    else if (strcmp(key, "touch_cal_y0") == 0) cfg->touch_cal_y0 = jbm_parse_int(val, cfg->touch_cal_y0);
    else if (strcmp(key, "touch_cal_x1") == 0) cfg->touch_cal_x1 = jbm_parse_int(val, cfg->touch_cal_x1);
    else if (strcmp(key, "touch_cal_y1") == 0) cfg->touch_cal_y1 = jbm_parse_int(val, cfg->touch_cal_y1);
    else if (strcmp(key, "prefer_drm") == 0) cfg->prefer_drm = jbm_parse_bool(val);
    else if (strcmp(key, "fallback_to_fb") == 0) cfg->fallback_to_fb = jbm_parse_bool(val);
    else if (strcmp(key, "log_level") == 0) cfg->log_level = jbm_parse_int(val, cfg->log_level);
    else if (strcmp(key, "log_file") == 0) cfg->log_file = jbm_parse_bool(val);
}

/* Applies every "key = value" of an INI buffer, ignoring sections/comments. */
static void jbm_foreach_kv(char *buf, void (*fn)(const char *, void *), void *ud) {
    char *line = buf;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        jbm_trim(line);
        if (*line && *line != '#' && *line != ';' && *line != '[') fn(line, ud);
        if (nl) line = nl + 1;
        else break;
    }
}

static void jbm_cfg_kv_cb(const char *line, void *ud) { jbm_parse_config_line(line, (jbm_config_t *)ud); }
static void jbm_parse_theme_line(const char *line, jbm_theme_t *theme);
static void jbm_theme_kv_cb(const char *line, void *ud) { jbm_parse_theme_line(line, (jbm_theme_t *)ud); }

static void jbm_load_config(jbm_config_t *cfg) {
    jbm_set_default_config(cfg);
    char buf[8192];
    char path[192];
    jbm_sn(path, "%s/config.ini", JBM_ROOT);
    if (jbm_read_text(path, buf, sizeof(buf)) == 0) {
        jbm_foreach_kv(buf, jbm_cfg_kv_cb, cfg);
        jbm_log("jbm: loaded config from %s", path);
    } else {
        jbm_log("jbm: no config at %s, using defaults", path);
    }
}

static void jbm_parse_theme_line(const char *line, jbm_theme_t *theme) {
    char key[128], val[160];
    if (jbm_split_kv(line, key, sizeof(key), val, sizeof(val)) != 0) return;

    if (strcmp(key, "extends") == 0) jbm_strcpy_(theme->extends, val, sizeof(theme->extends));
    else if (strcmp(key, "button_style") == 0) theme->button_style = jbm_parse_int(val, theme->button_style);
    else if (strcmp(key, "color_bg") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_bg = (u32)c; }
    else if (strcmp(key, "color_fg") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_fg = (u32)c; }
    else if (strcmp(key, "color_border") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_border = (u32)c; }
    else if (strcmp(key, "color_highlight") == 0) {
        u32 c = jbm_parse_color(val);
        if (c) { theme->color_highlight = (u32)c; theme->color_selected_bg = (u32)c; }
    }
    else if (strcmp(key, "color_selected_bg") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_selected_bg = (u32)c; }
    else if (strcmp(key, "color_pressed_bg") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_pressed_bg = (u32)c; }
    else if (strcmp(key, "color_disabled_text") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_disabled_text = (u32)c; }
    else if (strcmp(key, "color_text") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_text = (u32)c; }
    else if (strcmp(key, "color_text_sel") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_text_sel = (u32)c; }
    else if (strcmp(key, "color_accent") == 0) { u32 c = jbm_parse_color(val); if (c) theme->color_accent = (u32)c; }
    else if (strcmp(key, "button_width") == 0) theme->button_width = jbm_parse_int(val, theme->button_width);
    else if (strcmp(key, "button_height") == 0) theme->button_height = jbm_parse_int(val, theme->button_height);
    else if (strcmp(key, "button_spacing") == 0) theme->button_spacing = jbm_parse_int(val, theme->button_spacing);
    else if (strcmp(key, "button_radius") == 0) theme->button_radius = jbm_parse_int(val, theme->button_radius);
    else if (strcmp(key, "button_border_width") == 0) theme->button_border_width = jbm_parse_int(val, theme->button_border_width);
    else if (strcmp(key, "button_alpha") == 0) theme->button_alpha = jbm_parse_int(val, theme->button_alpha);
    else if (strcmp(key, "text_size") == 0) theme->text_size = jbm_parse_int(val, theme->text_size);
    else if (strcmp(key, "text_align") == 0) theme->text_align = jbm_parse_int(val, theme->text_align);
    else if (strcmp(key, "layout_margin") == 0) theme->layout_margin = jbm_parse_int(val, theme->layout_margin);
    else if (strcmp(key, "show_title") == 0) theme->show_title = jbm_parse_bool(val);
    else if (strcmp(key, "show_description") == 0) theme->show_description = jbm_parse_bool(val);
    else if (strcmp(key, "show_icons") == 0) theme->show_icons = jbm_parse_bool(val);
    else if (strcmp(key, "icon_scale") == 0) theme->icon_scale = jbm_parse_int(val, theme->icon_scale);
}

static int jbm_load_theme_file_depth(const char *name, jbm_theme_t *theme, int depth);

/* Merges <root>/themes/<name>.ini over an already-initialised theme,
 * resolving `extends = <theme>` first (up to two levels). */
static int jbm_load_theme_file(const char *name, jbm_theme_t *theme) {
    return jbm_load_theme_file_depth(name, theme, 0);
}

static int jbm_load_theme_file_depth(const char *name, jbm_theme_t *theme, int depth) {
    char path[192];
    jbm_sn(path, "%s/themes/%s.ini", JBM_ROOT, name);
    char buf[4096];
    if (jbm_read_text(path, buf, sizeof(buf)) != 0) {
        jbm_log("jbm: theme %s not found at %s, keeping base theme", name, path);
        return -1;
    }
    /* Resolve inheritance by peeking at the buffer before applying it. */
    if (depth < 2) {
        char parent[JBM_MAX_THEME_NAME];
        parent[0] = 0;
        char scan[4096];
        jbm_strcpy_(scan, buf, sizeof(scan));
        char *line = scan;
        while (*line) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = 0;
            jbm_trim(line);
            char key[64], val[128];
            if (line[0] != '#' && line[0] != ';' && line[0] != '[' &&
                jbm_split_kv(line, key, sizeof(key), val, sizeof(val)) == 0 &&
                strcmp(key, "extends") == 0) {
                jbm_strcpy_(parent, val, sizeof(parent));
                break;
            }
            if (nl) line = nl + 1;
            else break;
        }
        if (parent[0] && strcmp(parent, name) != 0)
            jbm_load_theme_file_depth(parent, theme, depth + 1);
    }
    jbm_foreach_kv(buf, jbm_theme_kv_cb, theme);
    jbm_strcpy_(theme->name, name, sizeof(theme->name));
    jbm_log("jbm: loaded theme %s from %s", name, path);
    return 0;
}

/* Clamps values that reach the renderer so a malformed file can never make the
 * menu draw out of bounds or divide by zero. */
static int jbm_clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void jbm_validate_config(void) {
    JBM_CFG.button_width = jbm_clampi(JBM_CFG.button_width, 80, 4000);
    JBM_CFG.button_height = jbm_clampi(JBM_CFG.button_height, 40, 1200);
    JBM_CFG.button_spacing = jbm_clampi(JBM_CFG.button_spacing, 0, 400);
    JBM_CFG.button_radius = jbm_clampi(JBM_CFG.button_radius, 0, 400);
    JBM_CFG.button_border_width = jbm_clampi(JBM_CFG.button_border_width, 0, 24);
    JBM_CFG.button_alpha = jbm_clampi(JBM_CFG.button_alpha, 0, 255);
    JBM_CFG.text_size = jbm_clampi(JBM_CFG.text_size, 8, 96);
    JBM_CFG.text_align = jbm_clampi(JBM_CFG.text_align, 0, 2);
    JBM_CFG.layout_margin = jbm_clampi(JBM_CFG.layout_margin, 0, 35);
    JBM_CFG.icon_scale = jbm_clampi(JBM_CFG.icon_scale, 50, 300);
    JBM_CFG.button_style = jbm_clampi(JBM_CFG.button_style, 0, 3);
    JBM_CFG.menu_timeout_sec = jbm_clampi(JBM_CFG.menu_timeout_sec, 0, 3600);
    JBM_CFG.log_level = jbm_clampi(JBM_CFG.log_level, 0, 7);
    for (int i = 0; i < JBM_NUM_ENTRIES; i++) {
        jbm_entry_t *e = &JBM_ENTRIES[i];
        if (e->button_style < -1) e->button_style = -1;
        if (e->button_style > 3) e->button_style = 3;
        if (e->button_width > 0) e->button_width = jbm_clampi(e->button_width, 80, 4000);
        if (e->button_height > 0) e->button_height = jbm_clampi(e->button_height, 40, 1200);
        if (e->button_radius >= 0) e->button_radius = jbm_clampi(e->button_radius, 0, 400);
        if (e->button_border_width >= 0) e->button_border_width = jbm_clampi(e->button_border_width, 0, 24);
        if (e->icon_scale > 0) e->icon_scale = jbm_clampi(e->icon_scale, 50, 300);
        if (e->text_align < -1) e->text_align = -1;
        if (e->text_align > 2) e->text_align = 2;
    }
}

/* Theme file first, then only the config colours/metrics that were set. */
static void jbm_build_effects(void) {
    jbm_set_default_theme(&JBM_FX);
    if (JBM_CFG.theme_name[0]) jbm_load_theme_file(JBM_CFG.theme_name, &JBM_FX);
    if (JBM_CFG.mask & JBM_K_COLOR_BG) JBM_FX.color_bg = JBM_CFG.color_bg;
    if (JBM_CFG.mask & JBM_K_COLOR_FG) JBM_FX.color_fg = JBM_CFG.color_fg;
    if (JBM_CFG.mask & JBM_K_COLOR_BORDER) JBM_FX.color_border = JBM_CFG.color_border;
    if (JBM_CFG.mask & JBM_K_COLOR_HL) JBM_FX.color_highlight = JBM_CFG.color_highlight;
    if (JBM_CFG.mask & JBM_K_COLOR_SEL_BG) JBM_FX.color_selected_bg = JBM_CFG.color_selected_bg;
    if (JBM_CFG.mask & JBM_K_COLOR_PRESSED) JBM_FX.color_pressed_bg = JBM_CFG.color_pressed_bg;
    if (JBM_CFG.mask & JBM_K_COLOR_DISABLED) JBM_FX.color_disabled_text = JBM_CFG.color_disabled_text;
    if (JBM_CFG.mask & JBM_K_COLOR_TEXT) JBM_FX.color_text = JBM_CFG.color_text;
    if (JBM_CFG.mask & JBM_K_COLOR_TEXTSEL) JBM_FX.color_text_sel = JBM_CFG.color_text_sel;
    if (JBM_CFG.mask & JBM_K_COLOR_ACCENT) JBM_FX.color_accent = JBM_CFG.color_accent;
    if (JBM_CFG.mask & JBM_K_BTN_W) JBM_FX.button_width = JBM_CFG.button_width;
    if (JBM_CFG.mask & JBM_K_BTN_H) JBM_FX.button_height = JBM_CFG.button_height;
    if (JBM_CFG.mask & JBM_K_BTN_GAP) JBM_FX.button_spacing = JBM_CFG.button_spacing;
    if (JBM_CFG.mask & JBM_K_BTN_R) JBM_FX.button_radius = JBM_CFG.button_radius;
    if (JBM_CFG.mask & JBM_K_BTN_BORDER) JBM_FX.button_border_width = JBM_CFG.button_border_width;
    if (JBM_CFG.mask & JBM_K_BTN_ALPHA) JBM_FX.button_alpha = JBM_CFG.button_alpha;
    if (JBM_CFG.mask & JBM_K_TEXT_SIZE) JBM_FX.text_size = JBM_CFG.text_size;
    if (JBM_CFG.mask & JBM_K_LAYOUT_MARGIN) JBM_FX.layout_margin = JBM_CFG.layout_margin;
    if (JBM_CFG.mask & JBM_K_ICON_SCALE) JBM_FX.icon_scale = JBM_CFG.icon_scale;
    if (JBM_CFG.mask & JBM_K_BTN_STYLE) JBM_FX.button_style = JBM_CFG.button_style;
    if (JBM_CFG.mask & JBM_K_SHOW_TITLE) JBM_FX.show_title = JBM_CFG.show_title;
    if (JBM_CFG.mask & JBM_K_SHOW_DESC) JBM_FX.show_description = JBM_CFG.show_description;
    if (JBM_CFG.mask & JBM_K_SHOW_ICONS) JBM_FX.show_icons = JBM_CFG.show_icons;
    if (JBM_CFG.mask & JBM_K_TEXT_ALIGN) JBM_FX.text_align = JBM_CFG.text_align;
    /* Legacy configs only know color_highlight; keep its selected fill. */
    if ((JBM_CFG.mask & JBM_K_COLOR_HL) && !(JBM_CFG.mask & JBM_K_COLOR_SEL_BG))
        JBM_FX.color_selected_bg = JBM_FX.color_highlight;
    jbm_validate_config();
    jbm_log("jbm: effects bg=%08x accent=%08x hl=%08x sel=%08x text=%08x style=%d", JBM_FX.color_bg,
            JBM_FX.color_accent, JBM_FX.color_highlight, JBM_FX.color_selected_bg,
            JBM_FX.color_text, JBM_FX.button_style);
}

static jbm_entry_type_t jbm_parse_entry_type(const char *s) {
    if (!s) return JBM_ENTRY_TYPE_SYSTEM;
    if (strcmp(s, "system") == 0) return JBM_ENTRY_TYPE_SYSTEM;
    if (strcmp(s, "recovery") == 0) return JBM_ENTRY_TYPE_RECOVERY;
    if (strcmp(s, "fastboot") == 0) return JBM_ENTRY_TYPE_FASTBOOT;
    if (strcmp(s, "download") == 0) return JBM_ENTRY_TYPE_DOWNLOAD;
    if (strcmp(s, "reboot") == 0) return JBM_ENTRY_TYPE_REBOOT;
    if (strcmp(s, "poweroff") == 0) return JBM_ENTRY_TYPE_POWEROFF;
    if (strcmp(s, "script") == 0) return JBM_ENTRY_TYPE_SCRIPT;
    if (strcmp(s, "binary") == 0) return JBM_ENTRY_TYPE_BINARY;
    return JBM_ENTRY_TYPE_SYSTEM;
}

static void jbm_parse_entry_line(const char *line, jbm_entry_t *entry) {
    char key[128], val[256];
    if (jbm_split_kv(line, key, sizeof(key), val, sizeof(val)) != 0) return;

    if (strcmp(key, "name") == 0) jbm_strcpy_(entry->name, val, sizeof(entry->name));
    else if (strcmp(key, "description") == 0) jbm_strcpy_(entry->description, val, sizeof(entry->description));
    else if (strcmp(key, "icon") == 0) jbm_strcpy_(entry->icon, val, sizeof(entry->icon));
    else if (strcmp(key, "type") == 0) entry->type = jbm_parse_entry_type(val);
    else if (strcmp(key, "path") == 0) jbm_strcpy_(entry->path, val, sizeof(entry->path));
    else if (strcmp(key, "args") == 0) jbm_strcpy_(entry->args, val, sizeof(entry->args));
    else if (strcmp(key, "enabled") == 0) entry->enabled = jbm_parse_bool(val);
    else if (strcmp(key, "order") == 0) entry->order = jbm_parse_int(val, entry->order);
    else if (strcmp(key, "default") == 0) entry->is_default = jbm_parse_bool(val);
    else if (strcmp(key, "behavior_after") == 0) entry->behavior_after = jbm_parse_int(val, entry->behavior_after);
    else if (strcmp(key, "button_style") == 0) entry->button_style = jbm_parse_int(val, entry->button_style);
    else if (strcmp(key, "button_width") == 0) entry->button_width = jbm_parse_int(val, entry->button_width);
    else if (strcmp(key, "button_height") == 0) entry->button_height = jbm_parse_int(val, entry->button_height);
    else if (strcmp(key, "button_radius") == 0) entry->button_radius = jbm_parse_int(val, entry->button_radius);
    else if (strcmp(key, "button_border_width") == 0) entry->button_border_width = jbm_parse_int(val, entry->button_border_width);
    else if (strcmp(key, "text_align") == 0) entry->text_align = jbm_parse_int(val, entry->text_align);
    else if (strcmp(key, "icon_scale") == 0) entry->icon_scale = jbm_parse_int(val, entry->icon_scale);
    else if (strcmp(key, "show_icon") == 0) entry->show_icon = jbm_parse_bool(val);
    else if (strcmp(key, "color_bg") == 0) { u32 c = jbm_parse_color(val); if (c) entry->color_bg = c; }
    else if (strcmp(key, "color_border") == 0) { u32 c = jbm_parse_color(val); if (c) entry->color_border = c; }
    else if (strcmp(key, "color_text") == 0) { u32 c = jbm_parse_color(val); if (c) entry->color_text = c; }
    else if (strcmp(key, "color_text_sel") == 0) { u32 c = jbm_parse_color(val); if (c) entry->color_text_sel = c; }
}

static void jbm_entry_defaults(jbm_entry_t *e) {
    e->button_style = -1;
    e->button_width = 0;
    e->button_height = 0;
    e->button_radius = -1;
    e->button_border_width = -1;
    e->text_align = -1;
    e->icon_scale = 0;
    e->show_icon = -1;
    e->color_bg = 0;
    e->color_border = 0;
    e->color_text = 0;
    e->color_text_sel = 0;
}

static jbm_entry_t jbm_entry_tmp;
static void jbm_load_entries(void) {
    JBM_NUM_ENTRIES = 0;
    char buf[16384];
    char path[192];
    jbm_sn(path, "%s/entries.ini", JBM_ROOT);
    if (jbm_read_text(path, buf, sizeof(buf)) != 0) {
        jbm_log("jbm: no entries at %s, using built-in entries", path);
        return;
    }

    jbm_entry_t *cur = NULL;
    char *line = buf;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        jbm_trim(line);
        if (line[0] == '[') {
            /* flush the previous section */
            if (cur && cur->enabled && JBM_NUM_ENTRIES < JBM_MAX_ENTRIES)
                JBM_ENTRIES[JBM_NUM_ENTRIES++] = *cur;
            cur = &jbm_entry_tmp;
            memset(cur, 0, sizeof(*cur));
            jbm_entry_defaults(cur);
            cur->enabled = 1;
            cur->order = JBM_NUM_ENTRIES;
            char *end = strchr(line + 1, ']');
            if (end) *end = 0;
            jbm_strcpy_(cur->id, line + 1, sizeof(cur->id));
        } else if (*line && cur) {
            jbm_parse_entry_line(line, cur);
        }
        if (nl) line = nl + 1;
        else break;
    }
    if (cur && cur->enabled && JBM_NUM_ENTRIES < JBM_MAX_ENTRIES)
        JBM_ENTRIES[JBM_NUM_ENTRIES++] = *cur;

    for (int i = 0; i < JBM_NUM_ENTRIES - 1; i++)
        for (int j = i + 1; j < JBM_NUM_ENTRIES; j++)
            if (JBM_ENTRIES[i].order > JBM_ENTRIES[j].order) {
                jbm_entry_t tmp = JBM_ENTRIES[i];
                JBM_ENTRIES[i] = JBM_ENTRIES[j];
                JBM_ENTRIES[j] = tmp;
            }

    jbm_log("jbm: loaded %d entries from %s", JBM_NUM_ENTRIES, path);
}

/* The six rows used both when entries.ini is missing and as the content of the
 * default entries.ini generated on first boot. Keep the two in sync. */
struct jbm_builtin_entry {
    const char *id;
    const char *name;
    jbm_entry_type_t type;
};
static const struct jbm_builtin_entry JBM_BUILTINS[] = {
    {"system", "System", JBM_ENTRY_TYPE_SYSTEM},
    {"recovery", "Recovery", JBM_ENTRY_TYPE_RECOVERY},
    {"fastboot", "Fastboot", JBM_ENTRY_TYPE_FASTBOOT},
    {"download", "Download", JBM_ENTRY_TYPE_DOWNLOAD},
    {"reboot", "Reboot", JBM_ENTRY_TYPE_REBOOT},
    {"poweroff", "Power off", JBM_ENTRY_TYPE_POWEROFF},
};
#define JBM_NUM_BUILTINS ((int)(sizeof(JBM_BUILTINS) / sizeof(JBM_BUILTINS[0])))

static const char *jbm_entry_type_name(jbm_entry_type_t t);
static const char *jbm_builtin_description(int i) {
    static const char *const d[] = {
        "Start Android", "Reboot to recovery", "Reboot to bootloader",
        "Reboot to download mode", "Restart the device", "Power the device off",
    };
    return (i >= 0 && i < 6) ? d[i] : "";
}
static const char *jbm_builtin_icon(int i) {
    static const char *const n[] = {"system", "recovery", "fastboot", "download", "reboot", "power"};
    return (i >= 0 && i < 6) ? n[i] : "";
}

static const char *jbm_entry_type_name(jbm_entry_type_t t) {
    switch (t) {
        case JBM_ENTRY_TYPE_RECOVERY: return "recovery";
        case JBM_ENTRY_TYPE_FASTBOOT: return "fastboot";
        case JBM_ENTRY_TYPE_DOWNLOAD: return "download";
        case JBM_ENTRY_TYPE_REBOOT: return "reboot";
        case JBM_ENTRY_TYPE_POWEROFF: return "poweroff";
        case JBM_ENTRY_TYPE_SCRIPT: return "script";
        case JBM_ENTRY_TYPE_BINARY: return "binary";
        default: return "system";
    }
}

static void jbm_ensure_default_entries(void) {
    if (JBM_NUM_ENTRIES > 0) return;
    for (int i = 0; i < JBM_NUM_BUILTINS; i++) {
        jbm_entry_t *e = &JBM_ENTRIES[JBM_NUM_ENTRIES++];
        memset(e, 0, sizeof(*e));
        jbm_entry_defaults(e);
        jbm_strcpy_(e->id, JBM_BUILTINS[i].id, sizeof(e->id));
        jbm_strcpy_(e->name, JBM_BUILTINS[i].name, sizeof(e->name));
        e->type = JBM_BUILTINS[i].type;
        e->enabled = 1;
        e->order = i;
        e->is_default = (i == 0);
    }
}

static int jbm_default_entry_index(void) {
    if (JBM_CFG.default_entry >= 0 && JBM_CFG.default_entry < JBM_NUM_ENTRIES)
        return JBM_CFG.default_entry;
    for (int i = 0; i < JBM_NUM_ENTRIES; i++)
        if (JBM_ENTRIES[i].is_default) return i;
    return JBM_NUM_ENTRIES > 0 ? 0 : -1;
}

/* ------------------------------------------------------------------ */
/* first-run config tree: create /cache/Jupiter and default files      */
/* ------------------------------------------------------------------ */

/* True when path opens as a directory (works for files too, but only a
 * directory passes O_DIRECTORY). */
static int jbm_dir_ok(const char *path) {
    int fd = jbm_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) return 0;
    jbm_close(fd);
    return 1;
}

/* Creates path and every missing parent with mode 0755. Existing components
 * are left alone (even if the mode differs). Returns 0 when path is a
 * directory afterwards, -1 on any failure. */
static int jbm_mkdir_p(const char *path) {
    char tmp[192];
    jbm_strcpy_(tmp, path, sizeof(tmp));
    u32 len = strlen(tmp);
    if (!len || len >= sizeof(tmp)) return -1;
    for (u32 i = 1; i <= len; i++) {
        if (tmp[i] != '/' && tmp[i] != 0) continue;
        char save = tmp[i];
        tmp[i] = 0;
        if (!jbm_dir_ok(tmp)) {
            i64 r = jbm_mkdir(tmp, 0755);
            if (!jbm_dir_ok(tmp)) {
                jbm_log("jbm: mkdir %s failed (%lld)", tmp, (long long)r);
                tmp[i] = save;
                return -1;
            }
        }
        tmp[i] = save;
    }
    return jbm_dir_ok(path) ? 0 : -1;
}

/* Appends s to buf[off..cap) and returns the new offset. */
static u32 jbm_app(char *buf, u32 cap, u32 off, const char *s) {
    while (*s && off + 1 < cap) buf[off++] = *s++;
    buf[off] = 0;
    return off;
}

static u32 jbm_app_str(char *b, u32 cap, u32 o, const char *key, const char *v) {
    char t[192];
    jbm_sn(t, "%s = %s\n", key, v);
    return jbm_app(b, cap, o, t);
}

static u32 jbm_app_int(char *b, u32 cap, u32 o, const char *key, int v) {
    char t[64];
    jbm_sn(t, "%s = %d\n", key, v);
    return jbm_app(b, cap, o, t);
}

static u32 jbm_app_color(char *b, u32 cap, u32 o, const char *key, u32 c) {
    char t[64];
    jbm_sn(t, "%s = #%06x\n", key, c & 0xFFFFFFu);
    return jbm_app(b, cap, o, t);
}

/* Writes data to path only when path does not exist, atomically through a
 * sibling .tmp file + rename. Existing files are never truncated. Returns 0
 * created, 1 already present (untouched), -1 on error. */
static int jbm_write_file_new(const char *path, const void *data, u32 len) {
    if (jbm_exists(path)) return 1;
    char tmp[224];
    jbm_sn(tmp, "%s.tmp", path);
    int fd = jbm_open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        jbm_log("jbm: cannot create %s", tmp);
        return -1;
    }
    const u8 *p = (const u8 *)data;
    u32 off = 0;
    while (off < len) {
        i64 w = jbm_write(fd, p + off, len - off);
        if (w <= 0) break;
        off += (u32)w;
    }
    jbm_close(fd);
    if (off < len) {
        jbm_log("jbm: short write to %s (%u/%u)", tmp, off, len);
        return -1;
    }
    if (jbm_rename(tmp, path) != 0) {
        jbm_log("jbm: rename %s -> %s failed", tmp, path);
        return -1;
    }
    return 0;
}

static int jbm_write_new_text(const char *path, const char *text) {
    return jbm_write_file_new(path, text, (u32)strlen(text));
}

static void jbm_write_default_config(void) {
    jbm_config_t d;
    jbm_set_default_config(&d);
    static char b[2048];
    char path[192];
    u32 o = 0;
    o = jbm_app(b, sizeof(b), o,
                "# JupiterBoot configuration -- generated on first boot.\n"
                "# Edit the values below and reboot. Delete this file to get the\n"
                "# defaults back. Full reference: README.md (Configuration).\n\n");
    o = jbm_app(b, sizeof(b), o, "# Look and feel\n");
    o = jbm_app_str(b, sizeof(b), o, "theme", d.theme_name);
    o = jbm_app_int(b, sizeof(b), o, "config_version", JBM_CONFIG_VERSION);
    o = jbm_app_str(b, sizeof(b), o, "title", d.title);
    o = jbm_app_str(b, sizeof(b), o, "subtitle", d.subtitle);
    o = jbm_app_int(b, sizeof(b), o, "show_title", d.show_title);
    o = jbm_app_int(b, sizeof(b), o, "show_description", d.show_description);
    o = jbm_app_int(b, sizeof(b), o, "show_icons", d.show_icons);
    o = jbm_app_int(b, sizeof(b), o, "icon_scale", d.icon_scale);
    o = jbm_app(b, sizeof(b), o, "# background = ");
    o = jbm_app(b, sizeof(b), o, JBM_ROOT);
    o = jbm_app(b, sizeof(b), o, "/background.bmp\n");
    o = jbm_app(b, sizeof(b), o,
                "# Colours/metrics live in themes/default.ini. Uncomment a line\n"
                "# here to override the theme just for this device.\n");
    o = jbm_app_int(b, sizeof(b), o, "# button_style", d.button_style);
    o = jbm_app_int(b, sizeof(b), o, "# button_alpha", d.button_alpha);
    o = jbm_app_int(b, sizeof(b), o, "# button_border_width", d.button_border_width);
    o = jbm_app_int(b, sizeof(b), o, "# text_align", d.text_align);
    o = jbm_app_int(b, sizeof(b), o, "# layout_margin", d.layout_margin);
    o = jbm_app_color(b, sizeof(b), o, "# color_bg", d.color_bg);
    o = jbm_app_color(b, sizeof(b), o, "# color_fg", d.color_fg);
    o = jbm_app_color(b, sizeof(b), o, "# color_border", d.color_border);
    o = jbm_app_color(b, sizeof(b), o, "# color_highlight", d.color_highlight);
    o = jbm_app_color(b, sizeof(b), o, "# color_selected_bg", d.color_selected_bg);
    o = jbm_app_color(b, sizeof(b), o, "# color_pressed_bg", d.color_pressed_bg);
    o = jbm_app_color(b, sizeof(b), o, "# color_disabled_text", d.color_disabled_text);
    o = jbm_app_color(b, sizeof(b), o, "# color_text", d.color_text);
    o = jbm_app_color(b, sizeof(b), o, "# color_text_sel", d.color_text_sel);
    o = jbm_app_color(b, sizeof(b), o, "# color_accent", d.color_accent);
    o = jbm_app_int(b, sizeof(b), o, "# button_width", d.button_width);
    o = jbm_app_int(b, sizeof(b), o, "# button_height", d.button_height);
    o = jbm_app_int(b, sizeof(b), o, "# button_spacing", d.button_spacing);
    o = jbm_app_int(b, sizeof(b), o, "# button_radius", d.button_radius);
    o = jbm_app_int(b, sizeof(b), o, "# text_size", d.text_size);
    o = jbm_app(b, sizeof(b), o, "\n# Behaviour\n");
    o = jbm_app_int(b, sizeof(b), o, "menu_timeout_sec", d.menu_timeout_sec);
    o = jbm_app_int(b, sizeof(b), o, "show_countdown", d.show_countdown);
    o = jbm_app_int(b, sizeof(b), o, "show_brand", d.show_brand);
    o = jbm_app_int(b, sizeof(b), o, "default_entry", d.default_entry);
    o = jbm_app(b, sizeof(b), o, "\n# Display (orientation: 0/90/180/270 or auto)\n");
    o = jbm_app_int(b, sizeof(b), o, "orientation", d.orientation);
    o = jbm_app_int(b, sizeof(b), o, "prefer_drm", d.prefer_drm);
    o = jbm_app_int(b, sizeof(b), o, "fallback_to_fb", d.fallback_to_fb);
    o = jbm_app(b, sizeof(b), o, "# input_device = event2\n");
    o = jbm_app(b, sizeof(b), o, "# touch_cal_x0 = 0\n# touch_cal_y0 = 0\n"
                                "# touch_cal_x1 = 0\n# touch_cal_y1 = 0\n");
    o = jbm_app(b, sizeof(b), o, "\n# Logging\n");
    o = jbm_app_int(b, sizeof(b), o, "log_level", d.log_level);
    o = jbm_app_int(b, sizeof(b), o, "log_file", d.log_file);
    (void)o;
    jbm_sn(path, "%s/config.ini", JBM_ROOT);
    jbm_write_new_text(path, b);
}

static void jbm_write_default_entries_file(void) {
    static char b[2048];
    char path[192], t[160];
    u32 o = 0;
    o = jbm_app(b, sizeof(b), o,
                "# JupiterBoot boot menu entries -- generated on first boot.\n"
                "# Each [section] is one row. Delete this file to get the\n"
                "# defaults back. type: system|recovery|fastboot|download|\n"
                "# reboot|poweroff|script|binary\n\n");
    for (int i = 0; i < JBM_NUM_BUILTINS; i++) {
        /* No `icon` line: the default modes render as text only. Users can
         * still add `icon = ...` plus show_icons = 1 to opt back in. */
        jbm_sn(t, "[%s]\nname = %s\ndescription = %s\ntype = %s\n"
                  "enabled = 1\norder = %d\ndefault = %d\n",
               JBM_BUILTINS[i].id, JBM_BUILTINS[i].name, jbm_builtin_description(i),
               jbm_entry_type_name(JBM_BUILTINS[i].type),
               i, i == 0 ? 1 : 0);
        o = jbm_app(b, sizeof(b), o, t);
        if (i + 1 < JBM_NUM_BUILTINS) o = jbm_app(b, sizeof(b), o, "\n");
    }
    (void)o;
    jbm_sn(path, "%s/entries.ini", JBM_ROOT);
    jbm_write_new_text(path, b);
}

static void jbm_theme_body(char *b, u32 cap, const jbm_theme_t *d, const char *extends) {
    u32 o = 0;
    o = jbm_app(b, cap, o, "# JupiterBoot theme -- generated on first boot.\n");
    if (extends && extends[0]) {
        char t[96];
        jbm_sn(t, "extends = %s\n", extends);
        o = jbm_app(b, cap, o, t);
    }
    o = jbm_app_int(b, cap, o, "button_style", d->button_style);
    o = jbm_app_color(b, cap, o, "color_bg", d->color_bg);
    o = jbm_app_color(b, cap, o, "color_fg", d->color_fg);
    o = jbm_app_color(b, cap, o, "color_border", d->color_border);
    o = jbm_app_color(b, cap, o, "color_highlight", d->color_highlight);
    o = jbm_app_color(b, cap, o, "color_selected_bg", d->color_selected_bg);
    o = jbm_app_color(b, cap, o, "color_pressed_bg", d->color_pressed_bg);
    o = jbm_app_color(b, cap, o, "color_disabled_text", d->color_disabled_text);
    o = jbm_app_color(b, cap, o, "color_text", d->color_text);
    o = jbm_app_color(b, cap, o, "color_text_sel", d->color_text_sel);
    o = jbm_app_color(b, cap, o, "color_accent", d->color_accent);
    o = jbm_app_int(b, cap, o, "button_width", d->button_width);
    o = jbm_app_int(b, cap, o, "button_height", d->button_height);
    o = jbm_app_int(b, cap, o, "button_spacing", d->button_spacing);
    o = jbm_app_int(b, cap, o, "button_radius", d->button_radius);
    o = jbm_app_int(b, cap, o, "button_border_width", d->button_border_width);
    o = jbm_app_int(b, cap, o, "button_alpha", d->button_alpha);
    o = jbm_app_int(b, cap, o, "text_size", d->text_size);
    o = jbm_app_int(b, cap, o, "text_align", d->text_align);
    o = jbm_app_int(b, cap, o, "layout_margin", d->layout_margin);
    o = jbm_app_int(b, cap, o, "show_title", d->show_title);
    o = jbm_app_int(b, cap, o, "show_description", d->show_description);
    o = jbm_app_int(b, cap, o, "show_icons", d->show_icons);
    o = jbm_app_int(b, cap, o, "icon_scale", d->icon_scale);
    (void)o;
}

static void jbm_write_theme_named(const char *name, const jbm_theme_t *theme, const char *extends) {
    static char b[2048];
    char path[192];
    jbm_theme_body(b, sizeof(b), theme, extends);
    jbm_sn(path, "%s/themes/%s.ini", JBM_ROOT, name);
    jbm_write_new_text(path, b);
}

static void jbm_write_default_theme_file(void) {
    jbm_theme_t d;
    jbm_set_default_theme(&d);
    jbm_write_theme_named("default", &d, NULL);

    /* Jupiter: the original warm amber-on-space look. */
    d.button_style = 0;
    d.color_bg = 0xFF020202;
    d.color_border = 0xFFD88A4E;
    d.color_highlight = 0xFFB86837;
    d.color_selected_bg = 0xFFB86837;
    d.color_pressed_bg = 0xFFD88A4E;
    d.color_text = 0xFF858585;
    d.color_text_sel = 0xFFFFFFFF;
    d.color_accent = 0xFFC87335;
    d.button_radius = 7;
    d.button_alpha = 255;
    jbm_write_theme_named("jupiter", &d, NULL);

    /* AMOLED: true black, thin outlined buttons, no fills. */
    d.button_style = 1;
    d.color_bg = 0xFF000000;
    d.color_fg = 0xFF000000;
    d.color_border = 0xFF3A3A3A;
    d.color_highlight = 0xFF101010;
    d.color_selected_bg = 0xFF1C1C1C;
    d.color_pressed_bg = 0xFF2A2A2A;
    d.color_text = 0xFF9A9A9A;
    d.color_text_sel = 0xFFFFFFFF;
    d.color_accent = 0xFFE6E6E6;
    d.button_alpha = 0;
    d.button_radius = 2;
    d.button_border_width = 1;
    d.text_size = 18;
    jbm_write_theme_named("amoled", &d, NULL);

    /* Minimal: flat, borderless, tight spacing. */
    d.button_style = 2;
    d.color_bg = 0xFF111318;
    d.color_border = 0xFF111318;
    d.color_highlight = 0xFF1E222A;
    d.color_selected_bg = 0xFF1E222A;
    d.color_pressed_bg = 0xFF2A2F39;
    d.color_text = 0xFFA6ADBA;
    d.color_text_sel = 0xFFF5F7FA;
    d.color_accent = 0xFF7DA2C9;
    d.button_alpha = 0;
    d.button_radius = 0;
    d.button_border_width = 0;
    d.button_spacing = 4;
    d.text_size = 17;
    jbm_write_theme_named("minimal", &d, NULL);
    jbm_log("jbm: ensured themes default/jupiter/amoled/minimal under %s/themes", JBM_ROOT);
}

static void jbm_le16(u8 *p, u16 v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
}
static void jbm_le32(u8 *p, u32 v) {
    p[0] = (u8)v;
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}

/* A small, valid 24-bpp BMP that approximates the built-in menu backdrop: a
 * dark field with the same warm glow in the same corner. Generated once so the
 * user has a real file to replace; deleting it brings the procedural scene
 * (which also draws the planet) back. */
static void jbm_write_default_background_file(void) {
    const int W = 180, H = 400;
    const u32 row = (u32)W * 3u; /* 540, already 4-byte aligned */
    const u32 datasize = row * (u32)H;
    const u32 filesize = 54u + datasize;
    char path[192], tmp[224];
    jbm_sn(path, "%s/background.bmp", JBM_ROOT);
    if (jbm_exists(path)) return;
    jbm_sn(tmp, "%s.tmp", path);
    int fd = jbm_open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        jbm_log("jbm: cannot create %s", tmp);
        return;
    }
    u8 hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B';
    hdr[1] = 'M';
    jbm_le32(hdr + 2, filesize);
    jbm_le32(hdr + 10, 54);
    jbm_le32(hdr + 14, 40);
    jbm_le32(hdr + 18, (u32)W);
    jbm_le32(hdr + 22, (u32)H);
    jbm_le16(hdr + 26, 1);
    jbm_le16(hdr + 28, 24);
    jbm_le32(hdr + 34, datasize);
    if (jbm_write(fd, hdr, 54) != 54) {
        jbm_close(fd);
        return;
    }
    int gx = W * 72 / 100, gy = H * 45 / 100, rad = 130;
    i64 rad2 = (i64)rad * rad;
    u8 rowbuf[180 * 3 + 4];
    for (int y = 0; y < H; y++) { /* file rows are bottom-up */
        int yy = H - 1 - y;
        for (int x = 0; x < W; x++) {
            int r = 2, g = 2, bl = 2;
            i64 dx = x - gx, dy = yy - gy;
            i64 d2 = dx * dx + dy * dy;
            if (d2 < rad2) {
                int f = (int)((rad2 - d2) * 200 / rad2); /* 0..200 */
                r += (0x64 - 2) * f / 200;
                g += (0x3E - 2) * f / 200;
                bl += (0x23 - 2) * f / 200;
            }
            rowbuf[x * 3 + 0] = (u8)bl;
            rowbuf[x * 3 + 1] = (u8)g;
            rowbuf[x * 3 + 2] = (u8)r;
        }
        if (jbm_write(fd, rowbuf, row) != (i64)row) {
            jbm_log("jbm: short write to %s", tmp);
            jbm_close(fd);
            return;
        }
    }
    jbm_close(fd);
    if (jbm_rename(tmp, path) != 0) jbm_log("jbm: rename %s -> %s failed", tmp, path);
}

static void jbm_ensure_tree(void) {
    char p[192];
    if (jbm_mkdir_p(JBM_ROOT) != 0) return;
    jbm_sn(p, "%s/themes", JBM_ROOT);
    jbm_mkdir_p(p);
    jbm_sn(p, "%s/scripts", JBM_ROOT);
    jbm_mkdir_p(p);
    jbm_sn(p, "%s/bin", JBM_ROOT);
    jbm_mkdir_p(p);
}

static void jbm_generate_defaults(void) {
    if (!jbm_dir_ok(JBM_ROOT)) {
        jbm_log("jbm: config root %s unavailable, skipping defaults", JBM_ROOT);
        return;
    }
    jbm_write_default_config();
    jbm_write_default_entries_file();
    jbm_write_default_theme_file();
    jbm_write_default_background_file();
    jbm_log("jbm: ensured default files under %s", JBM_ROOT);
}

/* ------------------------------------------------------------------ */
/* config root + /cache bring-up                                       */
/* ------------------------------------------------------------------ */

#ifndef JBM_HOST
/* True when the mount point already appears in /proc/mounts. */
static int jbm_is_mounted(const char *mp) {
    char buf[4096];
    if (jbm_read_text("/proc/mounts", buf, sizeof(buf)) != 0) return 0;
    char *line = buf;
    while (*line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        char *sp = strchr(line, ' ');
        if (sp) {
            char *mp0 = sp + 1;
            char *sp2 = strchr(mp0, ' ');
            if (sp2) *sp2 = 0;
            if (strcmp(mp0, mp) == 0) return 1;
        }
        if (nl) line = nl + 1;
        else break;
    }
    return 0;
}
#endif /* !JBM_HOST */

/* Picks /cache/Jupiter when it holds anything, else the ramdisk /Jupiter. */
static void jbm_cfg_pick_root(void) {
    const char *cands[2] = {JBM_CONFIG_DIR, JBM_RAMDISK_DIR};
    const char *files[4] = {"config.ini", "entries.ini", "themes", "background.bmp"};
    for (int c = 0; c < 2; c++) {
        for (int f = 0; f < 4; f++) {
            char p[192];
            jbm_sn(p, "%s/%s", cands[c], files[f]);
            if (jbm_exists(p)) {
                jbm_strcpy_(JBM_ROOT, cands[c], sizeof(JBM_ROOT));
                return;
            }
        }
    }
    jbm_log("jbm: no config root found, defaulting to %s", JBM_ROOT);
}

/* Counts directory entries other than "." and "..". */
#ifndef JBM_HOST
static int jbm_dir_count(const char *path) {
    int fd = jbm_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (fd < 0) return -1;
    static u8 buf[4096];
    int n = 0;
    for (;;) {
        long r = jbm_getdents(fd, buf, (u32)sizeof(buf));
        if (r <= 0) break;
        long off = 0;
        while (off < r) {
            struct jbm_dirent64 *d = (struct jbm_dirent64 *)(buf + off);
            off += d->d_reclen;
            if (d->d_name[0] == '.' &&
                (d->d_name[1] == 0 || (d->d_name[1] == '.' && d->d_name[2] == 0)))
                continue;
            n++;
        }
    }
    jbm_close(fd);
    return n;
}
#endif /* !JBM_HOST */

#ifndef JBM_HOST
/* Mounts node on /cache as ext4 then f2fs. known_cache marks a device we have
 * already identified as the cache partition (by-name or PARTNAME=cache); for
 * those any successful mount is accepted. The blind fallback below stays
 * conservative so userdata is never mounted over /cache by mistake. */
static int jbm_mount_cache_dev(const char *node, int known_cache) {
    const char *types[2] = {"ext4", "f2fs"};
    for (int t = 0; t < 2; t++) {
        if (jbm_mount(node, "/cache", types[t], 0, NULL) != 0) continue;
        int has_jup = jbm_exists("/cache/Jupiter");
        int n = jbm_dir_count("/cache");
        if (known_cache || has_jup || (n >= 0 && n <= 3)) {
            JBM_CACHE_MOUNTED = 1;
            JBM_CACHE_READY = 1;
            jbm_log("jbm: mounted %s (%s) on /cache (entries=%d jupiter=%d)", node, types[t], n,
                    has_jup);
            return 1;
        }
        jbm_log("jbm: %s (%s) does not look like cache (entries=%d), unmounting", node, types[t], n);
        jbm_umount("/cache");
    }
    return 0;
}

/* Creates /dev/block/<name> from /sys/class/block/<name>/dev. */
static int jbm_block_node(const char *name, char *node, u32 cap) {
    char devp[128], raw[64];
    jbm_sn(devp, "/sys/class/block/%s/dev", name);
    if (jbm_read_text(devp, raw, sizeof(raw)) != 0) return -1;
    const char *colon = strchr(raw, ':');
    if (!colon) return -1;
    int maj = jbm_atoi_(raw), min = jbm_atoi_(colon + 1);
    jbm_sn(node, "/dev/block/%s", name);
    (void)cap;
    jbm_mknod(node, 0600 | S_IFBLK, (maj << 8) | min);
    return 0;
}

/* True when a sysfs uevent buffer has a line "PARTNAME=cache" (the last line
 * has no trailing newline after jbm_read_text trims it). */
static int jbm_uevent_is_cache(const char *u) {
    const char key[] = "PARTNAME=cache";
    for (const char *p = u; *p; p++) {
        int i = 0;
        while (key[i] && p[i] == key[i]) i++;
        if (!key[i] && (p[i] == '\n' || p[i] == 0)) return 1;
    }
    return 0;
}

/* True when node carries an ext4 volume label exactly "cache". */
static int jbm_dev_label_is_cache(const char *node) {
    int fd = jbm_open(node, O_RDONLY, 0);
    if (fd < 0) return 0;
    u8 sb[1024];
    int ok = 0;
    if (jbm_lseek(fd, 1024, SEEK_SET) == 1024 && jbm_read(fd, sb, sizeof(sb)) == (long)sizeof(sb)) {
        u16 magic = (u16)(sb[0x38] | (sb[0x39] << 8));
        if (magic == 0xEF53) { /* ext2/3/4 superblock magic */
            char lbl[17];
            memcpy(lbl, sb + 0x78, 16); /* s_volume_name */
            lbl[16] = 0;
            if (strcmp(lbl, "cache") == 0) ok = 1;
        }
    }
    jbm_close(fd);
    return ok;
}
#endif /* !JBM_HOST */

/* Best-effort /cache bring-up: the menu runs before the real init mounts it.
 * The fstab names the partition by-name/cache, so prefer that symlink and the
 * sysfs PARTNAME=cache label; only fall back to a blind probe when neither is
 * available. */
static void jbm_cache_init(void) __attribute__((unused));
static void jbm_cache_init(void) {
#ifdef JBM_HOST
    return;
#else
    jbm_mkdir("/cache", 0755);
    if (jbm_is_mounted("/cache")) {
        jbm_log("jbm: /cache already mounted");
        JBM_CACHE_READY = 1;
        return;
    }
    if (jbm_exists(JBM_RAMDISK_DIR)) {
        jbm_log("jbm: using ramdisk config root %s", JBM_RAMDISK_DIR);
        jbm_strcpy_(JBM_ROOT, JBM_RAMDISK_DIR, sizeof(JBM_ROOT));
        JBM_CACHE_READY = 1;
        return;
    }

    jbm_mkdir("/dev", 0755);
    jbm_mkdir("/dev/block", 0755);

    /* 1. by-name symlinks (usually created by ueventd, sometimes already
     * present in the initramfs). */
    const char *byname[3] = {"/dev/block/platform/bootdevice/by-name/cache",
                             "/dev/block/by-name/cache", "/dev/block/bootdevice/by-name/cache"};
    for (int i = 0; i < 3; i++) {
        if (jbm_exists(byname[i]) && jbm_mount_cache_dev(byname[i], 1)) return;
    }

    /* Phones expose well over 100 entries here (dm/loop/zram + every GPT
     * partition), so the buffer must not truncate before the cache partition. */
    static char names[256][32];
    int count = 0;
    int dfd = jbm_open("/sys/class/block", O_RDONLY, 0);
    if (dfd >= 0) {
        static u8 dbuf[8192];
        for (;;) {
            long r = jbm_getdents(dfd, dbuf, (u32)sizeof(dbuf));
            if (r <= 0) break;
            long off = 0;
            while (off < r && count < 256) {
                struct jbm_dirent64 *d = (struct jbm_dirent64 *)(dbuf + off);
                off += d->d_reclen;
                if (d->d_name[0] == '.') continue;
                jbm_strcpy_(names[count++], d->d_name, 32);
            }
        }
        jbm_close(dfd);
    }

    /* 2. the partition whose GPT/uevent label is exactly "cache". */
    for (int i = 0; i < count; i++) {
        char uevp[128], uev[256];
        jbm_sn(uevp, "/sys/class/block/%s/uevent", names[i]);
        if (jbm_read_text(uevp, uev, sizeof(uev)) != 0) continue;
        if (!jbm_uevent_is_cache(uev)) continue;
        char node[96];
        if (jbm_block_node(names[i], node, sizeof(node)) != 0) continue;
        jbm_log("jbm: found cache partition %s (%s)", names[i], node);
        if (jbm_mount_cache_dev(node, 1)) return;
    }

    /* 3. conservative blind probe. */
    for (int i = 0; i < count; i++) {
        const char *nm = names[i];
        if (jbm_str_has(nm, "loop") || jbm_str_has(nm, "ram") || jbm_str_has(nm, "zram") ||
            jbm_str_has(nm, "dm-") || jbm_str_has(nm, "sr") || jbm_str_has(nm, "rpmb") ||
            jbm_str_has(nm, "boot"))
            continue;
        char node[96];
        if (jbm_block_node(nm, node, sizeof(node)) != 0) continue;
        int known = jbm_dev_label_is_cache(node);
        if (jbm_mount_cache_dev(node, known)) return;
    }
    jbm_log("jbm: could not mount /cache, using defaults");
#endif
}

static void jbm_log_file_open(void) {
    if (!JBM_CFG.log_file || JBM_CFG.log_level < 6) return;
    char path[192];
    jbm_sn(path, "%s/boot.log", JBM_ROOT);
    jbm_log_fd = jbm_open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (jbm_log_fd < 0) jbm_log_fd = -1;
}

/* One-shot startup: bring /cache up, pick the root, load config/theme/entries
 * and reduce them to the JBM_FX the renderer uses. */
static void jbm_config_init(void) {
#ifdef JBM_HOST
    /* Host previews/tests point at a directory with JUPITER_CONFIG_ROOT; when
     * it is unset we only probe (read-only) and never create /cache on the PC. */
    const char *eroot = getenv("JUPITER_CONFIG_ROOT");
    if (eroot && eroot[0]) {
        jbm_strcpy_(JBM_ROOT, eroot, sizeof(JBM_ROOT));
        JBM_CFG_WRITABLE = 1;
        jbm_log("jbm: JUPITER_CONFIG_ROOT=%s", JBM_ROOT);
    } else {
        jbm_cfg_pick_root();
        JBM_CFG_WRITABLE = 0;
    }
#else
    jbm_cache_init();
    jbm_cfg_pick_root();
    JBM_CFG_WRITABLE = JBM_CACHE_READY || (strcmp(JBM_ROOT, JBM_RAMDISK_DIR) == 0);
#endif
    if (JBM_CFG_WRITABLE) {
        jbm_ensure_tree();
        jbm_generate_defaults();
    } else {
        jbm_log("jbm: config root %s not writable, using built-in defaults", JBM_ROOT);
    }
    jbm_load_config(&JBM_CFG);
    if (!JBM_CFG.background_path[0])
        jbm_sn(JBM_CFG.background_path, "%s/background.bmp", JBM_ROOT);
    jbm_load_entries();
    jbm_ensure_default_entries();
    jbm_build_effects();
    jbm_log_file_open();
    jbm_log("jbm: config root=%s entries=%d default=%d timeout=%ds", JBM_ROOT, JBM_NUM_ENTRIES,
            jbm_default_entry_index(), JBM_CFG.menu_timeout_sec);
}

/* ------------------------------------------------------------------ */
/* background bitmap (BMP)                                             */
/* ------------------------------------------------------------------ */

struct jbm_bg {
    u32 *px; /* ARGB, top-down, w*h */
    int w, h, ok;
};

static struct jbm_bg JBM_BG;

static int jbm_mask_shift(u32 m, int *bits) {
    if (!m) {
        *bits = 0;
        return 0;
    }
    int s = 0;
    while (!(m & 1u)) {
        m >>= 1;
        s++;
    }
    int b = 0;
    while (m & 1u) {
        m >>= 1;
        b++;
    }
    *bits = b;
    return s;
}

static u32 jbm_mask_val(u32 raw, u32 mask) {
    int bits;
    int sh = jbm_mask_shift(mask, &bits);
    if (!bits) return 0;
    u32 v = (raw & mask) >> sh;
    if (bits >= 8) return (v & 0xFFu);
    return (v * 255u) / (((u32)1u << bits) - 1u);
}

/* Loads an uncompressed 8/24/32-bpp BMP into JBM_BG. Returns 0 on success. */
static int jbm_bg_load(const char *path) {
    JBM_BG.ok = 0;
    JBM_BG.px = 0;
    if (!path || !path[0] || strcmp(path, "-") == 0) return -1;

    int fd = jbm_open(path, O_RDONLY, 0);
    if (fd < 0) {
        jbm_log("jbm: background %s not found, keeping procedural art", path);
        return -1;
    }
    u8 hdr[64];
    long hn = jbm_read(fd, hdr, sizeof(hdr));
    if (hn < 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        jbm_log("jbm: %s is not a BMP", path);
        jbm_close(fd);
        return -1;
    }
    u32 data_off = *(u32 *)(hdr + 10);
    u32 dib = *(u32 *)(hdr + 14);
    i32 w = *(i32 *)(hdr + 18);
    i32 h = *(i32 *)(hdr + 22);
    u16 planes = *(u16 *)(hdr + 26);
    u16 bpp = *(u16 *)(hdr + 28);
    u32 comp = *(u32 *)(hdr + 30);
    int topdown = 0;
    if (h < 0) {
        h = -h;
        topdown = 1;
    }
    if (dib < 40 || w <= 0 || h <= 0 || w > 8192 || h > 8192 || planes != 1 ||
        (bpp != 8 && bpp != 24 && bpp != 32) || (comp != 0 && comp != 3)) {
        jbm_log("jbm: unsupported BMP %s (dib=%u %dx%d bpp=%u comp=%u)", path, dib, w, h, bpp, comp);
        jbm_close(fd);
        return -1;
    }

    u32 rmask = 0, gmask = 0, bmask = 0, amask = 0;
    if (comp == 3) {
        u8 mk[16];
        if (jbm_lseek(fd, 14 + (i64)dib, SEEK_SET) != 14 + (i64)dib ||
            jbm_read(fd, mk, sizeof(mk)) < 12) {
            jbm_log("jbm: %s bitfield masks unreadable", path);
            jbm_close(fd);
            return -1;
        }
        rmask = *(u32 *)(mk);
        gmask = *(u32 *)(mk + 4);
        bmask = *(u32 *)(mk + 8);
        if (dib >= 56) amask = *(u32 *)(mk + 12);
    } else if (bpp == 32) {
        rmask = 0x00FF0000u;
        gmask = 0x0000FF00u;
        bmask = 0x000000FFu;
        amask = 0xFF000000u;
    } else if (bpp == 24) {
        rmask = 0x00FF0000u;
        gmask = 0x0000FF00u;
        bmask = 0x000000FFu;
    } else { /* 8 bpp palette entries are BGRA */
        rmask = 0x00FF0000u;
        gmask = 0x0000FF00u;
        bmask = 0x000000FFu;
    }

    u32 pal[256];
    if (bpp == 8) {
        if (jbm_lseek(fd, 14 + (i64)dib, SEEK_SET) != 14 + (i64)dib ||
            jbm_read(fd, pal, sizeof(pal)) < (long)sizeof(pal)) {
            jbm_log("jbm: %s palette unreadable", path);
            jbm_close(fd);
            return -1;
        }
        for (int i = 0; i < 256; i++) {
            u32 bgra = pal[i];
            u32 a = (bgra >> 24) & 0xFF;
            pal[i] = ((a ? a : 0xFFu) << 24) | ((bgra & 0xFF) << 16) | (bgra & 0xFF00) |
                     ((bgra >> 16) & 0xFF);
        }
    }

    u32 row = (((u32)w * bpp) + 31u) / 32u * 4u;
    u32 total = row * (u32)h;
    if (total > 64u * 1024u * 1024u) {
        jbm_log("jbm: %s too large (%u bytes)", path, total);
        jbm_close(fd);
        return -1;
    }
    if (jbm_lseek(fd, (i64)data_off, SEEK_SET) != (i64)data_off) {
        jbm_close(fd);
        return -1;
    }
    u8 *raw = (u8 *)jbm_mmap(total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!raw) {
        jbm_close(fd);
        return -1;
    }
    u32 got = 0;
    while (got < total) {
        long r = jbm_read(fd, raw + got, total - got);
        if (r <= 0) break;
        got += (u32)r;
    }
    jbm_close(fd);
    if (got < total) {
        jbm_log("jbm: %s short read (%u/%u)", path, got, total);
        jbm_release(raw, total);
        return -1;
    }

    u32 npx = (u32)w * (u32)h;
    u32 *px = (u32 *)jbm_mmap(npx * 4u, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!px) {
        jbm_release(raw, total);
        return -1;
    }
    for (int y = 0; y < h; y++) {
        u8 *srow = raw + (u32)y * row;
        u32 *drow = px + (u32)(topdown ? y : (h - 1 - y)) * (u32)w;
        for (int x = 0; x < w; x++) {
            u32 argb;
            if (bpp == 8) {
                argb = pal[srow[x]];
            } else if (bpp == 24) {
                u32 b = srow[x * 3], g = srow[x * 3 + 1], r = srow[x * 3 + 2];
                argb = 0xFF000000u | (r << 16) | (g << 8) | b;
            } else {
                u32 rawpx = *(u32 *)(srow + x * 4);
                u32 a = amask ? jbm_mask_val(rawpx, amask) : 0xFF;
                argb = ((a ? a : 0xFFu) << 24) | (jbm_mask_val(rawpx, rmask) << 16) |
                       (jbm_mask_val(rawpx, gmask) << 8) | jbm_mask_val(rawpx, bmask);
            }
            drow[x] = argb;
        }
    }
    jbm_release(raw, total);

    JBM_BG.px = px;
    JBM_BG.w = w;
    JBM_BG.h = h;
    JBM_BG.ok = 1;
    jbm_log("jbm: loaded background %s (%dx%d bpp=%u)", path, w, h, bpp);
    return 0;
}

/* ------------------------------------------------------------------ */
/* canvas / framebuffer                                                */
/* ------------------------------------------------------------------ */

/* Minimal DRM/KMS ABI (struct layouts and ioctl encodings match
 * include/uapi/drm/drm_mode.h) so the menu can also drive a KMS node when
 * there is no usable framebuffer, or when prefer_drm=1 is set. */
struct jbm_drm_mode_card_res {
    u64 fb_id_ptr, crtc_id_ptr, connector_id_ptr, encoder_id_ptr;
    u32 count_fbs, count_crtcs, count_connectors, count_encoders;
    u32 min_width, max_width, min_height, max_height;
};

struct jbm_drm_mode_modeinfo {
    u32 clock;
    u16 hdisplay, hsync_start, hsync_end, htotal, hskew;
    u16 vdisplay, vsync_start, vsync_end, vtotal, vscan;
    u32 vrefresh, flags, type;
    char name[32];
};

struct jbm_drm_mode_get_connector {
    u64 encoders_ptr, modes_ptr, props_ptr, prop_values_ptr;
    u32 count_modes, count_props, count_encoders, encoder_id;
    u32 connector_id, connector_type, connector_type_id, connection;
    u32 mm_width, mm_height, subpixel, pad;
};

struct jbm_drm_mode_get_encoder {
    u32 encoder_id, encoder_type, crtc_id, possible_crtcs, possible_clones;
};

struct jbm_drm_mode_crtc {
    u64 set_connectors_ptr;
    u32 count_connectors;
    u32 crtc_id, fb_id;
    u32 x, y;
    u32 gamma_size, mode_valid;
    struct jbm_drm_mode_modeinfo mode;
};

struct jbm_drm_mode_create_dumb {
    u32 height, width, bpp, flags, handle, pitch;
    u64 size;
};

struct jbm_drm_mode_map_dumb {
    u32 handle, pad;
    u64 offset;
};

struct jbm_drm_mode_fb_cmd {
    u32 fb_id, width, height, pitch, bpp, depth, handle;
};

#define JBM_IOC(dir, type, nr, size) (((dir) << 30) | ((size) << 16) | ((type) << 8) | (nr))
#define JBM_DRM_IOWR(nr, sz) JBM_IOC(3, 'd', nr, sz)
#define DRM_IOCTL_MODE_GETRESOURCES JBM_DRM_IOWR(0xA0, sizeof(struct jbm_drm_mode_card_res))
#define DRM_IOCTL_MODE_GETCRTC JBM_DRM_IOWR(0xA1, sizeof(struct jbm_drm_mode_crtc))
#define DRM_IOCTL_MODE_GETENCODER JBM_DRM_IOWR(0xA6, sizeof(struct jbm_drm_mode_get_encoder))
#define DRM_IOCTL_MODE_GETCONNECTOR JBM_DRM_IOWR(0xA7, sizeof(struct jbm_drm_mode_get_connector))
#define DRM_IOCTL_MODE_SETCRTC JBM_DRM_IOWR(0xA8, sizeof(struct jbm_drm_mode_crtc))
#define DRM_IOCTL_MODE_ADDFB JBM_DRM_IOWR(0xAE, sizeof(struct jbm_drm_mode_fb_cmd))
#define DRM_IOCTL_MODE_CREATE_DUMB JBM_DRM_IOWR(0xB2, sizeof(struct jbm_drm_mode_create_dumb))
#define DRM_IOCTL_MODE_MAP_DUMB JBM_DRM_IOWR(0xB3, sizeof(struct jbm_drm_mode_map_dumb))

struct gfx {
    int fd;
    u8 *mem;
    u8 *cv;
    u32 stride;
    u32 len;
    u32 w, h, bpp;
    u32 ro, rl, go, gl, bo, bl, to, tl;
    int ok;
    int rot;    /* fb_var.rotate (0..3 quarter turns), diagnostics + auto orientation */
    int is_drm; /* 1 when G.mem is a dumb KMS buffer instead of fb0 */
    u32 drm_crtc, drm_fb;
    struct jbm_drm_mode_modeinfo drm_mode;
};

static struct gfx G;

/* Scales an 8-bit colour component down to a framebuffer channel of `len` bits. */
static u32 chan_down(u32 v, u32 len) {
    if (len >= 8) return v;
    if (len == 0) return 0;
    return (v * (((u32)1 << len) - 1) + 127) / 255;
}

/* Expands a framebuffer channel of `len` bits up to 8 bits. */
static u32 chan_up(u32 v, u32 len) {
    if (len == 0) return 0;
    if (len >= 8) return v & 0xFF;
    u32 m = ((u32)1 << len) - 1;
    return (v & m) * 255 / m;
}

/* Packs an ARGB8888 value into the native framebuffer pixel layout. */
static u32 pack_px(u32 argb) {
    u32 v = (chan_down((argb >> 16) & 0xFF, G.rl) << G.ro) |
            (chan_down((argb >> 8) & 0xFF, G.gl) << G.go) |
            (chan_down(argb & 0xFF, G.bl) << G.bo);
    if (G.tl) v |= chan_down((argb >> 24) & 0xFF, G.tl) << G.to;
    return v;
}

/* Unpacks a native framebuffer pixel back to ARGB8888. */
static u32 unpack_px(u32 raw) {
    u32 a = G.tl ? chan_up((raw >> G.to), G.tl) : 0xFF;
    return (a << 24) | (chan_up((raw >> G.ro), G.rl) << 16) | (chan_up((raw >> G.go), G.gl) << 8) |
           chan_up((raw >> G.bo), G.bl);
}

/* Rejects channel descriptions that cannot describe a real pixel format. */
static int chan_ok(u32 off, u32 len, u32 bpp) {
    if (len == 0 || len > 8) return 0;
    if (off + len > bpp) return 0;
    return 1;
}

/* errno (positive) of the last gfx_open() failure, 0 if the failure was a
 * validation reject rather than a syscall error. */
static int gfx_err = 0;
/* Suppress per-attempt failure lines while the caller is polling gfx_open(). */
static int gfx_quiet = 0;

static int gfx_open(void) {
    static char nodes[4][40];
    int fd = -1;
    gfx_err = 0;
    for (int i = 0; i < 4; i++) {
        jbm_sn(nodes[i], "/dev/fb%d", i);
        fd = jbm_open(nodes[i], O_RDWR, 0);
        if (fd >= 0) break;
        if (fd < 0 && -fd > gfx_err) gfx_err = -fd;
    }
    if (fd < 0) {
        jbm_mkdir("/dev", 0755);
        jbm_mkdir("/dev/graphics", 0755);
        i64 mk = jbm_mknod("/dev/graphics/fb0", 0600 | S_IFCHR, (29 << 8) | 0);
        fd = jbm_open("/dev/graphics/fb0", O_RDWR, 0);
        if (fd < 0) {
            gfx_err = -fd;
            if (mk < 0) jbm_log("jbm: mknod /dev/graphics/fb0 failed (%d)", (int)-mk);
        }
    }
    if (fd < 0) {
        if (!gfx_quiet) jbm_log("jbm: no framebuffer node (err %d)", gfx_err);
        return -1;
    }

    struct jbm_fb_var var;
    struct jbm_fb_fix fix;
    memset(&var, 0, sizeof(var));
    memset(&fix, 0, sizeof(fix));

    i64 vr = jbm_ioctl(fd, FBIOGET_VSCREENINFO, &var);
    if (vr != 0 || var.xres < 16 || var.xres > 8192 || var.yres < 16 || var.yres > 8192) {
        gfx_err = vr < 0 ? (int)-vr : 0;
        if (!gfx_quiet)
            jbm_log("jbm: FBIOGET_VSCREENINFO unusable (err %d xres=%u yres=%u bpp=%u)", gfx_err, var.xres,
                    var.yres, var.bits_per_pixel);
        jbm_close(fd);
        return -1;
    }
    int have_fix = jbm_ioctl(fd, FBIOGET_FSCREENINFO, &fix) == 0;
    if (var.bits_per_pixel != 32 && var.bits_per_pixel != 16) {
        jbm_log("jbm: unsupported bpp %u", var.bits_per_pixel);
        jbm_close(fd);
        return -1;
    }

    u32 stride = (have_fix && fix.line_length) ? fix.line_length : var.xres * (var.bits_per_pixel / 8);
    u32 len = (have_fix && fix.smem_len) ? fix.smem_len : stride * var.yres_virtual;
    if (stride < var.xres * (var.bits_per_pixel / 8) || len < stride * var.yres) {
        jbm_log("jbm: framebuffer geometry inconsistent");
        jbm_close(fd);
        return -1;
    }

    if (!chan_ok(var.red_offset, var.red_length, var.bits_per_pixel) ||
        !chan_ok(var.green_offset, var.green_length, var.bits_per_pixel) ||
        !chan_ok(var.blue_offset, var.blue_length, var.bits_per_pixel) ||
        (var.transp_length && !chan_ok(var.transp_offset, var.transp_length, var.bits_per_pixel))) {
        jbm_log("jbm: unusable fb channel layout r=%u/%u g=%u/%u b=%u/%u", var.red_offset, var.red_length,
                var.green_offset, var.green_length, var.blue_offset, var.blue_length);
        jbm_close(fd);
        return -1;
    }

    u8 *mem = (u8 *)jbm_mmap(len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!mem) {
        gfx_err = jbm_mmap_err;
        if (!gfx_quiet) jbm_log("jbm: fb mmap failed (err %d len=%u)", gfx_err, len);
        jbm_close(fd);
        return -1;
    }
    u8 *cv = (u8 *)jbm_mmap(stride * var.yres, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!cv) {
        jbm_log("jbm: canvas alloc failed");
        jbm_close(fd);
        return -1;
    }

    G.fd = fd;
    G.mem = mem;
    G.cv = cv;
    G.stride = stride;
    G.len = len;
    G.w = var.xres;
    G.h = var.yres;
    G.bpp = var.bits_per_pixel;
    G.ro = var.red_offset;
    G.rl = var.red_length;
    G.go = var.green_offset;
    G.gl = var.green_length;
    G.bo = var.blue_offset;
    G.bl = var.blue_length;
    G.to = var.transp_offset;
    G.tl = var.transp_length;
    G.rot = (int)var.rotate;
    G.is_drm = 0;
    G.ok = 1;
    jbm_log("jbm: fb %ux%u bpp=%u stride=%u len=%u rgb=%u:%u/%u:%u/%u:%u a=%u:%u rot=%u", G.w, G.h, G.bpp,
            stride, len, G.rl, G.ro, G.gl, G.go, G.bl, G.bo, G.tl, G.to, var.rotate);
    return 0;
}

/* ------------------------------------------------------------------ */
/* DRM/KMS backend (fallback or preferred via prefer_drm)              */
/* ------------------------------------------------------------------ */

/* Creates /dev/dri/card0 from the sysfs dev number if it is missing. */
static int drm_node(void) {
    int fd = jbm_open("/dev/dri/card0", O_RDWR, 0);
    if (fd >= 0) return fd;
    char raw[64];
    if (jbm_read_text("/sys/class/drm/card0/dev", raw, sizeof(raw)) != 0) return -1;
    const char *colon = strchr(raw, ':');
    if (!colon) return -1;
    int maj = jbm_atoi_(raw), min = jbm_atoi_(colon + 1);
    jbm_mkdir("/dev", 0755);
    jbm_mkdir("/dev/dri", 0755);
    jbm_mknod("/dev/dri/card0", 0600 | S_IFCHR, (maj << 8) | min);
    return jbm_open("/dev/dri/card0", O_RDWR, 0);
}

static int drm_set_crtc(int fd, u32 crtc, u32 fb, u32 conn,
                        const struct jbm_drm_mode_modeinfo *mode) {
    struct jbm_drm_mode_crtc c;
    memset(&c, 0, sizeof(c));
    c.set_connectors_ptr = (u64)(unsigned long)&conn;
    c.count_connectors = 1;
    c.crtc_id = crtc;
    c.fb_id = fb;
    c.x = 0;
    c.y = 0;
    c.mode_valid = 1;
    if (mode) c.mode = *mode;
    return jbm_ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &c);
}

/* Opens DRM/KMS, programs the first connected connector and maps its dumb
 * buffer as the scanout. Returns 0 on success, -1 to let the fb path try. */
static int drm_open_init(void) {
    int fd = drm_node();
    if (fd < 0) {
        jbm_log("jbm: no DRM card node");
        return -1;
    }

    static u32 crtcs[16], conns[32], encs[16], fbs[16];
    struct jbm_drm_mode_card_res res;
    memset(&res, 0, sizeof(res));
    res.crtc_id_ptr = (u64)(unsigned long)crtcs;
    res.connector_id_ptr = (u64)(unsigned long)conns;
    res.encoder_id_ptr = (u64)(unsigned long)encs;
    res.fb_id_ptr = (u64)(unsigned long)fbs;
    res.count_crtcs = 16;
    res.count_connectors = 32;
    res.count_encoders = 16;
    res.count_fbs = 16;
    if (jbm_ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res) != 0 || res.count_connectors == 0) {
        jbm_log("jbm: DRM GETRESOURCES failed");
        jbm_close(fd);
        return -1;
    }
    jbm_log("jbm: DRM card0 crtcs=%u connectors=%u", res.count_crtcs, res.count_connectors);

    static struct jbm_drm_mode_modeinfo modes[64];
    u32 crtc = res.count_crtcs ? crtcs[0] : 0;
    u32 conn_id = 0;
    struct jbm_drm_mode_modeinfo mode;
    memset(&mode, 0, sizeof(mode));
    int found = 0;

    for (u32 i = 0; i < res.count_connectors && i < 32 && !found; i++) {
        struct jbm_drm_mode_get_connector c;
        memset(&c, 0, sizeof(c));
        c.connector_id = conns[i];
        c.modes_ptr = (u64)(unsigned long)modes;
        c.count_modes = 64;
        if (jbm_ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &c) != 0) continue;
        if (c.connection != 1 || c.count_modes == 0) continue; /* 1 = connected */

        conn_id = c.connector_id;
        mode = modes[0];
        found = 1;
        if (c.encoder_id) {
            struct jbm_drm_mode_get_encoder e;
            memset(&e, 0, sizeof(e));
            e.encoder_id = c.encoder_id;
            if (jbm_ioctl(fd, DRM_IOCTL_MODE_GETENCODER, &e) == 0 && e.crtc_id)
                crtc = e.crtc_id;
        }
        jbm_log("jbm: DRM connector %u mode %ux%u@%u crtc=%u", conn_id, mode.hdisplay, mode.vdisplay,
                mode.vrefresh, crtc);
    }
    if (!found) {
        jbm_log("jbm: DRM has no connected connector");
        jbm_close(fd);
        return -1;
    }

    struct jbm_drm_mode_create_dumb dumb;
    memset(&dumb, 0, sizeof(dumb));
    dumb.width = mode.hdisplay;
    dumb.height = mode.vdisplay;
    dumb.bpp = 32;
    if (jbm_ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb) != 0 || !dumb.handle) {
        jbm_log("jbm: DRM CREATE_DUMB failed");
        jbm_close(fd);
        return -1;
    }

    struct jbm_drm_mode_fb_cmd fb;
    memset(&fb, 0, sizeof(fb));
    fb.width = mode.hdisplay;
    fb.height = mode.vdisplay;
    fb.pitch = dumb.pitch;
    fb.bpp = 32;
    fb.depth = 24;
    fb.handle = dumb.handle;
    if (jbm_ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fb) != 0 || !fb.fb_id) {
        jbm_log("jbm: DRM ADDFB failed");
        jbm_close(fd);
        return -1;
    }

    if (drm_set_crtc(fd, crtc, fb.fb_id, conn_id, &mode) != 0) {
        jbm_log("jbm: DRM SETCRTC failed");
        jbm_close(fd);
        return -1;
    }

    struct jbm_drm_mode_map_dumb map;
    memset(&map, 0, sizeof(map));
    map.handle = dumb.handle;
    if (jbm_ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
        jbm_log("jbm: DRM MAP_DUMB failed");
        jbm_close(fd);
        return -1;
    }
    u8 *mem = (u8 *)jbm_mmap((u32)dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (long)map.offset);
    if (!mem) {
        jbm_log("jbm: DRM mmap failed (err %d)", jbm_mmap_err);
        jbm_close(fd);
        return -1;
    }
    u8 *cv = (u8 *)jbm_mmap(dumb.pitch * mode.vdisplay, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!cv) {
        jbm_close(fd);
        return -1;
    }

    G.fd = fd;
    G.mem = mem;
    G.cv = cv;
    G.stride = dumb.pitch;
    G.len = (u32)dumb.size;
    G.w = mode.hdisplay;
    G.h = mode.vdisplay;
    G.bpp = 32;
    G.ro = 16;
    G.rl = 8;
    G.go = 8;
    G.gl = 8;
    G.bo = 0;
    G.bl = 8;
    G.to = 0;
    G.tl = 0;
    G.rot = 0;
    G.is_drm = 1;
    G.drm_crtc = crtc;
    G.drm_fb = fb.fb_id;
    G.drm_mode = mode;
    G.ok = 1;
    jbm_log("jbm: DRM %ux%u pitch=%u crtc=%u fb=%u", G.w, G.h, G.stride, crtc, fb.fb_id);
    return 0;
}

/* mtkfb keeps scanning whatever LK left programmed in its layer: writing the
 * framebuffer is not enough, the driver has to re-apply the mode for the layer
 * to pick up the new contents. Verified on this device: FBIOPUT_VSCREENINFO with
 * FB_ACTIVATE_FORCE is what makes fb0 visible (without it the boot logo stays
 * frozen on screen while the menu draws into memory nobody reads). */

/* Cheap re-apply: one ioctl pair, no blank/unblank and no sleep. Every screen
 * change has to go through this, otherwise a memcpy into the scanout buffer can
 * stay invisible and the panel keeps showing the previous screen forever. */
static void gfx_reapply(void) {
    if (!G.ok) return;
    if (G.is_drm) {
        drm_set_crtc(G.fd, G.drm_crtc, G.drm_fb, 0, &G.drm_mode);
        return;
    }
    struct jbm_fb_var var;
    memset(&var, 0, sizeof(var));
    i64 g = jbm_ioctl(G.fd, FBIOGET_VSCREENINFO, &var);
    var.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_ALL;
    i64 p = jbm_ioctl(G.fd, FBIOPUT_VSCREENINFO, &var);
    static int n = 0;
    if (++n <= 4 || (n % 60) == 0)
        jbm_log("jbm: reapply #%d get=%d put=%d", n, (int)g, (int)p);
}

static void gfx_kick(void) {
    if (!G.ok) return;
    if (G.is_drm) {
        drm_set_crtc(G.fd, G.drm_crtc, G.drm_fb, 0, &G.drm_mode);
        jbm_log("jbm: drm kick (setcrtc)");
        return;
    }
    struct jbm_fb_var var;
    memset(&var, 0, sizeof(var));
    i64 g = jbm_ioctl(G.fd, FBIOGET_VSCREENINFO, &var);
    var.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_ALL;
    i64 p = jbm_ioctl(G.fd, FBIOPUT_VSCREENINFO, &var);
    i64 b1 = jbm_ioctl(G.fd, FBIOBLANK, (void *)1);
    jbm_sleep_ms(250);
    i64 b0 = jbm_ioctl(G.fd, FBIOBLANK, (void *)0);
    jbm_log("jbm: gfx kick get=%d put=%d blank1=%d blank0=%d", (int)g, (int)p, (int)b1, (int)b0);
}

/* Automatic display detection:
 *   prefer_drm=1 -> KMS first, framebuffer as fallback when fallback_to_fb=1;
 *   otherwise    -> framebuffer first (polled while the driver registers a new
 *                   node), then KMS as a last resort.
 * Returns 0 when a backend is ready. */
static int gfx_detect(void) {
    if (JBM_CFG.prefer_drm) {
        jbm_log("jbm: prefer_drm set, probing KMS first");
        if (drm_open_init() == 0) return 0;
        if (!JBM_CFG.fallback_to_fb) {
            jbm_log("jbm: KMS failed and fallback_to_fb=0");
            return -1;
        }
    }

    int fb = 0;
    int last_err = -1;
    i64 t_first = jbm_now_ms();
    for (int i = 0; i < 400; i++) {
        gfx_quiet = (i != 0);
        if (gfx_open() == 0) {
            fb = 1;
            jbm_log("jbm: fb ready after %lld ms (attempt %d)", (long long)(jbm_now_ms() - t_first), i);
            break;
        }
        if (gfx_err != last_err) {
            jbm_log("jbm: fb attempt %d failed (err %d)", i, gfx_err);
            last_err = gfx_err;
        } else if (i % 40 == 39) {
            jbm_log("jbm: fb still unavailable after %lld ms (err %d)",
                    (long long)(jbm_now_ms() - t_first), gfx_err);
        }
        jbm_sleep_ms(50);
    }
    gfx_quiet = 0;
    if (fb) return 0;

    jbm_log("jbm: framebuffer unavailable after %lld ms, probing KMS",
            (long long)(jbm_now_ms() - t_first));
    if (drm_open_init() == 0) return 0;

    jbm_log("jbm: no display backend available");
    return -1;
}

/* Effective screen rotation in degrees; orientation=-1 means "follow the
 * rotation the framebuffer driver reports". */
static int jbm_orient_deg(void) {
    int o = JBM_CFG.orientation;
    if (o < 0) o = (G.rot & 3) * 90;
    o %= 360;
    if (o < 0) o += 360;
    return o;
}

/* Drawing colours are ARGB (0xAARRGGBB); these literals are written RGBA
 * (0xRRGGBBAA) for readability, so convert them once. */
#define RGBA(v) ((u32)((((u32)(v) & 0xFFu) << 24) | ((u32)(v) >> 8)))

static inline void store_px(int x, int y, u32 argb) {
    u8 *row = G.cv + (u32)y * G.stride;
    if (G.bpp == 16) {
        *(u16 *)(row + (u32)x * 2) = (u16)pack_px(argb);
    } else {
        *(u32 *)(row + (u32)x * 4) = pack_px(argb);
    }
}

static inline u32 load_px(int x, int y) {
    u8 *row = G.cv + (u32)y * G.stride;
    u32 raw = G.bpp == 16 ? (u32)*(u16 *)(row + (u32)x * 2) : *(u32 *)(row + (u32)x * 4);
    return unpack_px(raw);
}

/* Alpha-composites argb onto the canvas. Returns the resulting ARGB value. */
static inline u32 px(int x, int y, u32 argb) {
    if ((u32)x >= G.w || (u32)y >= G.h) return 0;
    u32 a = (argb >> 24) & 0xFF;
    if (!a) return load_px(x, y);
    if (a == 255) {
        store_px(x, y, argb);
        return argb;
    }
    u32 d = load_px(x, y);
    u32 da = d >> 24;
    u32 out_a = a + da * (255 - a) / 255;
    if (!out_a) {
        store_px(x, y, 0);
        return 0;
    }
    u32 k = (255 - a) * da / 255;
    u32 r = (((argb >> 16) & 0xFF) * a + ((d >> 16) & 0xFF) * k) / out_a;
    u32 g = (((argb >> 8) & 0xFF) * a + ((d >> 8) & 0xFF) * k) / out_a;
    u32 b = ((argb & 0xFF) * a + (d & 0xFF) * k) / out_a;
    u32 out = (out_a << 24) | (r << 16) | (g << 8) | b;
    store_px(x, y, out);
    return out;
}

static void blend(int x, int y, u32 argb) { px(x, y, argb); }

static void fill_rect(int x, int y, int w, int h, u32 argb) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)G.w) x1 = (int)G.w;
    if (y1 > (int)G.h) y1 = (int)G.h;
    if (x1 <= x0 || y1 <= y0) return;

    if ((argb >> 24) == 255) {
        u32 v = pack_px(argb);
        if (G.bpp == 16) {
            for (int yy = y0; yy < y1; yy++) {
                u16 *p = (u16 *)(G.cv + (u32)yy * G.stride) + x0;
                for (int xx = x0; xx < x1; xx++) *p++ = (u16)v;
            }
        } else {
            for (int yy = y0; yy < y1; yy++) {
                u32 *p = (u32 *)(G.cv + (u32)yy * G.stride) + x0;
                for (int xx = x0; xx < x1; xx++) *p++ = v;
            }
        }
        return;
    }
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) blend(xx, yy, argb);
}

static int rr_inset(int r, int row) {
    int d = r - 1 - row;
    int half = jbm_isqrt(r * r - d * d);
    int inset = r - half;
    return inset < 0 ? 0 : inset;
}

static void round_rect_border(int x, int y, int w, int h, int r, int th, u32 argb) {
    if (w <= 0 || h <= 0 || th <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    int ri = r - th;
    if (ri < 0) ri = 0;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int o = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        if (row < th || row >= h - th) {
            fill_rect(x + o, y + row, w - 2 * o, 1, argb);
            continue;
        }
        int ei = row - th < h - 1 - th - row ? row - th : h - 1 - th - row;
        int ii = th + ((ri > 0 && ei < ri) ? rr_inset(ri, ei) : 0);
        if (ii <= o) ii = o + 1;
        fill_rect(x + o, y + row, ii - o, 1, argb);
        fill_rect(x + w - ii, y + row, ii - o, 1, argb);
    }
}

static void disc(int cx, int cy, int r, u32 c) {
    if (r <= 0) return;
    for (int dy = -r; dy <= r; dy++) {
        int s = jbm_isqrt(r * r - dy * dy);
        fill_rect(cx - s, cy + dy, s * 2 + 1, 1, c);
    }
}

static void ring(int cx, int cy, int r, int th, u32 c) {
    if (th <= 0 || r <= 0) return;
    int ri = r - th;
    if (ri < 0) ri = 0;
    for (int dy = -r; dy <= r; dy++) {
        int s = jbm_isqrt(r * r - dy * dy);
        if (dy * dy < ri * ri) {
            int s2 = jbm_isqrt(ri * ri - dy * dy);
            fill_rect(cx - s, cy + dy, s - s2, 1, c);
            fill_rect(cx + s2 + 1, cy + dy, s - s2, 1, c);
        } else {
            fill_rect(cx - s, cy + dy, s * 2 + 1, 1, c);
        }
    }
}

static float jbm_sinf(float a) {
    const float pi = 3.14159265358979f;
    while (a > pi) a -= 2.0f * pi;
    while (a < -pi) a += 2.0f * pi;
    float x = a;
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f)));
}

static void arc(int cx, int cy, int r, int th, int a0, int a1, u32 c) {
    if (a1 <= a0 || r <= 0) return;
    int steps = ((a1 - a0) * r) / 40 + 8;
    for (int i = 0; i <= steps; i++) {
        float rad = 3.14159265358979f * (float)(a0 + (a1 - a0) * i / steps) / 180.0f;
        int dx = cx + (int)(jbm_sinf(rad + 1.5707963f) * (float)r);
        int dy = cy - (int)(jbm_sinf(rad) * (float)r);
        disc(dx, dy, th / 2 < 1 ? 1 : th / 2, c);
    }
}

/* ------------------------------------------------------------------ */
/* text                                                                */
/* ------------------------------------------------------------------ */

struct font {
    const char *data;
    const short (*meta)[7];
    int aw, ah, px, top, bot, first, count;
};

static struct font FN20 = {F20_data, F20_meta, F20_ATLAS_W, F20_ATLAS_H, F20_PX,
                           F20_TOP,   F20_BOT,   F20_FIRST, F20_COUNT};
static struct font FN32 = {F34_data, F34_meta, F34_ATLAS_W, F34_ATLAS_H, F34_PX,
                           F34_TOP,   F34_BOT,   F34_FIRST, F34_COUNT};
static struct font FN48 = {F48_data, F48_meta, F48_ATLAS_W, F48_ATLAS_H, F48_PX,
                           F48_TOP,   F48_BOT,   F48_FIRST, F48_COUNT};

#define TA_L 0
#define TA_C 1
#define TA_R 2
#define VA_T 0
#define VA_M 4
#define VA_B 8

static float glyph_adv(struct font *f, int idx, float k) { return (float)f->meta[idx][6] * k; }

static float scale_of(struct font *f, float px) { return px / (float)f->px; }

/* Must mirror draw_text_ls() exactly (per-glyph rounding, ls between chars),
 * otherwise sequential layouts drift from the drawn ink. */
static int measure(struct font *f, const char *s, float px, int ls) {
    float k = scale_of(f, px);
    int w = 0;
    int n = 0;
    for (const char *p = s; *p; p++) {
        int i = (int)(u8)*p - f->first;
        n++;
        if (i < 0 || i >= f->count) continue;
        w += (int)(glyph_adv(f, i, k) + 0.5f);
    }
    if (n > 1) w += ls * (n - 1);
    return w;
}

static int line_h(float px) { return (int)(px * 1.32f + 0.5f); }

static void draw_text_ls(struct font *f, int x, int y, float px, int ls, u32 argb, const char *s,
                         int align) {
    if (!f->count) return;
    float k = scale_of(f, px);
    int lh = line_h(px);
    if (align & TA_C) x -= measure(f, s, px, ls) / 2;
    else if (align & TA_R) x -= measure(f, s, px, ls);
    if (align & VA_M) y -= lh / 2;
    else if (align & VA_B) y -= lh;

    u32 alpha = argb >> 24;
    u32 rgb = argb & 0xFFFFFF;

    for (const char *p = s; *p; p++) {
        int idx = (int)(u8)*p - f->first;
        if (idx < 0 || idx >= f->count) continue;
        const short *m = f->meta[idx];
        int gw = m[2], gh = m[3];
        if (gw > 0 && gh > 0) {
            int sx = m[0], sy = m[1];
            float dw = (float)gw * k, dh = (float)gh * k;
            int dw2 = (int)(dw + 0.5f), dh2 = (int)(dh + 0.5f);
            if (dw2 < 1) dw2 = 1;
            if (dh2 < 1) dh2 = 1;
            int ox = x + (int)((float)m[4] * k);
            int oy = y + (int)((float)m[5] * k);
            float stepx = (float)gw / (float)dw2, stepy = (float)gh / (float)dh2;
            for (int yy = 0; yy < dh2; yy++) {
                float fy = ((float)yy + 0.5f) * stepy - 0.5f;
                int iy = (int)fy;
                float fy2 = fy - (float)iy;
                for (int xx = 0; xx < dw2; xx++) {
                    float fx = ((float)xx + 0.5f) * stepx - 0.5f;
                    int ix = (int)fx;
                    float fx2 = fx - (float)ix;
                    int ax0 = ix < 0 ? 0 : ix, ax1 = (ix + 1 >= gw) ? gw - 1 : ix + 1;
                    int ay0 = iy < 0 ? 0 : iy, ay1 = (iy + 1 >= gh) ? gh - 1 : iy + 1;
                    float v00 = (float)(u8)f->data[(sy + ay0) * f->aw + (sx + ax0)];
                    float v10 = (float)(u8)f->data[(sy + ay0) * f->aw + (sx + ax1)];
                    float v01 = (float)(u8)f->data[(sy + ay1) * f->aw + (sx + ax0)];
                    float v11 = (float)(u8)f->data[(sy + ay1) * f->aw + (sx + ax1)];
                    float a = v00 + (v10 - v00) * fx2;
                    float b = v01 + (v11 - v01) * fx2;
                    a = a + (b - a) * fy2;
                    if (a < 1.0f) continue;
                    u32 na = (u32)(a * (float)alpha / 255.0f + 0.5f);
                    if (!na) continue;
                    blend(ox + xx, oy + yy, (na << 24) | rgb);
                }
            }
        }
        x += (int)(glyph_adv(f, idx, k) + 0.5f) + ls;
    }
}

/* Uppercases src into dst (dst must hold strlen+1); the font faces are ASCII. */
static void up_str(char *dst, const char *src, int n) {
    int i = 0;
    for (; src[i] && i < n - 1; i++) {
        char c = src[i];
        dst[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }
    dst[i] = 0;
}

/* ------------------------------------------------------------------ */
/* touch input                                                         */
/* ------------------------------------------------------------------ */

struct touch {
    int fd;
    int slot, active, ranged, mt;
    i32 rx, ry, x0, x1, y0, y1;
    int sx, sy;
    int down_x, down_y;
    i64 down_t;
    int holding, pressed, released;
    int tap_x, tap_y;
};

static struct touch T = {.fd = -1};

static int touch_open(void) {
    static char ents[32][64];
    int count = 0;
    #ifdef JBM_HOST
    const char *dir = "/dev/input";
#else
    const char *dir = "/sys/class/input";
#endif
    {
        int fd = jbm_open(dir, O_RDONLY, 0);
        if (fd >= 0) {
            static u8 buf[8192];
            for (;;) {
                long r = jbm_getdents(fd, buf, (u32)sizeof(buf));
                if (r <= 0) break;
                long off = 0;
                while (off < r && count < 32) {
                    struct jbm_dirent64 *d = (struct jbm_dirent64 *)(buf + off);
                    off += d->d_reclen;
                    if (d->d_name[0] == '.') continue;
                    jbm_strcpy_(ents[count++], d->d_name, 64);
                }
            }
            jbm_close(fd);
        }
    }

    if (!count) {
        jbm_log("jbm: no input devices under %s", dir);
        return 0;
    }

    int best_fd = -1;
    int best_score = -1;
    int best_mt = 0;
    char best_nm[64];
    struct jbm_absinfo bax, bay;
    memset(best_nm, 0, sizeof(best_nm));

    for (int i = 0; i < count; i++) {
        if (!jbm_str_has(ents[i], "event")) continue;
        char node[80];
#ifdef JBM_HOST
        jbm_sn(node, "%s/%s", dir, ents[i]);
#else
        char sysp[128], raw[64];
        jbm_sn(sysp, "/sys/class/input/%s/dev", ents[i]);
        if (jbm_read_file(sysp, raw, sizeof(raw)) <= 0) continue;
        const char *colon = raw;
        while (*colon && *colon != ':') colon++;
        if (!*colon) continue;
        int maj = jbm_atoi_(raw), min = jbm_atoi_(colon + 1);

        jbm_mkdir("/dev", 0755);
        jbm_mkdir("/dev/input", 0755);
        jbm_sn(node, "/dev/input/%s", ents[i]);
        jbm_mknod(node, 0600 | S_IFCHR, (maj << 8) | min);
#endif

        int fd = jbm_open(node, O_RDONLY | O_NONBLOCK, 0);
        if (fd < 0) continue;

        char nm[64];
        memset(nm, 0, sizeof(nm));
        struct jbm_absinfo ax, ay;
        jbm_ioctl(fd, EVIOCGNAME(sizeof(nm)), nm);
        int mt = 1;
        int okx = jbm_ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &ax) == 0;
        int oky = jbm_ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &ay) == 0;
        if (!okx) {
            okx = jbm_ioctl(fd, EVIOCGABS(ABS_X), &ax) == 0;
            mt = 0;
        }
        if (!oky) {
            oky = jbm_ioctl(fd, EVIOCGABS(ABS_Y), &ay) == 0;
        }
        if (!okx || !oky) {
            jbm_close(fd);
            continue;
        }

        /* A collapsed axis range cannot be mapped to screen coordinates, so
         * devices like sec_touchproximity (0..0) are never usable. */
        if ((i64)ax.maximum - (i64)ax.minimum <= 0 || (i64)ay.maximum - (i64)ay.minimum <= 0) {
            jbm_close(fd);
            continue;
        }

        /* Score every candidate instead of taking the first "touch" match:
         * getdents order is arbitrary and would otherwise pick e.g. the
         * proximity sensor over the real touchscreen. */
        int score = 1;
        if (jbm_str_has(nm, "touchscreen")) score += 100;
        else if (jbm_str_has(nm, "touch")) score += 40;
        if (jbm_str_has(nm, "proximity")) score -= 60;
        if (mt) score += 10;

        /* input_device pins the node: either an exact /dev path or a substring
         * of the device name. Anything that does not match is dropped. */
        if (JBM_CFG.input_device[0]) {
            int match = (JBM_CFG.input_device[0] == '/') ? (strcmp(node, JBM_CFG.input_device) == 0)
                                                        : jbm_str_has(nm, JBM_CFG.input_device);
            if (!match) {
                jbm_close(fd);
                continue;
            }
            score += 1000;
        }

        if (score > best_score) {
            if (best_fd >= 0) jbm_close(best_fd);
            best_fd = fd;
            best_score = score;
            best_mt = mt;
            memcpy(best_nm, nm, sizeof(best_nm));
            bax = ax;
            bay = ay;
        } else {
            jbm_close(fd);
        }
    }

    if (best_fd >= 0) {
        T.fd = best_fd;
        T.mt = best_mt;
        T.ranged = 1;
        T.x0 = bax.minimum;
        T.x1 = bax.maximum;
        T.y0 = bay.minimum;
        T.y1 = bay.maximum;
        /* config.ini can override the raw ranges for panels whose ABS axes do
         * not span the full digitizer. */
        if (JBM_CFG.touch_cal_x0 >= 0 && JBM_CFG.touch_cal_x1 > JBM_CFG.touch_cal_x0) {
            T.x0 = JBM_CFG.touch_cal_x0;
            T.x1 = JBM_CFG.touch_cal_x1;
        }
        if (JBM_CFG.touch_cal_y0 >= 0 && JBM_CFG.touch_cal_y1 > JBM_CFG.touch_cal_y0) {
            T.y0 = JBM_CFG.touch_cal_y0;
            T.y1 = JBM_CFG.touch_cal_y1;
        }
        T.sx = (int)G.w / 2;
        T.sy = (int)G.h / 2;
        jbm_log("jbm: touch=%s x[%d..%d] y[%d..%d] %s score=%d orient=%d", best_nm[0] ? best_nm : "(noname)",
                T.x0, T.x1, T.y0, T.y1, T.mt ? "mt" : "abs", best_score, jbm_orient_deg());
        return 1;
    }
    jbm_log("jbm: no usable touchscreen (event nodes with ABS axes)");
    return 0;
}

/* Maps the raw digitizer point to screen coordinates and applies the configured
 * rotation: 90 = content turned clockwise, so the touch vector is rotated with
 * it. Screen space is X right / Y down. */
static void touch_map(void) {
    i64 rx = (i64)T.x1 - T.x0, ry = (i64)T.y1 - T.y0;
    if (rx <= 0 || ry <= 0) return;
    i64 ax = T.rx, ay = T.ry;
    if (ax < T.x0) ax = T.x0;
    if (ax > T.x1) ax = T.x1;
    if (ay < T.y0) ay = T.y0;
    if (ay > T.y1) ay = T.y1;

    /* Normalised 0..65535 so the rotation does not lose precision. */
    i64 u = ((ax - T.x0) * 65535) / rx;
    i64 v = ((ay - T.y0) * 65535) / ry;
    switch (jbm_orient_deg()) {
        case 90: { i64 nu = v, nv = 65535 - u; u = nu; v = nv; break; }
        case 180: u = 65535 - u; v = 65535 - v; break;
        case 270: { i64 nu = 65535 - v, nv = u; u = nu; v = nv; break; }
        default: break;
    }
    T.sx = (int)((u * ((i64)G.w - 1)) / 65535);
    T.sy = (int)((v * ((i64)G.h - 1)) / 65535);
}

static void touch_poll(void) {
    T.pressed = 0;
    T.released = 0;
    if (T.fd < 0) return;
    struct jbm_pollfd p;
    p.fd = T.fd;
    p.events = POLLIN;
    p.revents = 0;
    if (jbm_pollfd_ts(&p, 1, 0) <= 0) return;

    struct jbm_input_event e;
    for (;;) {
        if (jbm_read(T.fd, &e, (u32)sizeof(e)) != (i64)sizeof(e)) break;
        switch (e.type) {
            case EV_SYN:
                break;
            case EV_ABS:
                if (e.code == ABS_MT_SLOT) {
                    T.slot = e.value;
                } else if (e.code == ABS_MT_TRACKING_ID) {
                    if (e.value < 0) {
                        if (T.holding) {
                            T.holding = 0;
                            T.released = 1;
                            T.tap_x = T.sx;
                            T.tap_y = T.sy;
                        }
                    } else {
                        T.active = T.slot;
                        if (!T.holding) {
                            T.holding = 1;
                            T.pressed = 1;
                            T.down_x = T.sx;
                            T.down_y = T.sy;
                            T.down_t = jbm_now_ms();
                        }
                    }
                } else if (e.code == ABS_MT_POSITION_X || e.code == ABS_X) {
                    T.rx = e.value;
                    touch_map();
                } else if (e.code == ABS_MT_POSITION_Y || e.code == ABS_Y) {
                    T.ry = e.value;
                    touch_map();
                }
                break;
            case EV_KEY:
                if (e.code == BTN_TOUCH) {
                    if (e.value == 0) {
                        if (T.holding) {
                            T.holding = 0;
                            T.released = 1;
                            T.tap_x = T.sx;
                            T.tap_y = T.sy;
                        }
                    } else if (!T.holding) {
                        T.holding = 1;
                        T.pressed = 1;
                        T.down_x = T.sx;
                        T.down_y = T.sy;
                        T.down_t = jbm_now_ms();
                    }
                }
                break;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* actions (menu entries)                                              */
/* ------------------------------------------------------------------ */

/* Legacy action order, still the order of the built-in entries. */
enum { ACT_SYSTEM, ACT_RECOVERY, ACT_FASTBOOT, ACT_DOWNLOAD, ACT_REBOOT, ACT_POWEROFF };

static int act_n(void) { return JBM_NUM_ENTRIES; }

static jbm_entry_type_t act_type(int i) {
    if (i < 0 || i >= JBM_NUM_ENTRIES) return JBM_ENTRY_TYPE_SYSTEM;
    return JBM_ENTRIES[i].type;
}

static const char *act_name(int i) {
    if (i < 0 || i >= JBM_NUM_ENTRIES) return "?";
    return JBM_ENTRIES[i].name[0] ? JBM_ENTRIES[i].name : "?";
}

#ifdef JBM_HOST
/* Used by the host preview to pick built-in entries by type. */
static int jbm_find_type(jbm_entry_type_t t) {
    for (int i = 0; i < JBM_NUM_ENTRIES; i++)
        if (JBM_ENTRIES[i].type == t) return i;
    return 0;
}
#endif

static void enter_download_mode(void) {
    /* On this platform there is no known userspace trigger for the MTK download
     * (ODIN) mode. The Download action reboots with the reason "download" via
     * LINUX_REBOOT_CMD_RESTART2; whether the bootloader acts on that reason has
     * not been validated on hardware. */
    jbm_log("jbm: requesting download mode (reboot reason 'download')");
}

static void act_system(char **argv, char **envp, const char *why);

/* Runs a user entry from /cache/Jupiter. Absolute paths only, no traversal;
 * the child execve()s the program directly (no shell), so the arguments are
 * passed verbatim. Returns 0 to go back to the menu. */
static int jbm_run_entry(int idx, char **argv, char **envp) {
    jbm_entry_t *e = &JBM_ENTRIES[idx];
    if (e->path[0] != '/' || jbm_str_has(e->path, "..")) {
        jbm_log("jbm: refusing entry '%s': path must be absolute and free of '..'", e->path);
        return 0;
    }
    int fd = jbm_open(e->path, O_RDONLY, 0);
    if (fd < 0) {
        jbm_log("jbm: entry '%s' not readable (err %d)", e->path, -fd);
        return 0;
    }
    jbm_close(fd);
    jbm_fchmod(e->path, 0755); /* adb push does not set +x */

    static char avbuf[JBM_MAX_ENTRY_ARGS];
    static char *av[16];
    jbm_strcpy_(avbuf, e->args, sizeof(avbuf));
    int ac = 0;
    av[ac++] = e->path;
    char *p = avbuf;
    while (*p && ac < 15) {
        while (*p == ' ') p++;
        if (!*p) break;
        av[ac++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = 0;
    }
    av[ac] = 0;

    jbm_log("jbm: running %s entry '%s' (%d args)", e->type == JBM_ENTRY_TYPE_SCRIPT ? "script" : "binary",
            e->path, ac - 1);
    jbm_sync_all();

    long pid = jbm_fork();
    if (pid == 0) {
#ifdef JBM_HOST
        execve(e->path, av, envp);
        _exit(127);
#else
        sc6(SYS_execve, (long)e->path, (long)av, (long)envp, 0, 0, 0);
        /* If execve returned the program was not executable (e.g. a script
         * whose interpreter is missing); say so via the exit status. */
        sc1(SYS_exit_group, 127);
        for (;;) {}
#endif
    }
    if (pid < 0) {
        jbm_log("jbm: fork failed (%d)", (int)pid);
        return 0;
    }
    int st = 0;
    int rc = jbm_wait(pid, &st);
    jbm_log("jbm: '%s' finished rc=%d status=0x%x", e->path, rc, st);

    if (e->behavior_after == 1) act_system(argv, envp, "entry behavior_after=chain");
    if (e->behavior_after == 2) {
        jbm_sync_all();
        jbm_sleep_ms(120);
        jbm_reboot_cmd(JB_RB_RESTART, 0);
        for (;;) jbm_sleep_ms(500);
    }
    return 0;
}

/* Runs one menu entry. Reboot/power actions do not return; script/binary
 * entries return 0 so the main loop can come back to the menu. */
static int perform(int idx, char **argv, char **envp) {
    if (idx < 0 || idx >= JBM_NUM_ENTRIES) return 0;
    switch (act_type(idx)) {
        case JBM_ENTRY_TYPE_SYSTEM:
            act_system(argv, envp, "user selected System");
            return 1;
        case JBM_ENTRY_TYPE_RECOVERY:
            jbm_log("jbm: rebooting with reason 'recovery' (no flash write)");
            jbm_sync_all();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RECOVERY, 0);
            break;
        case JBM_ENTRY_TYPE_FASTBOOT:
            jbm_log("jbm: rebooting to bootloader (fastboot)");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_BOOTLOADER, 0);
            break;
        case JBM_ENTRY_TYPE_DOWNLOAD:
            enter_download_mode();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_DOWNLOAD, 0);
            break;
        case JBM_ENTRY_TYPE_REBOOT:
            jbm_log("jbm: rebooting to system");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RESTART, 0);
            break;
        case JBM_ENTRY_TYPE_POWEROFF:
            jbm_reboot_cmd(JB_RB_POWER_OFF, 0);
            break;
        case JBM_ENTRY_TYPE_SCRIPT:
        case JBM_ENTRY_TYPE_BINARY:
            return jbm_run_entry(idx, argv, envp);
    }
    /* Reboot/power commands normally do not return; if the syscall bounced,
     * keep the action screen up rather than spinning back into the menu. */
    for (;;) jbm_sleep_ms(500);
    return 1;
}

/* ------------------------------------------------------------------ */
/* ui                                                                  */
/* ------------------------------------------------------------------ */

struct ui {
    int sel;
    int scroll;    /* first visible entry row (long entry lists scroll) */
    int mode;      /* UI_MENU draws the menu, UI_BOOT/UI_BUSY the action screen */
    int act;       /* which action the action screen is announcing */
    i64 boot_t0;
    i64 t0;
    i64 countdown_ms;
    int countdown_on;
};

static struct ui U;

#define UI_MENU 0
#define UI_BOOT 1
#define UI_BUSY 2

/* Set whenever the screen changed wholesale, so the next loop iteration draws
 * it immediately instead of waiting for the next 33 ms frame slot. */
static int ui_dirty = 1;

#define BG_MENU 0
#define BG_BOOT 1

static void present(void);

static i64 elapsed_ms(void) { return jbm_now_ms() - U.t0; }

/* The layout below is authored for the 720x1600 panel (SM-A225M). */
static float dk(void) {
    float a = (float)G.w / 720.0f;
    float b = (float)G.h / 1600.0f;
    return a < b ? a : b;
}

static u32 mix_argb(u32 a1, u32 a2, u32 t) {
    u32 r = (((a1 >> 16) & 0xFF) * (255 - t) + ((a2 >> 16) & 0xFF) * t) / 255;
    u32 g = (((a1 >> 8) & 0xFF) * (255 - t) + ((a2 >> 8) & 0xFF) * t) / 255;
    u32 b = ((a1 & 0xFF) * (255 - t) + (a2 & 0xFF) * t) / 255;
    u32 al = (((a1 >> 24) & 0xFF) * (255 - t) + ((a2 >> 24) & 0xFF) * t) / 255;
    return (al << 24) | (r << 16) | (g << 8) | b;
}

/* CSS linear-gradient(90deg, c1, c2) inside a rounded rect. */
static void grad_rect(int x, int y, int w, int h, int r, u32 c1, u32 c2) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int in = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        int x0 = x + in, x1 = x + w - in;
        if (x0 < 0) x0 = 0;
        if (x1 > (int)G.w) x1 = (int)G.w;
        int yy = y + row;
        if (yy < 0 || yy >= (int)G.h) continue;
        for (int i = x0; i < x1; i++)
            blend(i, yy, mix_argb(c1, c2, (u32)((i - x) * 255 / (w > 1 ? w - 1 : 1))));
    }
}

/* CSS radial-gradient(circle, color, transparent 100%) inside a circle of r. */
static void radial_glow(int cx, int cy, int r, u32 rgb, int amax) {
    if (r <= 0 || amax <= 0) return;
    for (int dy = -r; dy <= r; dy++) {
        int yy = cy + dy;
        if (yy < 0 || yy >= (int)G.h) continue;
        int half = jbm_isqrt(r * r - dy * dy);
        int x0 = cx - half, x1 = cx + half;
        if (x0 < 0) x0 = 0;
        if (x1 > (int)G.w - 1) x1 = (int)G.w - 1;
        for (int xx = x0; xx <= x1; xx++) {
            int dx = xx - cx;
            int d = jbm_isqrt(dx * dx + dy * dy);
            int t = 255 - (int)((i64)d * 255 / r);
            if (t < 0) t = 0;
            u32 a = (u32)((i64)amax * t / 255);
            if (a) blend(xx, yy, (a << 24) | rgb);
        }
    }
}

/* Distance from (cx,cy) to the farthest screen corner (CSS "farthest-corner"). */
static int farthest_corner(int cx, int cy) {
    int dx = cx > (int)G.w - 1 - cx ? cx : (int)G.w - 1 - cx;
    int dy = cy > (int)G.h - 1 - cy ? cy : (int)G.h - 1 - cy;
    return jbm_isqrt(dx * dx + dy * dy);
}

/* Thin rotated ellipse outline (the ring around the brand planet icon). */
static void ellipse_ring(int cx, int cy, int rx, int ry, int rot, int th, u32 c) {
    int steps = 72;
    int d = th / 2 < 1 ? 1 : th / 2;
    float rr = (float)rot * 3.14159265f / 180.0f;
    float cr = jbm_sinf(rr + 1.5707963f), sr = jbm_sinf(rr);
    for (int i = 0; i < steps; i++) {
        float t = 6.2831853f * (float)i / (float)steps;
        float px = (float)rx * jbm_sinf(t + 1.5707963f);
        float py = (float)ry * jbm_sinf(t);
        int dx = cx + (int)(px * cr - py * sr);
        int dy = cy + (int)(px * sr + py * cr);
        disc(dx, dy, d, c);
    }
}

/* 8-bit palette image with a 1-bit mask, blitted at (X,Y). */
static void blit_img(const u32 *pal, const u8 *idx, const u8 *mask, int X, int Y, int W, int H) {
    for (int y = 0; y < H; y++) {
        int dy = Y + y;
        if (dy < 0 || dy >= (int)G.h) continue;
        for (int x = 0; x < W; x++) {
            int dx = X + x;
            if (dx < 0 || dx >= (int)G.w) continue;
            u32 i = (u32)y * (u32)W + (u32)x;
            if (!((mask[i >> 3] >> (7 - (i & 7))) & 1)) continue;
            blend(dx, dy, pal[idx[i]]);
        }
    }
}

/* One 2px star per background tile, exactly like the mocks' .space::before. */
static void star_field(int stepx, int stepy, int px, int py, int alpha) {
    for (int y = py + stepy / 2; y < (int)G.h; y += stepy)
        for (int x = px + stepx / 2; x < (int)G.w; x += stepx)
            fill_rect(x, y, 2, 2, ((u32)alpha << 24) | 0xFFFFFF);
}

/* Nearest-neighbour blit of the configured background.bmp onto the whole
 * screen. Alpha in the BMP is honoured by compositing over color_bg. */
/* Stretches the bitmap to *cover* the screen (preserving aspect ratio, centred,
 * cropping the overflow) so a non-matching image is never distorted or read out
 * of bounds. */
static void jbm_bg_blit(void) {
    if (!JBM_BG.ok || JBM_BG.w <= 0 || JBM_BG.h <= 0) return;
    i64 W = (i64)G.w, H = (i64)G.h;
    i64 sw = JBM_BG.w, sh = JBM_BG.h;
    i64 winW, winH, offX, offY;
    if (W * sh >= sw * H) {
        /* screen is relatively wider: fill width, crop top/bottom */
        winW = sw;
        winH = H * sw / W;
        if (winH < 1) winH = 1;
        offX = 0;
        offY = (sh - winH) / 2;
    } else {
        winH = sh;
        winW = W * sh / H;
        if (winW < 1) winW = 1;
        offY = 0;
        offX = (sw - winW) / 2;
    }
    for (i64 y = 0; y < H; y++) {
        i64 sy = offY + y * winH / H;
        if (sy < 0) sy = 0;
        if (sy >= sh) sy = sh - 1;
        u32 *src = JBM_BG.px + (u32)sy * (u32)sw;
        for (i64 x = 0; x < W; x++) {
            i64 sx = offX + x * winW / W;
            if (sx < 0) sx = 0;
            if (sx >= sw) sx = sw - 1;
            px((int)x, (int)y, src[sx]);
        }
    }
}

/* Space background (base + glow + stars + planet) or the user bitmap.
 * Rendered once per screen into BG and blitted afterwards. */
static u8 *BG;
static int BG_which = -1;

static void draw_space(int which) {
    if (BG && BG_which == which) {
        memcpy(G.cv, BG, G.stride * G.h);
        return;
    }

    fill_rect(0, 0, (int)G.w, (int)G.h, JBM_FX.color_bg);

    if (JBM_BG.ok) {
        jbm_bg_blit();
    } else if (which == BG_MENU) {
        int gx = (int)(0.72f * (float)G.w), gy = (int)(0.45f * (float)G.h);
        radial_glow(gx, gy, farthest_corner(gx, gy) * 38 / 100, 0x643E23, 31);
        star_field(170, 170, 20, 30, 48);
        star_field(260, 260, 100, 80, 29);
        star_field(390, 390, 200, 150, 19);
        /* .jupiter box-shadow: -45px 0 90px rgba(214,150,91,.10) */
        radial_glow(PLM_X - (int)(45.0f * dk()), PLM_Y + PLM_H / 2, (int)(90.0f * dk()),
                    0xD6965B, 25);
        blit_img(PLM_PAL, PLM_IDX, PLM_MASK, PLM_X, PLM_Y, PLM_W, PLM_H);
    } else {
        int g1x = (int)(0.20f * (float)G.w), g1y = (int)(0.45f * (float)G.h);
        int g2x = (int)(0.80f * (float)G.w), g2y = (int)(0.50f * (float)G.h);
        radial_glow(g1x, g1y, farthest_corner(g1x, g1y) * 35 / 100, 0x82481E, 20);
        radial_glow(g2x, g2y, farthest_corner(g2x, g2y) * 40 / 100, 0xB46428, 10);
        star_field(173, 211, 20, 40, 102);
        star_field(251, 187, 100, 120, 57);
        star_field(331, 293, 240, 30, 77);
        /* .jupiter drop-shadow(-5px 0 25px rgba(180,100,40,.15)) */
        radial_glow(PLB_X - (int)(5.0f * dk()), PLB_Y + PLB_H / 2, (int)(38.0f * dk()),
                    0xB46428, 38);
        blit_img(PLB_PAL, PLB_IDX, PLB_MASK, PLB_X, PLB_Y, PLB_W, PLB_H);
    }

    if (!BG) {
        BG = (u8 *)jbm_mmap(G.stride * G.h, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);
        if (!BG) jbm_log("jbm: bg cache unavailable, redrawing every frame");
    }
    if (BG) {
        memcpy(BG, G.cv, G.stride * G.h);
        BG_which = which;
    }
}

/* ------------------------------------------------------------------ */
/* menu geometry: .interface left:7vw top:9vh width:370px, .menu gap   */
/* ------------------------------------------------------------------ */

struct btnfx {
    int style;
    int width;
    int height;
    int radius;
    int border;
    int text_align;
    int icon_scale;
    int show_icon;
    u32 bg;
    u32 border_color;
    u32 text;
    u32 text_sel;
};

/* Merges the per-entry overrides over the theme. Any unset value (-1/0)
 * inherits from JBM_FX, which is what the Android preview mirrors. */
static void btn_effective(int i, struct btnfx *o) {
    const jbm_entry_t *e = &JBM_ENTRIES[i];
    o->style = e->button_style >= 0 ? e->button_style : JBM_FX.button_style;
    o->width = e->button_width > 0 ? e->button_width : JBM_FX.button_width;
    o->height = e->button_height > 0 ? e->button_height : JBM_FX.button_height;
    o->radius = e->button_radius >= 0 ? e->button_radius : JBM_FX.button_radius;
    o->border = e->button_border_width >= 0 ? e->button_border_width : JBM_FX.button_border_width;
    o->text_align = e->text_align >= 0 ? e->text_align : JBM_FX.text_align;
    o->icon_scale = e->icon_scale > 0 ? e->icon_scale : JBM_FX.icon_scale;
    o->show_icon = e->show_icon >= 0 ? e->show_icon : JBM_FX.show_icons;
    o->bg = e->color_bg ? e->color_bg : JBM_FX.color_selected_bg;
    o->border_color = e->color_border ? e->color_border : JBM_FX.color_border;
    o->text = e->color_text ? e->color_text : JBM_FX.color_text;
    o->text_sel = e->color_text_sel ? e->color_text_sel : JBM_FX.color_text_sel;
}

/* Picks a procedural glyph from the entry's `icon` string, falling back to
 * its type. There are no external icon files: every glyph is drawn with the
 * same primitives as the rest of the UI, so nothing can fail to load. */
static int icon_kind(const jbm_entry_t *e) {
    const char *n = e->icon[0] ? e->icon : jbm_builtin_icon((int)e->type);
    if (jbm_str_has(n, "recovery")) return 1;
    if (jbm_str_has(n, "fastboot")) return 2;
    if (jbm_str_has(n, "download")) return 3;
    if (jbm_str_has(n, "reboot") || jbm_str_has(n, "restart")) return 4;
    if (jbm_str_has(n, "power") || jbm_str_has(n, "off")) return 5;
    if (jbm_str_has(n, "script") || jbm_str_has(n, "binary")) return 6;
    return 0;
}

static void draw_icon(int kind, int cx, int cy, int r, u32 c) {
    if (r < 2) return;
    switch (kind) {
        case 1: /* recovery: plus inside a ring */
            ring(cx, cy, r, r / 5 < 1 ? 1 : r / 5, c);
            fill_rect(cx - r / 2, cy - 1, r, 3, c);
            fill_rect(cx - 1, cy - r / 2, 3, r, c);
            break;
        case 2: /* fastboot: arrow right */
            fill_rect(cx - r, cy - r / 6, r + r / 2, r / 3 + 1, c);
            for (int i = 0; i < r / 2; i++)
                fill_rect(cx + r / 2 + i, cy - r / 2 + i, 1, r - i * 2, c);
            break;
        case 3: /* download: arrow down into a tray */
            fill_rect(cx - 2, cy - r, 4, r + r / 4, c);
            for (int i = 0; i < r / 2; i++)
                fill_rect(cx - r / 2 + i, cy + r / 4 - i, r - i * 2, 2, c);
            fill_rect(cx - r, cy + r - 2, r * 2, 2, c);
            break;
        case 4: /* reboot: circular arrow */
            ring(cx, cy, r, r / 4 < 1 ? 1 : r / 4, c);
            fill_rect(cx + r / 2, cy - r, r / 2, r / 2, c);
            break;
        case 5: /* power: broken ring + stem */
            ring(cx, cy, r, r / 4 < 1 ? 1 : r / 4, c);
            fill_rect(cx - 1, cy - r - 1, 3, r + 2, c);
            break;
        case 6: /* script: terminal window */
            round_rect_border(cx - r, cy - r / 2, r * 2, r, 2, 1, c);
            fill_rect(cx - r / 2, cy, r / 2, r / 2, c);
            break;
        default: /* system: planet with ring */
            ring(cx, cy, r, r / 5 < 1 ? 1 : r / 5, c);
            disc(cx, cy, r / 3 < 1 ? 1 : r / 3, c);
            ellipse_ring(cx, cy, r + r / 3, r / 4 < 1 ? 1 : r / 4, -18, 2, c);
            break;
    }
}

static int btn_x(int i, int *w, int *h) {
    (void)i;
    float k = dk();
    struct btnfx fx;
    btn_effective(i, &fx);
    int avail = (int)G.w - (int)((float)G.w * (float)JBM_FX.layout_margin / 100.0f) * 2;
    *w = (int)((float)fx.width * k);
    if (*w > avail) *w = avail;
    if (*w < 80) *w = 80;
    *h = (int)((float)fx.height * k);
    if (*h < 30) *h = 30;
    return (int)((float)G.w * (float)JBM_FX.layout_margin / 100.0f);
}

static int jbm_show_header(void) {
    return JBM_FX.show_title && JBM_CFG.show_brand;
}

static int menu_top(void) {
    float k = dk();
    int top = (int)((float)G.h * 0.09f);
    if (jbm_show_header()) top += (int)(38.0f * k) + (int)(5.0f * k);
    top += (int)(45.0f * k); /* .device + margin-bottom */
    return top;
}

static void btn_geom(int i, int *x, int *y, int *w, int *h) {
    float k = dk();
    *x = btn_x(i, w, h);
    int yy = menu_top();
    int start = U.scroll > 0 ? U.scroll : 0;
    if (i < start) i = start;
    for (int j = start; j < i; j++) {
        int iw, ih;
        btn_x(j, &iw, &ih);
        yy += ih + (int)((float)JBM_FX.button_spacing * k);
    }
    *y = yy;
}

/* Number of rows fully visible below the header, used to bound scrolling. */
static int visible_rows(void) {
    float k = dk();
    int y = menu_top();
    int foot = (int)(150.0f * k); /* footer + autoboot readout */
    int avail = (int)G.h - y - foot;
    int n = 0;
    int used = 0;
    for (int i = U.scroll; i < act_n(); i++) {
        int w, h;
        btn_x(i, &w, &h);
        int need = h + (int)((float)JBM_FX.button_spacing * k);
        if (used + need > avail && n > 0) break;
        used += need;
        n++;
    }
    return n < 1 ? 1 : n;
}

/* Keeps the selected row on screen, scrolling the list when needed. */
static void ensure_visible(void) {
    int rows = visible_rows();
    if (U.sel < U.scroll) U.scroll = U.sel;
    if (U.sel >= U.scroll + rows) U.scroll = U.sel - rows + 1;
    if (U.scroll < 0) U.scroll = 0;
}

static int hit_btn(int px, int py) {
    for (int i = 0; i < act_n(); i++) {
        int x, y, w, h;
        btn_geom(i, &x, &y, &w, &h);
        if (py >= y && py < y + h && px >= x && px < x + w) return i;
    }
    return -1;
}

static i64 countdown_left(void) {
    if (!U.countdown_on) return 0;
    i64 el = jbm_now_ms() - U.t0 - JBM_INTRO_MS;
    if (el < 0) el = 0; /* the intro animation is not part of the countdown */
    i64 left = U.countdown_ms - el;
    return left < 0 ? 0 : left;
}

static void draw_ui(void) {
    float k = dk();
    i64 elapsed = elapsed_ms();

    draw_space(BG_MENU);

    int ix = (int)((float)G.w * (float)JBM_FX.layout_margin / 100.0f);
    int by = (int)((float)G.h * 0.09f);
    int br = (int)(19.0f * k); /* .planet-icon: 38px */
    int bcy = by + br;

    if (jbm_show_header()) {
        ring(ix + br, bcy, br, (int)(2.0f * k) + 1, (JBM_FX.color_border & 0xFFFFFFu) | 0xFF000000u);
        ellipse_ring(ix + br, bcy, (int)(21.5f * k), (int)(6.0f * k), -18, (int)(2.0f * k),
                     (JBM_FX.color_border & 0xFFFFFFu) | 0xFF000000u);
        float bpx = 28.0f * k;
        int nx = ix + (int)(38.0f * k) + (int)(14.0f * k); /* gap:14px */
        draw_text_ls(&FN32, nx, bcy, bpx, 0, RGBA(0xEEEEEEFF), JBM_CFG.title, TA_L | VA_M);
        if (JBM_CFG.subtitle[0]) {
            draw_text_ls(&FN32, nx + measure(&FN32, JBM_CFG.title, bpx, 0), bcy, bpx, 0,
                         (JBM_FX.color_accent & 0xFFFFFFu) | 0xFF000000u, JBM_CFG.subtitle,
                         TA_L | VA_M);
        }
    }

    int pressed = T.holding ? hit_btn(T.sx, T.sy) : -1;
    for (int i = 0; i < act_n(); i++) {
        int x, y, w, h;
        btn_geom(i, &x, &y, &w, &h);
        if (y >= (int)G.h || y + h <= 0) continue; /* scrolled off-screen */
        i64 a = elapsed - (60 + i * 45 > 320 ? 320 : 60 + i * 45);
        int alpha = a < 0 ? 0 : (a < 260 ? (int)(a * 255 / 260) : 255);
        if (alpha < 6) continue;
        int sel = (i == U.sel);
        int is_down = (i == pressed);
        struct btnfx fx;
        btn_effective(i, &fx);
        int rr = (int)((float)fx.radius * k);
        int border = (int)((float)fx.border * k);
        if (border < 1 && fx.style != 2) border = 1;

        /* Background fill: style 2 (minimal) paints only a selected row,
         * style 1 (outline) keeps it transparent, style 3 (glass) uses a
         * translucent fill, style 0 (solid) uses the configured colour. */
        int fill_a = 0;
        u32 fill = fx.bg;
        if (fx.style == 0) fill_a = sel ? (is_down ? 150 : 56) : 6;
        else if (fx.style == 1) fill_a = sel ? (is_down ? 80 : 28) : 0;
        else if (fx.style == 2) fill_a = sel ? (is_down ? 70 : 34) : 0;
        else if (fx.style == 3) fill_a = sel ? (is_down ? 190 : 110) : 60;
        fill_a = fill_a * alpha / 255;
        fill_a = fill_a * JBM_FX.button_alpha / 255;
        if (fill_a > 0) {
            u32 c = ((u32)fill_a << 24) | (fill & 0xFFFFFFu);
            if (sel && !is_down) grad_rect(x, y, w, h, rr, c,
                                           ((u32)(fill_a / 4) << 24) | (fx.bg & 0xFFFFFFu));
            else fill_rect(x, y, w, h, c);
        }
        if (fx.style != 2 || sel) {
            u32 bc = ((u32)(alpha * (sel ? 220 : (fx.style == 1 ? 110 : 80)) / 255) << 24) |
                     (fx.border_color & 0xFFFFFFu);
            if (is_down || sel)
                round_rect_border(x, y, w, h, rr, border > 0 ? border : 1,
                                  ((u32)alpha << 24) | (fx.border_color & 0xFFFFFFu));
            else if (fx.style != 2)
                round_rect_border(x, y, w, h, rr, border > 0 ? border : 1, bc);
        }
        if (sel || is_down) {
            u32 hl = is_down ? JBM_FX.color_pressed_bg : JBM_FX.color_accent;
            fill_rect(x, y + (int)(8.0f * k), (int)(2.0f * k) + 1, h - (int)(16.0f * k),
                      ((u32)alpha << 24) | (hl & 0xFFFFFFu));
        }

        char lab[JBM_MAX_ENTRY_NAME];
        up_str(lab, act_name(i), sizeof(lab));
        u32 base = sel ? fx.text_sel : fx.text;
        u32 tc = ((u32)alpha << 24) | (base & 0xFFFFFFu);

        int pad = (int)(18.0f * k);
        int icon_r = (int)((float)(h / 3) * (float)fx.icon_scale / 100.0f);
        if (icon_r < 5) icon_r = 5;
        if (icon_r > h / 2 - 4) icon_r = h / 2 - 4;
        if (icon_r < 5) icon_r = 5;
        int gap = (int)(10.0f * k);
        float tpx = (float)JBM_FX.text_size * k;
        int has_desc = JBM_FX.show_description && JBM_ENTRIES[i].description[0];
        int ls = (int)(2.5f * k);
        int dls = (int)(1.5f * k);
        char desc[96];
        desc[0] = 0;
        int text_w = measure(&FN20, lab, tpx, ls);
        if (has_desc) {
            up_str(desc, JBM_ENTRIES[i].description, sizeof(desc));
            int dw = measure(&FN20, desc, tpx * 0.72f, dls);
            if (dw > text_w) text_w = dw;
        }
        const int icon_block = fx.show_icon ? icon_r * 2 + gap : 0;
        int block = text_w + icon_block;
        int block_x = x + pad;
        if (fx.text_align == 1) block_x = x + (w - block) / 2;
        else if (fx.text_align == 2) block_x = x + w - pad - block;
        if (block_x < x + pad) block_x = x + pad;
        int text_x = block_x + icon_block;
        if (fx.show_icon)
            draw_icon(icon_kind(&JBM_ENTRIES[i]), block_x + icon_r, y + h / 2, icon_r, tc);
        int al = fx.text_align == 1 ? TA_C : (fx.text_align == 2 ? TA_R : TA_L);
        int tx = fx.text_align == 1 ? text_x + text_w / 2
                                    : (fx.text_align == 2 ? text_x + text_w : text_x);
        if (has_desc) {
            int th = (int)(tpx * 1.15f);
            int dh = (int)(tpx * 0.72f);
            int total = th + dh + (int)(3.0f * k);
            int top = y + h / 2 - total / 2;
            u32 shadow = (u32)(alpha * 150 / 255) << 24;
            draw_text_ls(&FN20, tx + 1, top + 1, tpx, ls, shadow, lab, al | VA_T);
            draw_text_ls(&FN20, tx, top, tpx, ls, tc, lab, al | VA_T);
            u32 dc = ((u32)(alpha * 150 / 255) << 24) | (base & 0xFFFFFFu);
            draw_text_ls(&FN20, tx + 1, top + th + 1, tpx * 0.72f, dls, shadow, desc, al | VA_T);
            draw_text_ls(&FN20, tx, top + th, tpx * 0.72f, dls, dc, desc, al | VA_T);
        } else {
            u32 shadow = (u32)(alpha * 150 / 255) << 24;
            draw_text_ls(&FN20, tx + 1, y + h / 2 + 1, tpx, ls, shadow, lab, al | VA_M);
            draw_text_ls(&FN20, tx, y + h / 2, tpx, ls, tc, lab, al | VA_M);
        }
    }

    int x0, y0, w0, h0;
    btn_geom(act_n() - 1, &x0, &y0, &w0, &h0);
    int foot_y = y0 + h0 + (int)(34.0f * k);
    if (foot_y > (int)G.h - (int)(40.0f * k)) foot_y = (int)G.h - (int)(40.0f * k);

    /* The autoboot readout used to be a 9px footer line whose ink was only 6px
     * tall, which is unreadable on the 720x1600 panel. It is now a progress bar
     * plus a 15px countdown, kept left of the planet art (PLM_X) so it never
     * sits on top of the artwork. */
    int cw = PLM_X - ix - (int)(18.0f * k);
    if (cw > w0 || cw < (int)(120.0f * k)) cw = w0;

    if (U.countdown_on) {
        i64 left = countdown_left();
        i64 total = U.countdown_ms > 0 ? U.countdown_ms : 1;
        int bh = (int)(6.0f * k) + 1;
        fill_rect(ix, foot_y, cw, bh, RGBA(0x141A24FF));
        int fw = (int)((left * cw) / total);
        if (fw > 0) fill_rect(ix, foot_y, fw > cw ? cw : fw, bh, RGBA(0xE08B45FF));

        char cd[32];
        jbm_sn(cd, "AUTO-BOOT %lldS", (long long)((left + 999) / 1000));
        int cpx = 15.0f * k, cls = (int)(3.0f * k);
        int ty = foot_y + bh + (int)(16.0f * k);
        draw_text_ls(&FN32, ix, ty, cpx, cls, RGBA(0xE8965AFF), cd, TA_L | VA_T);
        draw_text_ls(&FN20, ix + measure(&FN32, cd, cpx, cls) + (int)(9.0f * k), ty + (int)(7.0f * k),
                     9.0f * k, (int)(1.4f * k), RGBA(0x6E7683FF), "TAP TO CANCEL", TA_L | VA_T);
    } else {
        char lab[JBM_MAX_ENTRY_NAME], msg[128];
        up_str(lab, act_name(U.sel), sizeof(lab));
        jbm_sn(msg, "%s SELECTED - TAP TO RUN", lab);
        draw_text_ls(&FN20, ix, foot_y, 10.0f * k, (int)(1.6f * k), RGBA(0x6E7683FF), msg,
                     TA_L | VA_T);
    }
}

/* One-line subtitle per entry type, so every action says what it is about to do. */
static const char *act_sub(int i) {
    switch (act_type(i)) {
        case JBM_ENTRY_TYPE_SYSTEM: return "Starting Android...";
        case JBM_ENTRY_TYPE_RECOVERY: return "Requesting recovery reboot...";
        case JBM_ENTRY_TYPE_FASTBOOT: return "Rebooting to bootloader...";
        case JBM_ENTRY_TYPE_DOWNLOAD: return "Requesting download mode...";
        case JBM_ENTRY_TYPE_REBOOT: return "Restarting device...";
        case JBM_ENTRY_TYPE_POWEROFF: return "Shutting down...";
        case JBM_ENTRY_TYPE_SCRIPT: return "Running script...";
        case JBM_ENTRY_TYPE_BINARY: return "Running program...";
    }
    return "";
}

/* Kicker above the big name; "STARTING" only reads well for System. */
static const char *act_kicker(int idx) {
    switch (act_type(idx)) {
        case JBM_ENTRY_TYPE_SYSTEM: return "STARTING";
        case JBM_ENTRY_TYPE_REBOOT: return "RESTARTING";
        case JBM_ENTRY_TYPE_POWEROFF: return "SHUTTING DOWN";
        case JBM_ENTRY_TYPE_SCRIPT:
        case JBM_ENTRY_TYPE_BINARY: return "RUNNING";
        default: return "ENTERING";
    }
}

/* The screen shown while an action is pending: the boot artwork, the mode name
 * and a spinner. Used both for System (then chains to the real init) and for
 * every other mode (then runs the action), so any tap has obvious feedback. */
static void draw_action(int idx) {
    float k = dk();

    draw_space(BG_BOOT);

    int ix = (int)((float)G.w * 0.06f); /* @900px: left:6vw */
    int by = (int)((float)G.h * 0.09f);
    int br = (int)(29.0f * k); /* .brand-icon: 58px */
    int bcy = by + br;

    ring(ix + br, bcy, br, (int)(2.0f * k) + 1, RGBA(0xC87335FF));
    ellipse_ring(ix + br, bcy, (int)(35.0f * k), (int)(11.0f * k), -33, (int)(2.0f * k),
                 RGBA(0xC87335FF));
    disc(ix + br + (int)(15.0f * k), bcy - (int)(24.0f * k), (int)(4.0f * k), RGBA(0xC87335FF));

    float bpx = 30.0f * k;
    int bls = (int)(4.0f * k);
    int nx = ix + (int)(58.0f * k) + (int)(20.0f * k); /* gap:20px */
    draw_text_ls(&FN32, nx, bcy, bpx, bls, RGBA(0xEEEEEEFF), "Jupiter", TA_L | VA_M);
    draw_text_ls(&FN32, nx + measure(&FN32, "Jupiter", bpx, bls), bcy, bpx, bls,
                 RGBA(0xC87335FF), "Boot", TA_L | VA_M);

    char big[JBM_MAX_ENTRY_NAME];
    up_str(big, act_name(idx), sizeof(big));

    int y = by + br * 2 + (int)(130.0f * k); /* .boot-content margin-top (@900px) */
    draw_text_ls(&FN20, ix, y, 17.0f * k, (int)(7.0f * k), RGBA(0xDDDDDDFF), act_kicker(idx),
                 TA_L | VA_T);
    y += line_h(17.0f * k) + (int)(18.0f * k);

    /* Shrink the headline until a long mode name ("POWER OFF") fits the panel. */
    float hpx = 54.0f * k;
    int avail = (int)G.w - ix - (int)(24.0f * k);
    while (hpx > 16.0f * k && measure(&FN48, big, hpx, (int)(hpx * 0.26f)) > avail)
        hpx -= 1.0f * k;
    int hls = (int)(hpx * 0.26f);
    draw_text_ls(&FN48, ix - (int)(2.0f * k), y, hpx, hls, RGBA(0xC87335FF), big, TA_L | VA_T);
    y += line_h(hpx) + (int)(42.0f * k);

    /* Shrink the subtitle the same way; "Requesting download mode..." is long. */
    const char *sub = act_sub(idx);
    float spx = 20.0f * k;
    int savail = (int)G.w - ix - (int)(24.0f * k);
    while (spx > 11.0f * k && measure(&FN20, sub, spx, (int)(5.0f * k)) > savail) spx -= 0.5f * k;
    draw_text_ls(&FN20, ix, y, spx, (int)(5.0f * k), RGBA(0xDDDDDDFF), sub, TA_L | VA_T);
    y += line_h(spx) + (int)(48.0f * k);

    /* .loader: 70px ring, 6px, 1.15s/turn with a 180 degree coloured arc */
    int lcx = ix + (int)(35.0f * k), lcy = y + (int)(35.0f * k);
    ring(lcx, lcy, (int)(35.0f * k), (int)(6.0f * k), RGBA(0xFFFFFF1A));
    i64 el = jbm_now_ms() - U.boot_t0;
    int rot = (int)((el % 1150) * 360 / 1150);
    arc(lcx, lcy, (int)(35.0f * k), (int)(6.0f * k), -45 - rot, 135 - rot, RGBA(0xC87335FF));

    /* footer: "JUPITERBOOT · SYSTEM", bottom:7vh */
    float fpx = 11.0f * k;
    int fls = (int)(4.0f * k);
    int fy = (int)((float)G.h - (float)G.h * 0.07f);
    draw_text_ls(&FN20, ix, fy, fpx, fls, RGBA(0x8B542FFF), "JUPITERBOOT", TA_L | VA_B);
    int fx = ix + measure(&FN20, "JUPITERBOOT", fpx, fls) + (int)(7.0f * k);
    disc(fx, fy - (int)(fpx * 0.4f), (int)(1.2f * k) + 1, RGBA(0x666666FF));
    draw_text_ls(&FN20, fx + (int)(7.0f * k), fy, fpx, fls, RGBA(0x666666FF), big, TA_L | VA_B);
}

/* A confirmed row switches to the action screen. System keeps it up for
 * JBM_BOOT_MS and then chains to /init.system; every other mode keeps it up for
 * JBM_ACTION_MS and then runs the action. */
static void start_action(int idx) {
    U.sel = idx;
    U.act = idx;
    ensure_visible();
    U.mode = (act_type(idx) == JBM_ENTRY_TYPE_SYSTEM) ? UI_BOOT : UI_BUSY;
    U.boot_t0 = jbm_now_ms();
    ui_dirty = 1;
    jbm_log("jbm: action '%s' confirmed, mode=%d, screen up for %d ms", act_name(idx), U.mode,
            U.mode == UI_BOOT ? JBM_BOOT_MS : JBM_ACTION_MS);
}

static void present(void) {
    if (!G.ok) return;
    memcpy(G.mem, G.cv, G.stride * G.h);
}

/* ------------------------------------------------------------------ */
/* entry                                                               */
/* ------------------------------------------------------------------ */

static void cleanup_and_chain(char **argv, char **envp, const char *why) {
    jbm_log("jbm: chaining to %s (%s)", JBM_REAL_INIT, why);
    if (G.ok && G.mem && G.cv) present();
#ifndef JBM_HOST
    if (G.mem) {
        sc2(SYS_munmap, (long)G.mem, G.len);
        G.mem = 0;
    }
    if (G.cv) {
        sc2(SYS_munmap, (long)G.cv, G.stride * (u32)G.h);
        G.cv = 0;
    }
#endif
    /* Close the menu's fds so they do not leak into /init.system and so
     * umount("/dev") is not EBUSY. */
    if (G.ok && G.fd >= 0) jbm_close(G.fd);
    G.fd = -1;
    if (T.fd >= 0) jbm_close(T.fd);
    T.fd = -1;
    if (jbm_klog_fd >= 0) jbm_close(jbm_klog_fd);
    jbm_klog_fd = -1;
    if (jbm_log_fd >= 0) jbm_close(jbm_log_fd);
    jbm_log_fd = -1;
    /* TODO(hardware): unmounting /dev before execve may break the Android
     * first-stage init, which can expect /dev to stay populated. The fds above
     * are now closed, so the umount should no longer fail with EBUSY. Needs
     * testing on the device; consider leaving /dev mounted. */
    jbm_umount("/sys");
    jbm_umount("/proc");
    jbm_umount("/dev");
    /* /cache only exists because we mounted it; leave the system as we found it. */
    if (JBM_CACHE_MOUNTED) {
        jbm_umount("/cache");
        JBM_CACHE_MOUNTED = 0;
    }
    jbm_sleep_ms(20);
#ifdef JBM_HOST
    jbm_log("jbm: host build, would exec %s", JBM_REAL_INIT);
#else
    sc6(SYS_execve, (long)JBM_REAL_INIT, (long)argv, (long)envp, 0, 0, 0);
    jbm_log("jbm: exec %s failed, restarting instead of re-running the menu", JBM_REAL_INIT);
    jbm_sleep_ms(200);
    jbm_reboot_cmd(JB_RB_RESTART, 0);
#endif
    for (;;) jbm_sleep_ms(200);
}

static void act_system(char **argv, char **envp, const char *why) { cleanup_and_chain(argv, envp, why); }

int jbm_main(int argc, char **argv, char **envp);

/* Sets up just enough of /dev to log, read input and use the framebuffer. */
static void dev_init(void) {
#ifndef JBM_HOST
    jbm_log("jbm: dev_init begin");
    jbm_mkdir("/dev", 0755);
    i64 r = jbm_mount("tmpfs", "/dev", "tmpfs", 0, NULL);
    if (r != 0) jbm_log("jbm: tmpfs on /dev failed (%d), reusing ramdisk /dev", (int)r);
    i64 c = jbm_mknod("/dev/console", 0600 | S_IFCHR, (5 << 8) | 1);
    i64 k = jbm_mknod("/dev/kmsg", 0600 | S_IFCHR, (1 << 8) | 11);
    i64 nu = jbm_mknod("/dev/null", 0666 | S_IFCHR, (1 << 8) | 3);
    jbm_log("jbm: dev_init mount=%d mknod console=%d kmsg=%d null=%d", (int)r, (int)c, (int)k,
            (int)nu);
#endif
    /* -1 leaves jbm_log() free to retry later if the console node was not
     * available yet when dev_init() ran. */
    jbm_klog_fd = -1;
    jbm_log("jbm: dev_init done, klog_fd=%d kmsg_err=%d con_err=%d", jbm_klog_fd, jbm_kmsg_err,
            jbm_con_err);
}

#ifdef JBM_HOST
/* Host preview: renders the real draw_ui()/draw_action() path into an
 * off-screen ARGB8888 canvas at the panel size (720x1600) and writes a binary
 * PPM, so layout and colours can be checked without a device.
 * Usage: jbm_host out.ppm [at_ms] [mode]   mode: 0 menu, 1 menu/Recovery,
 *                                                  2 booting System,
 *                                                  3 action Recovery
 */
static void host_preview(const char *path, i64 at_ms, int mode) {
    G.w = 720;
    G.h = 1600;
    G.bpp = 32;
    G.stride = G.w * 4;
    G.len = G.stride * G.h;
    G.rl = G.gl = G.bl = G.tl = 8;
    G.ro = 16;
    G.go = 8;
    G.bo = 0;
    G.to = 24;
    G.cv = (u8 *)calloc(1, G.len);
    G.mem = (u8 *)calloc(1, G.len);
    if (!G.cv || !G.mem) {
        fprintf(stderr, "[jbm:host] out of memory\n");
        exit(1);
    }
    G.ok = 1;

    U.sel = (mode == 1) ? jbm_find_type(JBM_ENTRY_TYPE_RECOVERY) : jbm_find_type(JBM_ENTRY_TYPE_SYSTEM);
    U.act = (mode == 3) ? jbm_find_type(JBM_ENTRY_TYPE_RECOVERY) : jbm_find_type(JBM_ENTRY_TYPE_SYSTEM);
    U.mode = (mode == 2 || mode == 3) ? ((mode == 3) ? UI_BUSY : UI_BOOT) : UI_MENU;
    U.t0 = jbm_now_ms() - at_ms;
    U.boot_t0 = jbm_now_ms() - at_ms;
    U.countdown_ms = (i64)JBM_CFG.menu_timeout_sec * 1000;
    U.countdown_on = JBM_CFG.show_countdown && JBM_CFG.menu_timeout_sec > 0 && mode != 1;

    if (U.mode == UI_MENU) draw_ui();
    else draw_action(U.act);

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[jbm:host] cannot open %s\n", path);
        exit(1);
    }
    fprintf(f, "P6\n%u %u\n255\n", G.w, G.h);
    for (u32 y = 0; y < G.h; y++) {
        for (u32 x = 0; x < G.w; x++) {
            u32 c = load_px((int)x, (int)y);
            fputc((int)((c >> 16) & 0xFF), f);
            fputc((int)((c >> 8) & 0xFF), f);
            fputc((int)(c & 0xFF), f);
        }
    }
    fclose(f);
    fprintf(stderr, "[jbm:host] wrote %s (%ux%u, mode %d, countdown %llds left)\n", path, G.w, G.h,
            mode, (long long)((countdown_left() + 999) / 1000));
}

/* ------------------------------------------------------------------ */
/* host selftest: parses config/entries/theme/BMP from temp files and  */
/* checks the values the renderer would use. No device needed.         */
/* ------------------------------------------------------------------ */

static int st_fail;

static void chk(int cond, const char *what) {
    fprintf(stderr, "[selftest] %-42s %s\n", what, cond ? "ok" : "FAIL");
    if (!cond) st_fail++;
}

static void st_mkdir(const char *p) { (void)mkdir(p, 0755); }

static void st_write(const char *path, const char *text) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[selftest] cannot write %s\n", path);
        st_fail++;
        return;
    }
    fputs(text, f);
    fclose(f);
}

static int selftest(void) {
    const char *dir = "/tmp/jbm_selftest";
    char p[256];
    st_mkdir("/tmp");
    st_mkdir(dir);
    jbm_sn(p, "%s/themes", dir);
    st_mkdir(p);

    jbm_sn(p, "%s/config.ini", dir);
    st_write(p,
             "# JupiterBoot selftest\n"
             "theme = test\n"
             "color_accent = #123456\n"
             "button_width = 500\n"
             "menu_timeout_sec = 8\n"
             "show_countdown = 0\n"
             "prefer_drm = 1\n"
             "title = Selftest\n"
             "show_description = 1\n"
             "layout_margin = 9\n");

    jbm_sn(p, "%s/themes/test.ini", dir);
    st_write(p, "color_border = 0xAABBCC\ntext_size = 21\nbutton_style = 1\nbutton_alpha = 180\n");

    jbm_sn(p, "%s/entries.ini", dir);
    st_write(p,
             "[alpha]\n"
             "name = Alpha\n"
             "type = system\n"
             "order = 2\n"
             "\n"
             "[custom]\n"
             "name = MyScript\n"
             "type = script\n"
             "path = /data/jbm/test.sh\n"
             "args = --verbose run\n"
             "order = 1\n"
             "default = 1\n"
             "description = Runs a test\n"
             "icon = script\n"
             "button_style = 3\n"
             "button_width = 600\n"
             "button_radius = 24\n"
             "text_align = 1\n"
             "color_text_sel = #00FF00\n"
             "\n"
             "[off]\n"
             "name = Disabled\n"
             "type = reboot\n"
             "order = 0\n"
             "enabled = 0\n");

    jbm_strcpy_(JBM_ROOT, dir, sizeof(JBM_ROOT));
    jbm_set_default_config(&JBM_CFG);
    jbm_load_config(&JBM_CFG);
    jbm_load_entries();
    jbm_ensure_default_entries();
    jbm_build_effects();

    chk(JBM_NUM_ENTRIES == 2, "entries: disabled row dropped (2 left)");
    chk(JBM_NUM_ENTRIES == 2 && strcmp(JBM_ENTRIES[0].name, "MyScript") == 0, "entries: sorted by order");
    chk(JBM_NUM_ENTRIES == 2 && JBM_ENTRIES[0].type == JBM_ENTRY_TYPE_SCRIPT, "entries: type parsed");
    chk(JBM_NUM_ENTRIES == 2 && strcmp(JBM_ENTRIES[0].args, "--verbose run") == 0, "entries: args parsed");
    chk(jbm_default_entry_index() == 0, "entries: default flag honoured");
    chk(JBM_CFG.menu_timeout_sec == 8, "config: menu_timeout_sec");
    chk(JBM_CFG.show_countdown == 0, "config: show_countdown");
    chk(JBM_CFG.prefer_drm == 1, "config: prefer_drm");
    chk((JBM_CFG.mask & JBM_K_COLOR_ACCENT) != 0, "config: accent mask set");
    chk(JBM_FX.color_accent == 0xFF123456u, "fx: accent from config");
    chk(JBM_FX.color_border == 0xFFAABBCCu, "fx: border from theme");
    chk(JBM_FX.text_size == 21, "fx: text_size from theme");
    chk(JBM_FX.button_width == 500, "fx: button_width from config");
    chk(strcmp(JBM_CFG.title, "Selftest") == 0, "config: title");
    chk(JBM_CFG.layout_margin == 9 && JBM_FX.layout_margin == 9, "config: layout_margin");
    chk(JBM_FX.button_style == 1, "theme: button_style inherited");
    chk(JBM_FX.button_alpha == 180, "theme: button_alpha parsed");
    chk(JBM_ENTRIES[0].button_style == 3, "entry: button_style");
    chk(JBM_ENTRIES[0].button_width == 600, "entry: button_width");
    chk(JBM_ENTRIES[0].button_radius == 24, "entry: button_radius");
    chk(JBM_ENTRIES[0].text_align == 1, "entry: text_align");
    chk(JBM_ENTRIES[0].color_text_sel == 0xFF00FF00u, "entry: color_text_sel");
    chk(strcmp(JBM_ENTRIES[0].description, "Runs a test") == 0, "entry: description");
    { struct btnfx bfx; btn_effective(0, &bfx); chk(bfx.style == 3 && bfx.width == 600 && bfx.text_align == 1, "entry: effective style merge"); }

    /* 2x2 24-bpp BMP: top row red,green; bottom row blue,white. */
    jbm_sn(p, "%s/background.bmp", dir);
    FILE *f = fopen(p, "wb");
    if (f) {
        static const u8 hdr[54] = {'B', 'M', 70, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0,
                                   2, 0, 0, 0, 2, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0, 0, 16, 0, 0, 0,
                                   0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        static const u8 pix[16] = {255, 0, 0, 255, 255, 255, 0, 0, 0, 0, 255, 0, 255, 0, 0, 0};
        fwrite(hdr, 1, sizeof(hdr), f);
        fwrite(pix, 1, sizeof(pix), f);
        fclose(f);
    }
    int br = jbm_bg_load(p);
    chk(br == 0 && JBM_BG.ok, "bmp: loaded");
    chk(JBM_BG.w == 2 && JBM_BG.h == 2, "bmp: 2x2");
    chk(JBM_BG.px && JBM_BG.px[0] == 0xFFFF0000u, "bmp: top-left red");
    chk(JBM_BG.px && JBM_BG.px[3] == 0xFFFFFFFFu, "bmp: bottom-right white");

    /* ---- first-run tree creation in a fresh directory ---- */
    char root[128], bad[128], envroot[128], ep[256];
    jbm_sn(root, "/tmp/jbm_tree_%d", (int)getpid());
    jbm_strcpy_(JBM_ROOT, root, sizeof(JBM_ROOT));
    JBM_CFG_WRITABLE = 1;
    jbm_ensure_tree();
    jbm_generate_defaults();
    chk(jbm_dir_ok(root), "tree: root created");
    jbm_sn(p, "%s/themes", root);
    chk(jbm_dir_ok(p), "tree: themes/ created");
    jbm_sn(p, "%s/scripts", root);
    chk(jbm_dir_ok(p), "tree: scripts/ created");
    jbm_sn(p, "%s/bin", root);
    chk(jbm_dir_ok(p), "tree: bin/ created");
    jbm_sn(p, "%s/config.ini", root);
    chk(jbm_exists(p), "tree: config.ini created");
    jbm_sn(p, "%s/entries.ini", root);
    chk(jbm_exists(p), "tree: entries.ini created");
    jbm_sn(p, "%s/themes/default.ini", root);
    chk(jbm_exists(p), "tree: themes/default.ini created");
    jbm_sn(ep, "%s/background.bmp", root);
    chk(jbm_exists(ep), "tree: background.bmp created");

    /* Generated files must parse back to the built-in defaults. */
    jbm_load_config(&JBM_CFG);
    jbm_load_entries();
    jbm_ensure_default_entries();
    jbm_build_effects();
    chk(JBM_NUM_ENTRIES == 6, "tree: 6 default entries parse");
    chk(JBM_NUM_ENTRIES == 6 && strcmp(JBM_ENTRIES[0].id, "system") == 0, "tree: first entry is system");
    chk(JBM_CFG.menu_timeout_sec == JBM_AUTOBOOT_SEC, "tree: config default honoured");

    /* The generated background is a valid, loadable BMP. */
    chk(jbm_bg_load(ep) == 0 && JBM_BG.ok, "tree: default background loads");
    chk(JBM_BG.w == 180 && JBM_BG.h == 400, "tree: background dimensions");

    /* A user edit must survive a later regeneration. */
    jbm_sn(p, "%s/config.ini", root);
    st_write(p, "menu_timeout_sec = 3\nshow_brand = 0\n");
    jbm_write_default_config();
    jbm_load_config(&JBM_CFG);
    chk(JBM_CFG.menu_timeout_sec == 3, "tree: existing config not overwritten");
    chk(JBM_CFG.show_brand == 0, "tree: existing config value kept");

    /* An invalid bitmap is rejected and leaves the procedural fallback. */
    jbm_sn(p, "%s/broken.bmp", root);
    st_write(p, "definitely not a bitmap");
    chk(jbm_bg_load(p) != 0 && !JBM_BG.ok, "bmp: invalid file rejected");

    /* A root path that is actually a file fails cleanly. */
    jbm_sn(bad, "/tmp/jbm_badfile_%d", (int)getpid());
    st_write(bad, "x");
    char saved_root[128];
    jbm_strcpy_(saved_root, JBM_ROOT, sizeof(saved_root));
    jbm_strcpy_(JBM_ROOT, bad, sizeof(JBM_ROOT));
    jbm_ensure_tree();
    jbm_generate_defaults();
    chk(!jbm_dir_ok(bad), "tree: file-instead-of-dir handled");
    jbm_load_config(&JBM_CFG);
    chk(JBM_CFG.menu_timeout_sec == JBM_AUTOBOOT_SEC, "tree: defaults after write failure");
    jbm_strcpy_(JBM_ROOT, saved_root, sizeof(JBM_ROOT));

    /* JUPITER_CONFIG_ROOT wins on the host and triggers creation there. */
    jbm_sn(envroot, "/tmp/jbm_env_%d", (int)getpid());
    setenv("JUPITER_CONFIG_ROOT", envroot, 1);
    jbm_config_init();
    chk(strcmp(JBM_ROOT, envroot) == 0, "env: JUPITER_CONFIG_ROOT honoured");
    chk(jbm_dir_ok(envroot), "env: tree created under env root");
    jbm_sn(p, "%s/entries.ini", envroot);
    chk(jbm_exists(p), "env: entries.ini created");
    unsetenv("JUPITER_CONFIG_ROOT");
    char fb[32];
    jbm_sn(fb, "#%06x", 0x020202u);
    chk(strcmp(fb, "#020202") == 0, "fmt: %06x pads to width 6");
    jbm_sn(fb, "%d/%08x", -5, 0xabu);
    chk(strcmp(fb, "-5/000000ab") == 0, "fmt: %d and %08x");

    fprintf(stderr, "[selftest] %s (%d failures)\n", st_fail ? "FAILED" : "PASSED", st_fail);
    return st_fail ? 1 : 0;
}

int main(int argc, char **argv) {
    fprintf(stderr, "[jbm:host] argc=%d argv0=%s\n", argc, argc > 0 ? argv[0] : "-");
    dev_init();
    if (argc >= 2 && strcmp(argv[1], "--selftest") == 0) return selftest();
    jbm_config_init();
    if (argc >= 2) {
        i64 at_ms = argc >= 3 ? (i64)atoi(argv[2]) : 0;
        int mode = argc >= 4 ? atoi(argv[3]) : 0;
        host_preview(argv[1], at_ms, mode);
        return 0;
    }
    touch_open();
    fprintf(stderr, "[jbm:host] gfx_ok=%d touch_fd=%d\n", G.ok, T.fd);
    fprintf(stderr, "[jbm:host] usage: %s out.ppm [at_ms] [mode 0|1|2|3]\n", argv[0]);
    return 0;
}
#endif

int jbm_main(int argc, char **argv, char **envp) {
    dev_init();
    if (jbm_mount("proc", "/proc", "proc", 0, NULL) != 0)
        jbm_log("jbm: mount /proc failed");
    if (jbm_mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
        jbm_log("jbm: mount /sys failed");
    jbm_log("jbm: v%s start argc=%d argv0=%s", JBM_VERSION, argc, argc > 0 && argv ? argv[0] : "-");

    /* Bring /cache up (if needed), create any missing default files, and load
     * config.ini/entries.ini/themes. */
    jbm_config_init();

    /* Optional background bitmap; falls back to the procedural space scene. */
    if (JBM_CFG.background_path[0] &&
        jbm_bg_load(JBM_CFG.background_path) == 0)
        jbm_log("jbm: background %s (%dx%d)", JBM_CFG.background_path, JBM_BG.w, JBM_BG.h);
    else
        jbm_log("jbm: no background bitmap, using built-in scene");

    /* Pick a display backend: framebuffer by default, KMS as fallback (or the
     * other way round when prefer_drm=1). */
    if (gfx_detect() != 0)
        cleanup_and_chain(argv, envp, "no display backend");
    jbm_log("jbm: display %ux%u backend=%s orient=%d", G.w, G.h, G.is_drm ? "drm" : "fb",
            jbm_orient_deg());

    touch_open();

    U.sel = jbm_default_entry_index();
    U.act = U.sel;
    U.mode = UI_MENU;
    U.t0 = jbm_now_ms();
    U.countdown_ms = (i64)JBM_CFG.menu_timeout_sec * 1000;
    U.countdown_on = JBM_CFG.show_countdown && JBM_CFG.menu_timeout_sec > 0;

    i64 start = U.t0;
    i64 last_draw = 0;
    i64 last_sync = 0;
    i64 last_cd = -1;
    i64 last_mode = UI_MENU;
    int kicked = 0;
    i64 kick_t = 0;

    for (;;) {
        i64 now = jbm_now_ms();
        touch_poll();

        /* One tap runs the row under the finger: press highlights it, release
         * executes it. A confirm-by-second-tap model only ever looked like a
         * dead menu. */
        if (T.pressed && U.mode == UI_MENU) {
            U.countdown_on = 0;
            int idx = hit_btn(T.sx, T.sy);
            if (idx >= 0) {
                U.sel = idx;
                ui_dirty = 1;
            }
        }
        if (T.released && U.mode == UI_MENU) {
            int idx = hit_btn(T.tap_x, T.tap_y);
            if (idx >= 0) start_action(idx);
        }

        if (U.mode == UI_MENU && U.countdown_on) {
            i64 s = countdown_left() / 1000;
            if (s != last_cd) {
                last_cd = s;
                jbm_log("jbm: autoboot %llds left", (long long)s);
            }
            if (elapsed_ms() >= JBM_INTRO_MS && countdown_left() <= 0) {
                U.countdown_on = 0;
                start_action(jbm_default_entry_index());
            }
        }

        /* System keeps the booting screen up for JBM_BOOT_MS and then chains to
         * the real init; the other modes run their action after JBM_ACTION_MS,
         * so the screen change is actually visible before the reboot. */
        if (U.mode == UI_BOOT && now - U.boot_t0 >= JBM_BOOT_MS) {
            jbm_log("jbm: boot screen done after %lld ms, chaining", (long long)(now - U.boot_t0));
            cleanup_and_chain(argv, envp, "boot screen elapsed");
        }
        if (U.mode == UI_BUSY && now - U.boot_t0 >= JBM_ACTION_MS) {
            jbm_log("jbm: action screen done after %lld ms, running %s",
                    (long long)(now - U.boot_t0), act_name(U.act));
            if (perform(U.act, argv, envp) == 0) {
                /* script/binary finished: come back to the menu */
                U.mode = UI_MENU;
                U.countdown_on = 0;
                U.t0 = jbm_now_ms();
                ui_dirty = 1;
            }
        }

        if (now - start > (i64)JBM_TOTAL_TIMEOUT_SEC * 1000) {
            cleanup_and_chain(argv, envp, "safety timeout");
        }

        if (ui_dirty || now - last_draw > 33) {
            int changed = ui_dirty;
            if (U.mode == UI_MENU) draw_ui();
            else draw_action(U.act);
            present();
            if (changed || U.mode != last_mode) {
                jbm_log("jbm: drew mode %d, reapplying fb", U.mode);
                gfx_reapply();
            }
            last_mode = U.mode;
            ui_dirty = 0;
            last_draw = now;
            /* Two kicks: right after the first frame (so the layer is
             * reprogrammed with the menu already in memory) and once more
             * later, in case something re-asserts the boot logo. */
            if (kicked == 0) {
                kicked = 1;
                kick_t = now;
                gfx_kick();
            } else if (kicked == 1 && now - kick_t > 1500) {
                kicked = 2;
                gfx_kick();
            }
        } else {
            jbm_sleep_ms(8);
        }

        /* Heartbeat: one cheap mode re-apply per second, so a dropped update
         * cannot leave the panel stuck on an old screen. */
        if (now - last_sync > 1000) {
            last_sync = now;
            gfx_reapply();
        }
    }
    return 0;
}
