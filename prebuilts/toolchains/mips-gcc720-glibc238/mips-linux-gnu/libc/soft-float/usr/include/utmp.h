/* Copyright (C) 1993-2023 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#ifndef	_UTMP_H
#define	_UTMP_H	1

#include <features.h>

#include <sys/types.h>


__BEGIN_DECLS

/* Get system dependent values and data structures.  */
#include <bits/utmp.h>

/* Compatibility names for the strings of the canonical file names.  */
#define UTMP_FILE	_PATH_UTMP
#define UTMP_FILENAME	_PATH_UTMP
#define WTMP_FILE	_PATH_WTMP
#define WTMP_FILENAME	_PATH_WTMP



/* Make FD be the controlling terminal, stdin, stdout, and stderr;
   then close FD.  Returns 0 on success, nonzero on error.  */
extern int login_tty (int __fd) __THROW;


/* Write the given entry into utmp and wtmp.  */
#ifndef __USE_TIME_BITS64
extern void login (const struct utmp *__entry) __THROW;
#else
# ifdef __REDIRECT_NTH
extern void login64 (const struct utmp *__entry) __THROW;
extern void __REDIRECT_NTH (login, (const struct utmp *__entry),
                           login64) __THROW;
# else
#  define login login64
# endif
#endif

/* Write the utmp entry to say the user on UT_LINE has logged out.  */
extern int logout (const char *__ut_line) __THROW;

/* Append to wtmp an entry for the current time and the given info.  */
extern void logwtmp (const char *__ut_line, const char *__ut_name,
		     const char *__ut_host) __THROW;

/* Append entry UTMP to the wtmp-like file WTMP_FILE.  */
#ifndef __USE_TIME_BITS64
extern void updwtmp (const char *__wtmp_file, const struct utmp *__utmp)
     __THROW;
#else
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (updwtmp, (const char *__wtmp_file, const struct utmp *__utmp),
                           updwtmp64) __THROW;
# else
#  define updwtmp updwtmp64
# endif
#endif

/* Change name of the utmp file to be examined.  */
extern int utmpname (const char *__file) __THROW;

/* Read next entry from a utmp-like file.  */
#ifndef __USE_TIME_BITS64
extern struct utmp *getutent (void) __THROW;
#else
# ifdef __REDIRECT_NTH
extern struct utmp *__REDIRECT_NTH (getutent, (void),
                                    getutent64) __THROW;
# else
#  define getutid getutent64
# endif
#endif

/* Reset the input stream to the beginning of the file.  */
extern void setutent (void) __THROW;

/* Close the current open file.  */
extern void endutent (void) __THROW;

/* Search forward from the current point in the utmp file until the
   next entry with a ut_type matching ID->ut_type.  */
#ifndef __USE_TIME_BITS64
extern struct utmp *getutid (const struct utmp *__id) __THROW;
#else
# ifdef __REDIRECT_NTH
extern struct utmp *__REDIRECT_NTH (getutid, (const struct utmp *__id),
                                     getutid64) __THROW;
# else
#  define getutid getutid64
# endif
#endif

/* Search forward from the current point in the utmp file until the
   next entry with a ut_line matching LINE->ut_line.  */
#ifndef __USE_TIME_BITS64
extern struct utmp *getutline (const struct utmp *__line) __THROW;
#else
# ifdef __REDIRECT_NTH
extern struct utmp *__REDIRECT_NTH (getutline, (const struct utmp *__line),
                                     getutline64) __THROW;
# else
#  define getutline getutline64
# endif
#endif

/* Write out entry pointed to by UTMP_PTR into the utmp file.  */
#ifndef __USE_TIME_BITS64
extern struct utmp *pututline (const struct utmp *__utmp_ptr) __THROW;
#else
# ifdef __REDIRECT_NTH
extern struct utmp *__REDIRECT_NTH (pututline, (const struct utmp *__utmp_ptr),
                                     pututline64) __THROW;
# else
#  define pututline pututline64
# endif
#endif


#ifdef	__USE_MISC
/* Reentrant versions of the file for handling utmp files.  */
#ifndef __USE_TIME_BITS64
extern int getutent_r (struct utmp *__buffer, struct utmp **__result) __THROW;
#else
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (getutent_r, (struct utmp *__buffer, struct utmp **__result),
                                     getutent_r64) __THROW;
# else
#  define getutent_r getutent_r64
# endif
#endif

#ifndef __USE_TIME_BITS64
extern int getutid_r (const struct utmp *__id, struct utmp *__buffer,
		      struct utmp **__result) __THROW;
#else
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (getutid_r, (const struct utmp *__id, struct utmp *__buffer,
                                        struct utmp **__result), getutid_r64) __THROW;
# else
#  define getutid_r getutid_r64
# endif
#endif

#ifndef __USE_TIME_BITS64
extern int getutline_r (const struct utmp *__line,
			struct utmp *__buffer, struct utmp **__result) __THROW;
#else
# ifdef __REDIRECT_NTH
extern int __REDIRECT_NTH (getutline_r, (const struct utmp *__line, struct utmp *__buffer,
                                        struct utmp **__result), getutline_r64) __THROW;
# else
#  define getutline_r getutline_r64
# endif
#endif

#endif	/* Use misc.  */

__END_DECLS

#endif /* utmp.h  */
