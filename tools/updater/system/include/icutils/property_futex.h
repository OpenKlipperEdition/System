#ifndef PROPERTY_FUTEX_H
#define PROPERTY_FUTEX_H

#include <unistd.h>
#include <linux/futex.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <errno.h>

#define ANDROID_MEMBAR_FULL() __asm__ __volatile__("" ::: "memory")

#define DISALLOW_COPY_AND_ASSIGN(TypeName)		\
	TypeName(const TypeName&) = delete;			\
	void operator=(const TypeName&) = delete

#define BIONIC_ALIGN(value, alignment)					\
	(((value) + (alignment) - 1) & ~((alignment) - 1))


#define	__predict_true(exp)	(exp)
#define	__predict_false(exp) (exp)

//#define __always_inline __attribute__((always_inline))
static  __always_inline int __futex(volatile void* ftx, int op, int value, const struct timespec* timeout) {
	// Our generated syscall assembler sets errno, but our callers (pthread functions) don't want to.
	int saved_errno = errno;
	int result = syscall(__NR_futex, ftx, op, value, timeout);
	if (__predict_false(result == -1)) {
		result = -errno;
		errno = saved_errno;
	}
	return result;
}

static inline int __futex_wake(volatile void* ftx, int count) {
	return __futex(ftx, FUTEX_WAKE, count, NULL);
}

static inline int __futex_wait(volatile void* ftx, int value, const struct timespec* timeout) {
	return __futex(ftx, FUTEX_WAIT, value, timeout);
}

#endif /* PROPERTY_FUTEX_H */
