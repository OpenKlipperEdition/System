/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Errors

   This regroups all the errors and warnings that can be launched
   from the decoder or the encoder libraries.

   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"

#define AL_DEF_WARNING(N) ((AL_ERR)(0x00 + (N)))
#define AL_DEF_ERROR(N) ((AL_ERR)(0x80 + (N)))

enum
{
  /*! The decoder had to conceal some errors in the stream */
  AL_WARN_CONCEAL_DETECT = AL_DEF_WARNING(1),
  /*! Some LCU exceed the maximum allowed bits in the stream */
  AL_WARN_LCU_OVERFLOW = AL_DEF_WARNING(2),
  /*! Number of slices have been adjusted to be hardware compatible */
  AL_WARN_NUM_SLICES_ADJUSTED = AL_DEF_WARNING(3),
  /*! Sps not compatible with channel settings, decoder discards it */
  AL_WARN_SPS_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS = AL_DEF_WARNING(4),
  /*! Sei metadata buffer is too small to contains all sei messages */
  AL_WARN_SEI_OVERFLOW = AL_DEF_WARNING(5),
};

enum
{
  /*! The operation succeeded without encountering any error */
  AL_SUCCESS = ((AL_ERR)0x00000000),
  /*! Unknown error */
  AL_ERROR = AL_DEF_ERROR(0),
  /*! Couldn't allocate a resource because no memory was left
   * This can be dma memory, mcu specific memory if available or
   * simply virtual memory shortage */
  AL_ERR_NO_MEMORY = AL_DEF_ERROR(7),
  /*! The generated stream couldn't fit inside the allocated stream buffer */
  AL_ERR_STREAM_OVERFLOW = AL_DEF_ERROR(8),
  /*! If SliceSize mode is supported, the constraint couldn't be respected
   * as too many slices were required to respect it */
  AL_ERR_TOO_MANY_SLICES = AL_DEF_ERROR(9),
  /*! The scheduler can't handle more channel (fixed limit of AL_SCHEDULER_MAX_CHANNEL) */
  AL_ERR_CHAN_CREATION_NO_CHANNEL_AVAILABLE = AL_DEF_ERROR(13),
  /*! The processing power of the available cores is insufficient to handle this channel */
  AL_ERR_CHAN_CREATION_RESOURCE_UNAVAILABLE = AL_DEF_ERROR(14),
  /*! Couldn't spread the load on enough cores (a special case of ERROR_RESOURCE_UNAVAILABLE)
   * or the load can't be spread so much (each core has a requirement on the minimal number
   * of resources it can handle) */
  AL_ERR_CHAN_CREATION_NOT_ENOUGH_CORES = AL_DEF_ERROR(15),
  /*! Some parameters in the request have an invalid value */
  AL_ERR_REQUEST_MALFORMED = AL_DEF_ERROR(16),
  /*! The command is not allowed in some configuration */
  AL_ERR_CMD_NOT_ALLOWED = AL_DEF_ERROR(17),
  /*! The value associated with the command is invalid (in the current configuration) */
  AL_ERR_INVALID_CMD_VALUE = AL_DEF_ERROR(18),
  /*! The generated intermediate buffer couldn't fit inside the allocated buffer */
  AL_ERR_INTERMEDIATE_BUFFER_OVERFLOW = AL_DEF_ERROR(19),
};

static AL_INLINE bool AL_IS_ERROR_CODE(AL_ERR eErrorCode)
{
  return eErrorCode >= AL_ERROR;
}

static AL_INLINE bool AL_IS_WARNING_CODE(AL_ERR eErrorCode)
{
  return (eErrorCode != AL_SUCCESS) && (eErrorCode < AL_ERROR);
}

static AL_INLINE bool AL_IS_SUCCESS_CODE(AL_ERR eErrorCode)
{
  return eErrorCode == AL_SUCCESS;
}

/*@}*/

