/*
 * hv_compiler.h
 *
 * Compiler specific macros.
 *
 * azuepke, 2013-03-22
 */

#ifndef __HV_COMPILER_CW_H__
#define __HV_COMPILER_CW_H__

/* these are gcc 3+ specific macros,
 * but most free compilers like LLVM/Clang, PCC or TinyCC also understand them
 */

/** compiler optimization barrier */
#define barrier()			__asm__ volatile("" : : : "memory")

/** optimization hints */
#define likely(cond)		__builtin_expect((cond) != 0, 1)
#define unlikely(cond)		__builtin_expect((cond) != 0, 0)
#define const_var(var)		__builtin_constant_p(var)
#if __GNUC_MAJOR__ >= 4 && __GNUC_MINOR__ >= 5
#define unreachable()		__builtin_unreachable()
#else
#define unreachable()		do { } while (1)
#endif

/** workaround for broken GCC 3.x analysis of uninitialized variables */
#if __GNUC_MAJOR__ <= 3
#define GCC_NOWARN_UNINITIALIZED(x)	x = 0
#else
#define GCC_NOWARN_UNINITIALIZED(x)	x
#endif


/** common attributes */
#define __packed			__attribute__((__packed__))
#define __unused			__attribute__((__unused__))
#define __used				__attribute__((__used__))
#define __noinline			__attribute__((__noinline__))
#define __alwaysinline		__attribute__((__always_inline__))
#define __noreturn			__attribute__((__noreturn__))

/* HACK */
#define __pure

#define __const				__attribute__((__const__))


/* HACK */
#define __section(s)

#define __aligned(a)		__attribute__((__aligned__(a)))
#define __wur				__attribute__((__warn_unused_result__))
#define __nonnull(x...)		__attribute__((__nonnull__(x)))
#if __GNUC_MAJOR__ >= 4 && __GNUC_MINOR__ >= 9
#define __returns_nonnull	__attribute__((__returns_nonnull__))
#else
#define __returns_nonnull
#endif
#define __alloc_size(x...)	__attribute__((__alloc_size__(x)))
#define __malloc_type		__attribute__((__malloc__))

/** Tricore specific */
#ifdef __tricore__
#define __interrupt			__attribute__((__interrupt__))
#define __fardata			__attribute__((__fardata__))
#define __longcall			__attribute__((__longcall__))
#endif

/** linkage */
#define __weak				__attribute__((__weak__))
#define __weakref(s)		__attribute__((__weak__, __alias__(#s)))
#define __alias(s)			__attribute__((__alias__(#s)))

/** hot or not? */
/* HACK */
#define __cold
#define __init

/** special sections for stacks and register contexts */
#define __section_stack				__section(.bss.stack)
#define __section_stack_core(x)		__section(.core ## x.bss.stack)
#define __section_kern_stack_core(x)	__section(.core ## x.bss.kern_stack)
#define __section_nmi_stack_core(x)		__section(.core ## x.bss.nmi_stack)
#define __section_context_core(x)	__section(.core ## x.bss.context)
#define __section_reg_core(x)		__section(.core ## x.bss.reg)
#define __section_bss_core(x)		__section(.core ## x.bss)
#define __section_data_core(x)		__section(.core ## x.data)
#define __section_sched_state_core(x)	__section(.core ## x.bss.sched_state)
#define __section_cfg				__section(.rodata.cfg)
#define __section_cfg_pt1			__section(.rodata.pt1)
#define __section_cfg_pt2			__section(.rodata.pt2)


/** preprocessor stringify magic */
#define __stringify2(x...)	#x
#define __stringify(x...)	__stringify2(x)


/** concatenate and expand macros */
#define __concatenate2(a,b)	a ## b
#define __concatenate(a,b)	__concatenate2(a, b)


/** formatting */
/* HACK */
#define __printflike(n,m)

/** offsetof() */
#if __GNUC_MAJOR__ >= 4
#define offsetof(type, member)	__builtin_offsetof(type, member)
#else
#define offsetof(type, member)	((size_t)(&((type *)0)->member))
#endif

/** typeof() */
#define typeof	__typeof__

/** countof() */
#define countof(a)	(sizeof(a) / sizeof((a)[0]))

/** container_of() */
/* see http://www.kroah.com/log/linux/container_of.html */
#define container_of(ptr, type, member) ({	\
		typeof(((type *)0)->member) *__mptr = (ptr);	\
		(type *)((char *)__mptr - offsetof(type, member));	\
	})

/** const_container_of() */
#define const_container_of(ptr, type, member) ({	\
		typeof(((const type *)0)->member) *__mptr = (ptr);	\
		(const type *)((const char *)__mptr - offsetof(type, member));	\
	})


/** Special ABI conventions for system call wrappers */
#ifdef __tricore__
#define __syscall
#define __tc_fastcall		__interrupt
#else
#define __syscall
#define __tc_fastcall
#endif

/** calling function */
#define __caller() __builtin_return_address(0)

#endif
