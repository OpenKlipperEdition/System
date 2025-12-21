#ifndef UTILS_H
#define UTILS_H
#include <unistd.h>

#ifndef TEMP_FAILURE_RETRY
/* Used to retry syscalls that can return EINTR. */
#define TEMP_FAILURE_RETRY(exp) ({					\
			typeof (exp) _rc;						\
			do {									\
				_rc = (exp);						\
			} while (_rc == -1 && errno == EINTR);	\
			_rc; })
#endif

#endif /* UTILS_H */
