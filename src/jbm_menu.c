#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdint.h>

#include "jbm_font.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

#define NULL ((void *)0)
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
#define JBM_VERSION "1.0"
#endif
#ifndef JBM_TOTAL_TIMEOUT_SEC
#define JBM_TOTAL_TIMEOUT_SEC 240
#endif

/* Duration of the entry slide-in animation, in ms. */
#define JBM_INTRO_MS 480

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

#else /* freestanding aarch64 */

#define SYS_ioctl 29
#define SYS_mknodat 33
#define SYS_mkdirat 34
#define SYS_umount2 39
#define SYS_mount 40
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
#define SYS_munmap 215
#define SYS_syncfs 267
#define SYS_execve 221
#define SYS_mmap 222

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64
#define O_EXCL 128
#define O_TRUNC 512
#define O_NONBLOCK 2048

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
#define FBIOGET_FSCREENINFO 0x4602

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
    if (e < 0 && e > -4096) return 0; /* kernel returned -errno */
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
        while (*fmt == 'l' || *fmt == 'z' || *fmt == 'j' || *fmt == 'h') fmt++;
        switch (*fmt++) {
            case 'd':
            case 'i':
                o = jbm_fmtd(o, va_arg(ap, i64), pad, padc);
                break;
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

static void jbm_log(const char *fmt, ...) {
    char b[512];
    char l[544];
    va_list ap;
    va_start(ap, fmt);
    jbm_vfmt(b, fmt, ap);
    va_end(ap);
    int n = 0;
    if (jbm_klog_fd < 0) {
        jbm_klog_fd = jbm_open("/dev/console", O_WRONLY, 0);
        if (jbm_klog_fd < 0) jbm_klog_fd = jbm_open("/dev/kmsg", O_WRONLY, 0);
    }
    l[n++] = '<';
    l[n++] = '6';
    l[n++] = '>';
    for (int i = 0; b[i] && n < (int)sizeof(l) - 2; i++) l[n++] = b[i];
    l[n++] = '\n';
    if (jbm_klog_fd >= 0) jbm_write(jbm_klog_fd, l, (u32)n);
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
/* canvas / framebuffer                                                */
/* ------------------------------------------------------------------ */

struct gfx {
    int fd;
    u8 *mem;
    u8 *cv;
    u32 stride;
    u32 len;
    u32 w, h, bpp;
    u32 ro, rl, go, gl, bo, bl, to, tl;
    int ok;
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

static int gfx_open(void) {
    static char nodes[4][40];
    int fd = -1;
    for (int i = 0; i < 4; i++) {
        jbm_sn(nodes[i], "/dev/fb%d", i);
        fd = jbm_open(nodes[i], O_RDWR, 0);
        if (fd >= 0) break;
    }
    if (fd < 0) {
        jbm_mkdir("/dev", 0755);
        jbm_mkdir("/dev/graphics", 0755);
        jbm_mknod("/dev/graphics/fb0", 0600, (29 << 8) | 0);
        fd = jbm_open("/dev/graphics/fb0", O_RDWR, 0);
    }
    if (fd < 0) {
        jbm_log("jbm: no framebuffer node");
        return -1;
    }

    struct jbm_fb_var var;
    struct jbm_fb_fix fix;
    memset(&var, 0, sizeof(var));
    memset(&fix, 0, sizeof(fix));

    if (jbm_ioctl(fd, FBIOGET_VSCREENINFO, &var) != 0 || var.xres < 16 || var.xres > 8192 ||
        var.yres < 16 || var.yres > 8192) {
        jbm_log("jbm: FBIOGET_VSCREENINFO unusable");
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
        jbm_log("jbm: fb mmap failed");
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
    G.ok = 1;
    jbm_log("jbm: fb %ux%u bpp=%u stride=%u len=%u rgb=%u:%u/%u:%u/%u:%u a=%u:%u", G.w, G.h, G.bpp, stride, len,
            G.rl, G.ro, G.gl, G.go, G.bl, G.bo, G.tl, G.to);
    return 0;
}

static u32 mix(u32 c1, u32 c2, u32 t) {
    u32 r = (((c1 >> 16) & 0xFF) * (255 - t) + ((c2 >> 16) & 0xFF) * t) / 255;
    u32 g = (((c1 >> 8) & 0xFF) * (255 - t) + ((c2 >> 8) & 0xFF) * t) / 255;
    u32 b = ((c1 & 0xFF) * (255 - t) + (c2 & 0xFF) * t) / 255;
    return (r << 16) | (g << 8) | b;
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

static void round_rect(int x, int y, int w, int h, int r, u32 argb) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int in = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        fill_rect(x + in, y + row, w - 2 * in, 1, argb);
    }
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

static void tri(int x1, int y1, int x2, int y2, int x3, int y3, u32 c) {
    int xs[3], ys[3];
    xs[0] = x1;
    xs[1] = x2;
    xs[2] = x3;
    ys[0] = y1;
    ys[1] = y2;
    ys[2] = y3;
    int ymin = y1 < y2 ? y1 : y2, ymax = y1 > y2 ? y1 : y2;
    if (y3 < ymin) ymin = y3;
    if (y3 > ymax) ymax = y3;
    for (int y = ymin; y <= ymax; y++) {
        int lo = 0x7FFFFFFF, hi = -0x7FFFFFFF;
        for (int e = 0; e < 3; e++) {
            int ax = xs[e], ay = ys[e], bx = xs[(e + 1) % 3], by = ys[(e + 1) % 3];
            if (ay == by) continue;
            if (!((y >= ay && y < by) || (y >= by && y < ay))) continue;
            int x = ax + (int)((i64)(bx - ax) * (y - ay) / (by - ay));
            if (x < lo) lo = x;
            if (x > hi) hi = x;
        }
        if (hi >= lo) fill_rect(lo, y, hi - lo + 1, 1, c);
    }
}

static void quad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, u32 c) {
    tri(x1, y1, x2, y2, x3, y3, c);
    tri(x1, y1, x3, y3, x4, y4, c);
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
static struct font FDIG = {FDIG_data, FDIG_meta, FDIG_ATLAS_W, FDIG_ATLAS_H, FDIG_PX,
                           FDIG_TOP,  FDIG_BOT,  FDIG_FIRST, FDIG_COUNT};

#define TA_L 0
#define TA_C 1
#define TA_R 2
#define VA_T 0
#define VA_M 4
#define VA_B 8

static float glyph_adv(struct font *f, int idx, float k) { return (float)f->meta[idx][6] * k; }

static float scale_of(struct font *f, float px) { return px / (float)f->px; }

static int measure(struct font *f, const char *s, float px) {
    float k = scale_of(f, px);
    float w = 0;
    for (const char *p = s; *p; p++) {
        int i = (int)(u8)*p - f->first;
        if (i < 0 || i >= f->count) continue;
        w += glyph_adv(f, i, k);
    }
    return (int)(w + 0.5f);
}

static int line_h(float px) { return (int)(px * 1.32f + 0.5f); }

static void draw_text(struct font *f, int x, int y, float px, u32 argb, const char *s, int align) {
    if (!f->count) return;
    float k = scale_of(f, px);
    int lh = line_h(px);
    if (align & TA_C) x -= measure(f, s, px) / 2;
    else if (align & TA_R) x -= measure(f, s, px);
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
        x += (int)(glyph_adv(f, idx, k) + 0.5f);
    }
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

    int fallback_fd = -1;
    int fallback_mt = 0;
    char fallback_nm[64];
    struct jbm_absinfo fax, fay;
    memset(fallback_nm, 0, sizeof(fallback_nm));

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
        jbm_mknod(node, 0600, (maj << 8) | min);
#endif

        int fd = jbm_open(node, O_RDONLY | O_NONBLOCK, 0);
        if (fd < 0) continue;

        char nm[64];
        memset(nm, 0, sizeof(nm));
        struct jbm_absinfo ax, ay;
        int hx = jbm_ioctl(fd, EVIOCGNAME(sizeof(nm)), nm) >= 0;
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

        /* Prefer a device whose name mentions "touch"; keep the first usable
         * event node as a fallback in case none is named like a touchscreen. */
        if (jbm_str_has(nm, "touch")) {
            T.fd = fd;
            T.mt = mt;
            T.ranged = 1;
            T.x0 = ax.minimum;
            T.x1 = ax.maximum;
            T.y0 = ay.minimum;
            T.y1 = ay.maximum;
            T.sx = (int)G.w / 2;
            T.sy = (int)G.h / 2;
            jbm_log("jbm: touch=%s x[%d..%d] y[%d..%d] %s", hx ? nm : "(noname)", T.x0, T.x1, T.y0,
                    T.y1, T.mt ? "mt" : "abs");
            if (fallback_fd >= 0) jbm_close(fallback_fd);
            return 1;
        }
        if (fallback_fd < 0) {
            fallback_fd = fd;
            fallback_mt = mt;
            memcpy(fallback_nm, nm, sizeof(fallback_nm));
            fax = ax;
            fay = ay;
        } else {
            jbm_close(fd);
        }
    }

    if (fallback_fd >= 0) {
        T.fd = fallback_fd;
        T.mt = fallback_mt;
        T.ranged = 1;
        T.x0 = fax.minimum;
        T.x1 = fax.maximum;
        T.y0 = fay.minimum;
        T.y1 = fay.maximum;
        T.sx = (int)G.w / 2;
        T.sy = (int)G.h / 2;
        jbm_log("jbm: touch=%s x[%d..%d] y[%d..%d] %s (fallback: no named touchscreen)",
                fallback_nm[0] ? fallback_nm : "(noname)", T.x0, T.x1, T.y0, T.y1,
                T.mt ? "mt" : "abs");
        return 1;
    }
    jbm_log("jbm: no usable touchscreen (event nodes with ABS axes)");
    return 0;
}

static void touch_map(void) {
    i64 rx = (i64)T.x1 - T.x0, ry = (i64)T.y1 - T.y0;
    if (rx <= 0 || ry <= 0) return;
    i64 ax = T.rx, ay = T.ry;
    if (ax < T.x0) ax = T.x0;
    if (ax > T.x1) ax = T.x1;
    if (ay < T.y0) ay = T.y0;
    if (ay > T.y1) ay = T.y1;
    i64 sx = ((ax - T.x0) * ((i64)G.w - 1)) / rx;
    i64 sy = ((ay - T.y0) * ((i64)G.h - 1)) / ry;
    T.sx = (int)sx;
    T.sy = (int)sy;
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
/* actions                                                             */
/* ------------------------------------------------------------------ */

#define ACT_COUNT 6
enum { ACT_SYSTEM, ACT_RECOVERY, ACT_FASTBOOT, ACT_DOWNLOAD, ACT_REBOOT, ACT_POWEROFF };

static const char *act_label[ACT_COUNT] = {"System",      "Recovery",   "Fastboot",
                                           "Download",    "Reboot",     "Power off"};
static const char *act_sub[ACT_COUNT] = {"Boot installed Android", "Reboot to recovery",
                                         "Reboot to bootloader",   "Enter download / ODIN mode",
                                         "Restart the device",     "Shut the device down"};
static const u32 act_color[ACT_COUNT] = {0x34D399, 0x60A5FA, 0xFBBF24,
                                         0xA78BFA, 0x22D3EE, 0xF87171};
static const int act_hold_ms[ACT_COUNT] = {0, 900, 700, 900, 700, 900};

static void enter_download_mode(void) {
    /* On this platform there is no known userspace trigger for the MTK download
     * (ODIN) mode. The Download action now reboots with the reason "download"
     * via LINUX_REBOOT_CMD_RESTART2; whether the A22 5G bootloader acts on that
     * reason has not been validated on hardware.
     */
    jbm_log("jbm: requesting download mode (reboot reason 'download')");
}

static void act_system(char **argv, char **envp, const char *why);

static void perform(int idx, char **argv, char **envp) {
    switch (idx) {
        case ACT_SYSTEM:
            act_system(argv, envp, "user selected System");
            break;
        case ACT_RECOVERY:
            jbm_log("jbm: rebooting with reason 'recovery' (no flash write)");
            jbm_sync_all();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RECOVERY, 0);
            break;
        case ACT_FASTBOOT:
            jbm_log("jbm: rebooting to bootloader (fastboot)");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_BOOTLOADER, 0);
            break;
        case ACT_DOWNLOAD:
            enter_download_mode();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_DOWNLOAD, 0);
            break;
        case ACT_REBOOT:
            jbm_log("jbm: rebooting to system");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RESTART, 0);
            break;
        default:
            jbm_reboot_cmd(JB_RB_POWER_OFF, 0);
            break;
    }
    for (;;) jbm_sleep_ms(500);
}

/* ------------------------------------------------------------------ */
/* ui                                                                  */
/* ------------------------------------------------------------------ */

struct ui {
    int sel;
    int hold_idx;
    i64 hold_start;
    i64 t0;
    i64 countdown_ms;
    int countdown_on;
    char status[160];
    int busy;
};

static struct ui U;

static i64 elapsed_ms(void) { return jbm_now_ms() - U.t0; }

static float ui_scale(void) {
    float kh = (float)G.h / 2400.0f;
    float kw = (float)G.w / 1080.0f;
    float k = kh < kw ? kh : kw;
    if (k < 0.35f) k = 0.35f;
    return k;
}

static void draw_bg(float k) {
    u32 c1 = 0x0C111B, c2 = 0x05070C;
    for (int y = 0; y < (int)G.h; y += 2) {
        u32 t = (u32)((u32)y * 255u / (G.h > 1 ? G.h - 1 : 1));
        fill_rect(0, y, (int)G.w, 2, mix(c1, c2, t) | 0xFF000000u);
    }
    int cx = (int)G.w / 2;
    int cy = (int)((float)G.h * 0.16f);
    int rad = (int)((float)G.w * 1.1f);
    for (int i = 0; i < 26; i++) {
        int rr = rad - i * (int)((float)G.w * 0.05f);
        if (rr <= 0) break;
        u32 a = (u32)(7 + i);
        if (a > 40) a = 40;
        ring(cx, cy, rr, (int)(k * 10.0f) + 4, (a << 24) | 0x6E86FF);
    }
}

static void draw_icon(int kind, int cx, int cy, int r, u32 c) {
    int t = r / 4 + 1;
    switch (kind) {
        case ACT_SYSTEM:
            ring(cx, cy, r, t * 2, c);
            tri(cx - r / 4, cy - r / 2, cx - r / 4, cy + r / 2, cx + r / 2, cy, c);
            break;
        case ACT_RECOVERY:
            arc(cx, cy, r - t, t * 2, -70, 210, c);
            tri(cx + r / 2, cy - r * 4 / 5, cx + r / 2, cy - r * 4 / 5 + r, cx + r * 5 / 4,
                cy - r * 4 / 5 + r, c);
            break;
        case ACT_FASTBOOT:
            quad(cx - r / 5, cy - r * 3 / 5, cx + r / 5, cy - r / 5, cx - r / 10, cy - r / 5,
                 cx - r / 2, cy + r * 3 / 5, c);
            quad(cx + r / 10, cy - r * 3 / 5, cx + r / 2, cy - r * 6 / 5, cx + r * 3 / 5,
                 cy - r * 6 / 5, cx + r / 10, cy - r / 5, c);
            break;
        case ACT_DOWNLOAD:
            fill_rect(cx - t, cy - r * 3 / 5, t * 2, r, c);
            tri(cx - r / 2, cy - r / 5, cx - r / 2, cy + r / 5, cx, cy + r / 5 + r / 2, c);
            tri(cx + r / 2, cy - r / 5, cx + r / 2, cy + r / 5, cx, cy + r / 5 + r / 2, c);
            fill_rect(cx - r * 3 / 5, cy + r * 3 / 5, r * 6 / 5, t, c);
            break;
        case ACT_REBOOT:
            arc(cx, cy, r - t, t * 2, 30, 320, c);
            tri(cx - r * 5 / 4, cy - r / 2, cx - r * 5 / 4, cy + r / 3, cx - r / 2, cy - r / 8, c);
            break;
        default:
            arc(cx, cy, r - t, t * 2, 120, 420, c);
            fill_rect(cx - t, cy - r, t * 2, r + r / 2, c);
            break;
    }
}

static void card_geom(float k, int i, int *x, int *y, int *w, int *h) {
    int pad = (int)((float)G.w * 0.052f);
    *x = pad;
    *w = (int)G.w - pad * 2;
    *h = (int)((float)G.h * 0.093f);
    int gap = (int)((float)G.h * 0.0085f);
    int top = (int)((float)G.h * 0.142f);
    *y = top + i * (*h + gap);
    (void)k;
}

static i64 countdown_left(void) {
    if (!U.countdown_on) return 0;
    i64 el = jbm_now_ms() - U.t0 - JBM_INTRO_MS;
    i64 left = U.countdown_ms - el;
    return left < 0 ? 0 : left;
}

static void draw_ui(void) {
    float k = ui_scale();
    i64 now = jbm_now_ms();
    i64 elapsed = now - U.t0;

    draw_bg(k);

    int pad = (int)((float)G.w * 0.052f);
    i64 intro_el = elapsed_ms() < JBM_INTRO_MS ? elapsed_ms() : JBM_INTRO_MS;
    int slide = (int)((JBM_INTRO_MS - intro_el) * 100 / JBM_INTRO_MS);
    int ey = slide * (int)((float)G.h * 0.03f) / 100;

    draw_text(&FN48, pad, (int)((float)G.h * 0.042f) + ey, 48.0f * k, RGBA(0xEAF0FAFF), "Boot Manager", TA_L | VA_M);
    draw_text(&FN20, pad, (int)((float)G.h * 0.072f) + ey, 20.0f * k, RGBA(0x6E7C93FF), "CUSTOM BOOT MENU",
              TA_L | VA_M);
    draw_text(&FN20, (int)G.w - pad, (int)((float)G.h * 0.045f) + ey, 20.0f * k,
              (act_color[U.sel] | 0xFF000000u), JBM_VERSION, TA_R | VA_M);

    fill_rect(pad, (int)((float)G.h * 0.112f) + ey, (int)(3.0f * k) + 1, (int)((float)G.h * 0.018f),
              RGBA(0x2A3446FF));

    if (U.busy) {
        for (int i = 0; i < ACT_COUNT; i++) {
            int x, y, w, h;
            card_geom(k, i, &x, &y, &w, &h);
            i64 start = 60 + i * 55;
            i64 a = elapsed - start;
            if (a < 0) continue;
            int alpha = (int)(255 * (a < 260 ? (a < 0 ? 0 : a) / 260.0f : 1.0f));
            if (alpha < 0) alpha = 0;
            if (alpha > 255) alpha = 255;
            if (alpha < 6) continue;
            int off = (int)((260 - a > 0 ? (260 - a) / 260.0f : 0.0f) * (float)G.w * 0.06f);
            int sel = (i == U.sel);
            u32 base = sel ? RGBA(0x19212FFFu) : RGBA(0x111823FFu);
            round_rect(x + off, y, w, h, (int)((float)h * 0.22f), base);
            round_rect_border(x + off, y, w, h, (int)((float)h * 0.22f), (int)(2.0f * k) + 1,
                              (sel ? ((u32)act_color[i] | 0xFF000000u) : RGBA(0x232E3FFFu)));
            if (sel) {
                fill_rect(x + off, y + (int)((float)h * 0.22f) - (int)(10.0f * k), (int)(5.0f * k),
                          h - (int)((float)h * 0.44f) + (int)(20.0f * k),
                          (act_color[i] & 0xFFFFFF) | ((u32)alpha << 24));
            }
            int icy = y + h / 2;
            int ir = (int)((float)h * 0.28f);
            disc(x + off + (int)((float)h * 0.38f), icy, ir, (u32)((act_color[i] & 0xFFFFFF) | 0x24000000u));
            draw_icon(i, x + off + (int)((float)h * 0.38f), icy, ir,
                      (act_color[i] & 0xFFFFFF) | ((u32)alpha << 24));
            draw_text(&FN32, x + off + (int)((float)h * 0.72f), icy - (int)(14.0f * k), 34.0f * k,
                      0xEAF0FAu | ((u32)alpha << 24), act_label[i], TA_L | VA_B);
            draw_text(&FN20, x + off + (int)((float)h * 0.72f), icy + (int)(12.0f * k), 20.0f * k,
                      0x8A97ACu | ((u32)(alpha * 200 / 255) << 24), act_sub[i], TA_L | VA_T);
            if (sel) {
                int ax = x + off + w - (int)((float)h * 0.34f);
                disc(ax, icy, (int)(9.0f * k) + 2, (act_color[i] & 0xFFFFFF) | ((u32)alpha << 24));
                ring(ax, icy, (int)((float)h * 0.19f), (int)(3.0f * k) + 1,
                     (act_color[i] & 0xFFFFFF) | 0x38000000u);
            }
        }

        int fy = (int)((float)G.h * 0.80f);
        draw_text(&FN32, pad, fy, 34.0f * k, RGBA(0xEAF0FAFF), U.status, TA_L | VA_M);
        int bw = (int)G.w - pad * 2;
        fill_rect(pad, fy + (int)((float)G.h * 0.024f), bw, (int)((float)G.h * 0.0042f) + 1, RGBA(0x1B2431FF));
        int seg = bw / 5;
        int phase = (int)((now / 80) % 5);
        for (int i = 0; i < 5; i++)
            fill_rect(pad + i * seg + seg / 5, fy + (int)((float)G.h * 0.024f), seg * 3 / 5,
                      (int)((float)G.h * 0.0042f) + 1, i == phase ? (act_color[U.sel] | 0xFF000000u)
                                                                  : RGBA(0x1B2431FF));
        return;
    }

    for (int i = 0; i < ACT_COUNT; i++) {
        int x, y, w, h;
        card_geom(k, i, &x, &y, &w, &h);
        i64 start = 60 + i * 55;
        i64 a = elapsed - start;
        if (a < 0) continue;
        int ease = a < 300 ? (int)(a / 300.0f * 255.0f) : 255;
        if (ease < 0) ease = 0;
        if (ease > 255) ease = 255;
        int off = (int)((255 - ease) / 255.0f * (float)G.w * 0.08f);
        int sel = (i == U.sel);
        int radius = (int)((float)h * 0.22f);

        int lift = sel ? (int)((float)G.h * 0.0018f) : 0;
        int cy = y - lift;

        u32 card = sel ? RGBA(0x18212FFFu) : RGBA(0x111823FFu);
        round_rect(x + off, cy, w, h, radius, card);
        round_rect_border(x + off, cy, w, h, radius, (int)(2.0f * k) + 1,
                          sel ? ((act_color[i] & 0xFFFFFF) | 0xCD000000u) : RGBA(0x232E3FFFu));

        if (sel) fill_rect(x + off, cy + radius - (int)(10.0f * k), (int)(5.0f * k),
                           h - radius * 2 + (int)(20.0f * k), act_color[i] | 0xFF000000u);

        int icy = cy + h / 2;
        int icx = x + off + (int)((float)h * 0.38f);
        int ir = (int)((float)h * 0.28f);
        disc(icx, icy, ir, (act_color[i] & 0xFFFFFF) | 0x26000000u);
        draw_icon(i, icx, icy, ir, act_color[i] | 0xFF000000u);

        if (i == ACT_SYSTEM) {
            i64 left = countdown_left();
            i64 secs = (left + 999) / 1000;
            char num[8];
            num[0] = (char)('0' + (int)(secs / 10) % 10);
            num[1] = (char)('0' + (int)(secs % 10));
            num[2] = 0;
            draw_text(&FDIG, (int)G.w - pad - (int)((float)G.w * 0.13f), icy, 132.0f * k,
                      (act_color[i] & 0xFFFFFF) | (U.countdown_on ? 0xFF000000u : 0x60000000u), num,
                      TA_R | VA_M);
        }

        draw_text(&FN32, x + off + (int)((float)h * 0.72f), icy - (int)(13.0f * k), 34.0f * k,
                  RGBA(0xEAF0FAFF), act_label[i], TA_L | VA_B);
        draw_text(&FN20, x + off + (int)((float)h * 0.72f), icy + (int)(13.0f * k), 20.0f * k,
                  RGBA(0x8A97ACFF), act_sub[i], TA_L | VA_T);

        if (sel) {
            int ax = x + off + w - (int)((float)h * 0.34f);
            int dot = (int)(9.0f * k) + 2;
            int rr = (int)((float)h * 0.20f);
            if (U.hold_idx == i) {
                i64 held = now - U.hold_start;
                i64 need = act_hold_ms[i] > 0 ? act_hold_ms[i] : 1;
                int sweep = (int)((float)rr * (float)(held % 1200) / 1200.0f);
                if (sweep < 1) sweep = 1;
                ring(ax, icy, rr, (int)(4.0f * k) + 1, RGBA(0x1F6FEB4Fu));
                ring(ax, icy, rr, (int)(4.0f * k) + 1, (act_color[i] & 0xFFFFFF) | 0xFF000000u);
                arc(ax, icy, rr, (int)(5.0f * k) + 2, -90, -90 + (int)(360.0f * (float)held / (float)need),
                    act_color[i] | 0xFF000000u);
                (void)sweep;
            } else {
                ring(ax, icy, rr, (int)(3.0f * k) + 1, (act_color[i] & 0xFFFFFF) | 0x40000000u);
            }
            disc(ax, icy, dot, act_color[i] | 0xFF000000u);
        }
    }

    int fy = (int)((float)G.h * 0.855f);
    if (U.countdown_on) {
        i64 left = countdown_left();
        char lbl[160];
        jbm_sn(lbl, "Auto-starting System in %llds", (long long)((left + 999) / 1000));
        draw_text(&FN20, pad, fy, 20.0f * k, RGBA(0x6E7C93FF), lbl, TA_L | VA_M);
        int bw = (int)G.w - pad * 2;
        int by = (int)((float)G.h * 0.885f);
        fill_rect(pad, by, bw, (int)((float)G.h * 0.0035f) + 1, RGBA(0x1B2431FF));
        i64 total = U.countdown_ms > 0 ? U.countdown_ms : 1;
        i64 prog = total - left;
        if (prog < 0) prog = 0;
        if (prog > total) prog = total;
        fill_rect(pad, by, (int)((i64)bw * prog / total), (int)((float)G.h * 0.0035f) + 1, RGBA(0x34D399EB));
    } else {
        draw_text(&FN20, pad, fy, 20.0f * k, RGBA(0x6E7C93FF), "Auto-start cancelled - pick an option",
                  TA_L | VA_M);
    }
    draw_text(&FN20, pad, (int)((float)G.h * 0.915f), 20.0f * k, RGBA(0x4E5A6EFF),
              "Tap System to boot now. Hold Recovery, Download, Fastboot, Reboot or Power off.", TA_L | VA_M);
}

static int hit_card(int px, int py) {
    float k = ui_scale();
    for (int i = 0; i < ACT_COUNT; i++) {
        int x, y, w, h;
        card_geom(k, i, &x, &y, &w, &h);
        if (px >= x && px < x + w && py >= y && py < y + h) return i;
    }
    return -1;
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
    /* TODO(hardware): unmounting /dev before execve may break the Android
     * first-stage init, which can expect /dev to stay populated. The fds above
     * are now closed, so the umount should no longer fail with EBUSY. Needs
     * testing on the device; consider leaving /dev mounted. */
    jbm_umount("/sys");
    jbm_umount("/proc");
    jbm_umount("/dev");
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
    jbm_mkdir("/dev", 0755);
    if (jbm_mount("tmpfs", "/dev", "tmpfs", 0, NULL) != 0)
        jbm_log("jbm: tmpfs on /dev failed, reusing ramdisk /dev");
    jbm_mknod("/dev/console", 0600, (5 << 8) | 1);
    jbm_mknod("/dev/kmsg", 0600, (1 << 8) | 11);
    jbm_mknod("/dev/null", 0666, (1 << 8) | 3);
#endif
    /* -1 leaves jbm_log() free to retry later if the console node was not
     * available yet when dev_init() ran. */
    jbm_klog_fd = -1;
}

#ifdef JBM_HOST
/* Host preview: renders the real draw_ui() path into an off-screen ARGB8888
 * canvas and writes a binary PPM, so layout and colours can be checked without
 * a device. Usage: jbm_host [out.ppm] [at_ms_into_countdown]
 */
static void host_preview(const char *path, i64 at_ms) {
    G.w = 1080;
    G.h = 2400;
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

    U.sel = ACT_SYSTEM;
    U.hold_idx = -1;
    U.t0 = jbm_now_ms() - at_ms;
    U.countdown_ms = JBM_AUTOBOOT_SEC * 1000;
    U.countdown_on = JBM_AUTOBOOT_SEC > 0;

    draw_ui();

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
    fprintf(stderr, "[jbm:host] wrote %s (%ux%u, countdown %llds left)\n", path, G.w, G.h,
            (long long)((countdown_left() + 999) / 1000));
}

int main(int argc, char **argv) {
    fprintf(stderr, "[jbm:host] argc=%d argv0=%s\n", argc, argc > 0 ? argv[0] : "-");
    dev_init();
    if (argc >= 2) {
        i64 at_ms = argc >= 3 ? (i64)atoi(argv[2]) : 0;
        host_preview(argv[1], at_ms);
        return 0;
    }
    touch_open();
    fprintf(stderr, "[jbm:host] gfx_ok=%d touch_fd=%d\n", G.ok, T.fd);
    fprintf(stderr, "[jbm:host] usage: %s out.ppm [at_ms]\n", argv[0]);
    return 0;
}
#endif

int jbm_main(int argc, char **argv, char **envp) {
    dev_init();
    jbm_mount("proc", "/proc", "proc", 0, NULL);
    jbm_mount("sysfs", "/sys", "sysfs", 0, NULL);
    jbm_log("jbm: v%s start argc=%d argv0=%s", JBM_VERSION, argc, argc > 0 && argv ? argv[0] : "-");

    int fb = 0;
    for (int i = 0; i < 60; i++) {
        if (gfx_open() == 0) {
            fb = 1;
            break;
        }
        jbm_sleep_ms(50);
    }
    if (!fb) {
        jbm_log("jbm: framebuffer unavailable, booting Android directly");
        cleanup_and_chain(argv, envp, "no framebuffer");
    }

    touch_open();

    U.sel = ACT_SYSTEM;
    U.hold_idx = -1;
    U.t0 = jbm_now_ms();
    U.countdown_ms = JBM_AUTOBOOT_SEC * 1000;
    U.countdown_on = JBM_AUTOBOOT_SEC > 0;

    i64 start = U.t0;
    i64 last_draw = 0;

    for (;;) {
        i64 now = jbm_now_ms();
        touch_poll();

        if (T.pressed) {
            U.countdown_on = 0;
            int idx = hit_card(T.sx, T.sy);
            if (idx >= 0) {
                U.sel = idx;
                U.hold_idx = (act_hold_ms[idx] > 0) ? idx : -1;
                if (U.hold_idx >= 0) U.hold_start = now;
            }
        }

        if (U.hold_idx >= 0) {
            int idx = hit_card(T.sx, T.sy);
            if (idx != U.hold_idx) {
                U.hold_idx = -1;
            } else if (now - U.hold_start >= act_hold_ms[U.hold_idx]) {
                int go = U.hold_idx;
                U.hold_idx = -1;
                U.busy = 1;
                jbm_sn(U.status, "Starting %s...", act_label[go]);
                for (int f = 0; f < 3; f++) {
                    draw_ui();
                    present();
                    jbm_sleep_ms(70);
                }
                jbm_log("jbm: action %s confirmed", act_label[go]);
                perform(go, argv, envp);
            }
        }

        if (T.released) {
            int idx = hit_card(T.tap_x, T.tap_y);
            if (idx >= 0 && act_hold_ms[idx] == 0 && U.hold_idx < 0) {
                U.sel = idx;
                U.busy = 1;
                jbm_sn(U.status, "Starting %s...", act_label[idx]);
                for (int f = 0; f < 3; f++) {
                    draw_ui();
                    present();
                    jbm_sleep_ms(70);
                }
                jbm_log("jbm: action %s tapped", act_label[idx]);
                perform(idx, argv, envp);
            }
            U.hold_idx = -1;
        }

        if (U.countdown_on && elapsed_ms() >= JBM_INTRO_MS && countdown_left() <= 0) {
            cleanup_and_chain(argv, envp, "countdown elapsed");
        }

        if (now - start > (i64)JBM_TOTAL_TIMEOUT_SEC * 1000) {
            cleanup_and_chain(argv, envp, "safety timeout");
        }

        if (now - last_draw > 33) {
            draw_ui();
            present();
            last_draw = now;
        } else {
            jbm_sleep_ms(8);
        }
    }
    return 0;
}
