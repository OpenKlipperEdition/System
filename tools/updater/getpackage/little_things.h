/*
 * Copyright (C) 2016 Ingenic Semiconductor
 *
 * SunWenZhong(Fighter) <wenzhong.sun@ingenic.com, wanmyqawdr@126.com>
 *
 * Little things from everywhere, Thank you for your work!
 *
 * For project-5
 *
 * Release under GPLv2
 *
 */


#ifndef LITTLE_THINGS_H
#define LITTLE_THINGS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef UNUSED
#define UNUSED(x) ((void)(x))

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * @breif cast a member of a structure out to the containing structure
 * @param ptr:    the pointer to the member.
 * @param type:   the type of the container struct this is embedded in.
 * @param member: the name of the member within the struct.
 *
 */
#undef container_of
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#undef address_offset
#define address_offset(void_ptr, offset) \
    ((void *)((uint8_t *)(void_ptr) + (offset)))

#undef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))

/**
 * @breif calculate the bit mask of a bit filed
 * @param start: start index of bit filed
 * @param end:   end index of bit filed
 * @details e.g BIT_MASK(0, 7) --> 0xff
 *              BIT_MASK(0, 0) --> 0x01
 *              BIT_MASK(1, 1) --> 0x02
 */
#define BIT_MASK(start, end) (((1 << ((end) - (start) + 1)) - 1) << (start))

/**
 * @breif modify the value, clear the bits_to_clr then set the bits_to_set
 * @param bits_to_clr: bits to clear, generally clear the mask of a bits filed
 * @param bits_to_set: bits to set, generally the val of a bits filed
 */
#define modify_val(val, bits_to_clr, bits_to_set)                   \
    do {                                                            \
        (val) = ((val) & ~(bits_to_clr)) | (bits_to_set);           \
    } while (0)

#undef true
#define true 1

#undef false
#define false 0

#define comac_arg_n( _0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,\
    _17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,\
    _N,...) _N
#define comac_get_args_cnt( ... ) comac_arg_n( __VA_ARGS__ )
#define comac_args_seqs() 36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,\
    16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

/**
 *  comac_argc: calculate macro args' count, maxim to 36
 */
#define comac_argc(...) comac_get_args_cnt( 0, ##__VA_ARGS__, comac_args_seqs())

#ifdef __cplusplus
}
#endif

#endif
