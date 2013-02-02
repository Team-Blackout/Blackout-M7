/*
 *  arch/arm/include/asm/uaccess.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ASMARM_UACCESS_H
#define _ASMARM_UACCESS_H

#include <linux/string.h>
#include <linux/thread_info.h>
#include <asm/errno.h>
#include <asm/memory.h>
#include <asm/domain.h>
#include <asm/unified.h>
#include <asm/compiler.h>

#define VERIFY_READ 0
#define VERIFY_WRITE 1


struct exception_table_entry
{
	unsigned long insn, fixup;
};

extern int fixup_exception(struct pt_regs *regs);

extern int __get_user_bad(void);
extern int __put_user_bad(void);

#define KERNEL_DS	0x00000000
#define get_ds()	(KERNEL_DS)

#ifdef CONFIG_MMU

#define USER_DS		TASK_SIZE
#define get_fs()	(current_thread_info()->addr_limit)

static inline void set_fs(mm_segment_t fs)
{
	current_thread_info()->addr_limit = fs;
	modify_domain(DOMAIN_KERNEL, fs ? DOMAIN_CLIENT : DOMAIN_MANAGER);
}

#define segment_eq(a,b)	((a) == (b))

#define __addr_ok(addr) ({ \
	unsigned long flag; \
	__asm__("cmp %2, %0; movlo %0, #0" \
		: "=&r" (flag) \
		: "0" (current_thread_info()->addr_limit), "r" (addr) \
		: "cc"); \
	(flag == 0); })

#define __range_ok(addr,size) ({ \
	unsigned long flag, roksum; \
	__chk_user_ptr(addr);	\
	__asm__("adds %1, %2, %3; sbcccs %1, %1, %0; movcc %0, #0" \
		: "=&r" (flag), "=&r" (roksum) \
		: "r" (addr), "Ir" (size), "0" (current_thread_info()->addr_limit) \
		: "cc"); \
	flag; })

extern int __get_user_1(void *);
extern int __get_user_2(void *);
extern int __get_user_4(void *);

#define __get_user_x(__r2,__p,__e,__s,__i...)				\
	   __asm__ __volatile__ (					\
		__asmeq("%0", "r0") __asmeq("%1", "r2")			\
		"bl	__get_user_" #__s				\
		: "=&r" (__e), "=r" (__r2)				\
		: "0" (__p)						\
		: __i, "cc")

#define get_user(x,p)							\
	({								\
		register const typeof(*(p)) __user *__p asm("r0") = (p);\
		register unsigned long __r2 asm("r2");			\
		register int __e asm("r0");				\
		switch (sizeof(*(__p))) {				\
		case 1:							\
			__get_user_x(__r2, __p, __e, 1, "lr");		\
	       		break;						\
		case 2:							\
			__get_user_x(__r2, __p, __e, 2, "r3", "lr");	\
			break;						\
		case 4:							\
	       		__get_user_x(__r2, __p, __e, 4, "lr");		\
			break;						\
		default: __e = __get_user_bad(); break;			\
		}							\
		x = (typeof(*(p))) __r2;				\
		__e;							\
	})

extern int __put_user_1(void *, unsigned int);
extern int __put_user_2(void *, unsigned int);
extern int __put_user_4(void *, unsigned int);
extern int __put_user_8(void *, unsigned long long);

#define __put_user_x(__r2,__p,__e,__s)					\
	   __asm__ __volatile__ (					\
		__asmeq("%0", "r0") __asmeq("%2", "r2")			\
		"bl	__put_user_" #__s				\
		: "=&r" (__e)						\
		: "0" (__p), "r" (__r2)					\
		: "ip", "lr", "cc")

#define put_user(x,p)							\
	({								\
		register const typeof(*(p)) __r2 asm("r2") = (x);	\
		register const typeof(*(p)) __user *__p asm("r0") = (p);\
		register int __e asm("r0");				\
		switch (sizeof(*(__p))) {				\
		case 1:							\
			__put_user_x(__r2, __p, __e, 1);		\
			break;						\
		case 2:							\
			__put_user_x(__r2, __p, __e, 2);		\
			break;						\
		case 4:							\
			__put_user_x(__r2, __p, __e, 4);		\
			break;						\
		case 8:							\
			__put_user_x(__r2, __p, __e, 8);		\
			break;						\
		default: __e = __put_user_bad(); break;			\
		}							\
		__e;							\
	})

#else 

#define USER_DS			KERNEL_DS

#define segment_eq(a,b)		(1)
#define __addr_ok(addr)		(1)
#define __range_ok(addr,size)	(0)
#define get_fs()		(KERNEL_DS)

static inline void set_fs(mm_segment_t fs)
{
}

#define get_user(x,p)	__get_user(x,p)
#define put_user(x,p)	__put_user(x,p)

#endif 

#define access_ok(type,addr,size)	(__range_ok(addr,size) == 0)

#define user_addr_max() \
	(segment_eq(get_fs(), USER_DS) ? TASK_SIZE : ~0UL)

/*
 * The "__xxx" versions of the user access functions do not verify the
 * address space - it must have been done previously with a separate
 * "access_ok()" call.
 *
 * The "xxx_error" versions set the third argument to EFAULT if an
 * error occurs, and leave it unchanged on success.  Note that these
 * versions are void (ie, don't return a value as such).
 */
#define __get_user(x,ptr)						\
({									\
	long __gu_err = 0;						\
	__get_user_err((x),(ptr),__gu_err);				\
	__gu_err;							\
})

#define __get_user_error(x,ptr,err)					\
({									\
	__get_user_err((x),(ptr),err);					\
	(void) 0;							\
})

#define __get_user_err(x,ptr,err)					\
do {									\
	unsigned long __gu_addr = (unsigned long)(ptr);			\
	unsigned long __gu_val;						\
	__chk_user_ptr(ptr);						\
	switch (sizeof(*(ptr))) {					\
	case 1:	__get_user_asm_byte(__gu_val,__gu_addr,err);	break;	\
	case 2:	__get_user_asm_half(__gu_val,__gu_addr,err);	break;	\
	case 4:	__get_user_asm_word(__gu_val,__gu_addr,err);	break;	\
	default: (__gu_val) = __get_user_bad();				\
	}								\
	(x) = (__typeof__(*(ptr)))__gu_val;				\
} while (0)

#define __get_user_asm_byte(x,addr,err)				\
	__asm__ __volatile__(					\
	"1:	" TUSER(ldrb) "	%1,[%2],#0\n"			\
	"2:\n"							\
	"	.pushsection .fixup,\"ax\"\n"			\
	"	.align	2\n"					\
	"3:	mov	%0, %3\n"				\
	"	mov	%1, #0\n"				\
	"	b	2b\n"					\
	"	.popsection\n"					\
	"	.pushsection __ex_table,\"a\"\n"		\
	"	.align	3\n"					\
	"	.long	1b, 3b\n"				\
	"	.popsection"					\
	: "+r" (err), "=&r" (x)					\
	: "r" (addr), "i" (-EFAULT)				\
	: "cc")

#ifndef __ARMEB__
#define __get_user_asm_half(x,__gu_addr,err)			\
({								\
	unsigned long __b1, __b2;				\
	__get_user_asm_byte(__b1, __gu_addr, err);		\
	__get_user_asm_byte(__b2, __gu_addr + 1, err);		\
	(x) = __b1 | (__b2 << 8);				\
})
#else
#define __get_user_asm_half(x,__gu_addr,err)			\
({								\
	unsigned long __b1, __b2;				\
	__get_user_asm_byte(__b1, __gu_addr, err);		\
	__get_user_asm_byte(__b2, __gu_addr + 1, err);		\
	(x) = (__b1 << 8) | __b2;				\
})
#endif

#define __get_user_asm_word(x,addr,err)				\
	__asm__ __volatile__(					\
	"1:	" TUSER(ldr) "	%1,[%2],#0\n"			\
	"2:\n"							\
	"	.pushsection .fixup,\"ax\"\n"			\
	"	.align	2\n"					\
	"3:	mov	%0, %3\n"				\
	"	mov	%1, #0\n"				\
	"	b	2b\n"					\
	"	.popsection\n"					\
	"	.pushsection __ex_table,\"a\"\n"		\
	"	.align	3\n"					\
	"	.long	1b, 3b\n"				\
	"	.popsection"					\
	: "+r" (err), "=&r" (x)					\
	: "r" (addr), "i" (-EFAULT)				\
	: "cc")

#define __put_user(x,ptr)						\
({									\
	long __pu_err = 0;						\
	__put_user_err((x),(ptr),__pu_err);				\
	__pu_err;							\
})

#define __put_user_error(x,ptr,err)					\
({									\
	__put_user_err((x),(ptr),err);					\
	(void) 0;							\
})

#define __put_user_err(x,ptr,err)					\
do {									\
	unsigned long __pu_addr = (unsigned long)(ptr);			\
	__typeof__(*(ptr)) __pu_val = (x);				\
	__chk_user_ptr(ptr);						\
	switch (sizeof(*(ptr))) {					\
	case 1: __put_user_asm_byte(__pu_val,__pu_addr,err);	break;	\
	case 2: __put_user_asm_half(__pu_val,__pu_addr,err);	break;	\
	case 4: __put_user_asm_word(__pu_val,__pu_addr,err);	break;	\
	case 8:	__put_user_asm_dword(__pu_val,__pu_addr,err);	break;	\
	default: __put_user_bad();					\
	}								\
} while (0)

#define __put_user_asm_byte(x,__pu_addr,err)			\
	__asm__ __volatile__(					\
	"1:	" TUSER(strb) "	%1,[%2],#0\n"			\
	"2:\n"							\
	"	.pushsection .fixup,\"ax\"\n"			\
	"	.align	2\n"					\
	"3:	mov	%0, %3\n"				\
	"	b	2b\n"					\
	"	.popsection\n"					\
	"	.pushsection __ex_table,\"a\"\n"		\
	"	.align	3\n"					\
	"	.long	1b, 3b\n"				\
	"	.popsection"					\
	: "+r" (err)						\
	: "r" (x), "r" (__pu_addr), "i" (-EFAULT)		\
	: "cc")

#ifndef __ARMEB__
#define __put_user_asm_half(x,__pu_addr,err)			\
({								\
	unsigned long __temp = (unsigned long)(x);		\
	__put_user_asm_byte(__temp, __pu_addr, err);		\
	__put_user_asm_byte(__temp >> 8, __pu_addr + 1, err);	\
})
#else
#define __put_user_asm_half(x,__pu_addr,err)			\
({								\
	unsigned long __temp = (unsigned long)(x);		\
	__put_user_asm_byte(__temp >> 8, __pu_addr, err);	\
	__put_user_asm_byte(__temp, __pu_addr + 1, err);	\
})
#endif

#define __put_user_asm_word(x,__pu_addr,err)			\
	__asm__ __volatile__(					\
	"1:	" TUSER(str) "	%1,[%2],#0\n"			\
	"2:\n"							\
	"	.pushsection .fixup,\"ax\"\n"			\
	"	.align	2\n"					\
	"3:	mov	%0, %3\n"				\
	"	b	2b\n"					\
	"	.popsection\n"					\
	"	.pushsection __ex_table,\"a\"\n"		\
	"	.align	3\n"					\
	"	.long	1b, 3b\n"				\
	"	.popsection"					\
	: "+r" (err)						\
	: "r" (x), "r" (__pu_addr), "i" (-EFAULT)		\
	: "cc")

#ifndef __ARMEB__
#define	__reg_oper0	"%R2"
#define	__reg_oper1	"%Q2"
#else
#define	__reg_oper0	"%Q2"
#define	__reg_oper1	"%R2"
#endif

#define __put_user_asm_dword(x,__pu_addr,err)			\
	__asm__ __volatile__(					\
 ARM(	"1:	" TUSER(str) "	" __reg_oper1 ", [%1], #4\n"	) \
 ARM(	"2:	" TUSER(str) "	" __reg_oper0 ", [%1]\n"	) \
 THUMB(	"1:	" TUSER(str) "	" __reg_oper1 ", [%1]\n"	) \
 THUMB(	"2:	" TUSER(str) "	" __reg_oper0 ", [%1, #4]\n"	) \
	"3:\n"							\
	"	.pushsection .fixup,\"ax\"\n"			\
	"	.align	2\n"					\
	"4:	mov	%0, %3\n"				\
	"	b	3b\n"					\
	"	.popsection\n"					\
	"	.pushsection __ex_table,\"a\"\n"		\
	"	.align	3\n"					\
	"	.long	1b, 4b\n"				\
	"	.long	2b, 4b\n"				\
	"	.popsection"					\
	: "+r" (err), "+r" (__pu_addr)				\
	: "r" (x), "i" (-EFAULT)				\
	: "cc")


#ifdef CONFIG_MMU
extern unsigned long __must_check __copy_from_user(void *to, const void __user *from, unsigned long n);
extern unsigned long __must_check __copy_to_user(void __user *to, const void *from, unsigned long n);
extern unsigned long __must_check __copy_to_user_std(void __user *to, const void *from, unsigned long n);
extern unsigned long __must_check __clear_user(void __user *addr, unsigned long n);
extern unsigned long __must_check __clear_user_std(void __user *addr, unsigned long n);
#else
#define __copy_from_user(to,from,n)	(memcpy(to, (void __force *)from, n), 0)
#define __copy_to_user(to,from,n)	(memcpy((void __force *)to, from, n), 0)
#define __clear_user(addr,n)		(memset((void __force *)addr, 0, n), 0)
#endif

static inline unsigned long __must_check copy_from_user(void *to, const void __user *from, unsigned long n)
{
	if (access_ok(VERIFY_READ, from, n))
		n = __copy_from_user(to, from, n);
	else 
		memset(to, 0, n);
	return n;
}

static inline unsigned long __must_check copy_to_user(void __user *to, const void *from, unsigned long n)
{
	if (access_ok(VERIFY_WRITE, to, n))
		n = __copy_to_user(to, from, n);
	return n;
}

#define __copy_to_user_inatomic __copy_to_user
#define __copy_from_user_inatomic __copy_from_user

static inline unsigned long __must_check clear_user(void __user *to, unsigned long n)
{
	if (access_ok(VERIFY_WRITE, to, n))
		n = __clear_user(to, n);
	return n;
}

extern long strncpy_from_user(char *dest, const char __user *src, long count);

extern __must_check long strlen_user(const char __user *str);
extern __must_check long strnlen_user(const char __user *str, long n);

#endif 
