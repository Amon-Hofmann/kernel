/* kernel_common.h
 * Whatever this program does
 *
 * Author - Amon Hofmann
 */

#ifndef KERNEL_COMMON_H
#define KERNEL_COMMON_H

#define KERNEL_NONNULL     __attribute__((nonnull))
#define KERNEL_INLINE      __attribute__((always_inline)) static inline
#define KERNEL_NORETURN    __attribute__((noreturn))
#define KERNEL_UNUSED      __attribute__((unused))
#define KERNEL_PACKED      __attribute__((packed))
#define KERNEL_COLD        __attribute__((cold))
#define KERNEL_HOT         __attribute__((hot))
#define KERNEL_WUNUSED     __attribute__((warn_unused_result))
#define KERNEL_RET_NONNULL __attribute__((returns_nonnull))
#define KERNEL_ALIGN_PAGE  __attribute__((aligned(4096)))

#endif  // KERNEL_COMMON_H
