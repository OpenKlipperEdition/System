/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/*************************************************************************//*!
   \addtogroup Traces
   @{
   \file
*****************************************************************************/
#pragma once

/*************************************************************************//*!
   \brief Trace type enum
*****************************************************************************/
typedef enum e_TraceType
{
  AL_PARSING_INPUT_TRACE,
  AL_PARSING_OUTPUT_TRACE,
  AL_DECODING_INPUT_TRACE,
  AL_DECODING_OUTPUT_TRACE,
  AL_JPEG_INPUT_TRACE,
  AL_JPEG_OUTPUT_TRACE,
  AL_TRACE_TYPE_MAX_ENUM, /* sentinel */
}AL_ETraceType;

/*************************************************************************//*!
   \brief Output trace mode
*****************************************************************************/
typedef enum e_TraceMode
{
  AL_TRACE_NONE,
  AL_TRACE_ON_ERR,
  AL_TRACE_LATEST,
  AL_TRACE_ALL,
  AL_TRACE_FRAME,
  AL_TRACE_STATUS,
  AL_TRACE_MODE_MAX_ENUM, /* sentinel */
}AL_ETraceMode;

/*@}*/

