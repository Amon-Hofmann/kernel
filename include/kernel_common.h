/* kernel_common.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#define _NONNULL     __attribute__((nonnull))
#define _INLINE      __attribute__((always_inline)) static inline
#define _NORETURN    __attribute__((noreturn))
#define _UNUSED      __attribute__((unused))
#define _PACKED      __attribute__((packed))
#define _COLD        __attribute__((cold))
#define _HOT         __attribute__((hot))
#define _WUNUSED     __attribute__((warn_unused_result))
#define _RET_NONNULL __attribute__((returns_nonnull))

#endif  // KERNEL_COMMON_H
