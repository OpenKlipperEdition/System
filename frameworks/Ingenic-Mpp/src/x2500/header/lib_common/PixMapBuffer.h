/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufferPixMapMeta.h"
#include "lib_common/BufferAPI.h"

/*************************************************************************//*!
   \brief Plane parameters
*****************************************************************************/
typedef struct AL_t_PlaneDescription
{
  AL_EPlaneId ePlaneId; /*!< Type of plane */
  int iOffset;          /*!< Offset of the plane from beginning of the buffer chunk (in bytes) */
  int iPitch;           /*!< Pitch of the plane (in bytes) */
  size_t zSize;         /*!< Size of the plane */
}AL_TPlaneDescription;

/*************************************************************************//*!
   \brief Creates an AL_TBuffer meant to store pixel planes.
   \param[in] pAllocator Pointer to an Allocator.
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \param[in] tDim Dimension of the picture (width and height in pixels)
   \param[in] tFourCC FourCC of the framebuffer
   \return Return a pointer the AL_TBuffer created if successfull, false
   otherwise
*****************************************************************************/
AL_TBuffer* AL_PixMapBuffer_Create(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack, AL_TDimension tDim, TFourCC tFourCC);

/*************************************************************************//*!
   \brief Allocate a continuous memory chunk to store planes, and add the
   planes to the buffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] zSize The size of the memory chunk to allocate
   \param[in] pPlDescriptions Pointer to a list a planes description
   \param[in] iNbPlanes Number of planes of the list
   \return Return true if memory has been allocated and planes added, false
   otherwise
*****************************************************************************/
bool AL_PixMapBuffer_Allocate_And_AddPlanes(AL_TBuffer* pBuf, size_t zSize, bool bUseExtraBuf, void *extraBufHandle, AL_TPlaneDescription* pPlDesc, int iNbPlanes, char const* name);

/*************************************************************************//*!
   \brief Add an already allocated memory chunk to store planes, and add the
   planes to the buffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] hChunk Handle to an already allocated memory chunk (with pAllocator)
   \param[in] zSize The size of the memory chunk
   \param[in] pPlDescriptions Pointer to a list a planes description
   \param[in] iNbPlanes Number of planes of the list
   \return Return true if memory chunk and associated planes have been added
   successfully, false otherwise
*****************************************************************************/
bool AL_PixMapBuffer_AddPlanes(AL_TBuffer* pBuf, AL_HANDLE hChunk, size_t zSize, bool bUseExtraBuf, void *extraBufHandle, AL_TPlaneDescription* pPlDesc, int iNbPlanes);

/*************************************************************************//*!
   \brief Get the virtual address of a plane of a AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] ePlaneId Type of plane
   \return Returns a pointer to the specified plane if successfull, NULL otherwise
*****************************************************************************/
uint8_t* AL_PixMapBuffer_GetPlaneAddress(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Get the pitch of a plane of a AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] ePlaneId Type of plane
   \return Returns the pitch of the specified plane if successfull, 0 otherwise
*****************************************************************************/
int AL_PixMapBuffer_GetPlanePitch(AL_TBuffer const* pBuf, AL_EPlaneId ePlaneId);

/*************************************************************************//*!
   \brief Get the dimension of the frame stored in the AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \return Returns the dimension of the frame if successfull, null dimension
   otherwise
*****************************************************************************/
AL_TDimension AL_PixMapBuffer_GetDimension(AL_TBuffer const* pBuf);

/*************************************************************************//*!
   \brief Set the dimension of the frame stored in the AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] tDim The new dimension
   \return Returns true if dimension was correctly set, false otherwise
*****************************************************************************/
bool AL_PixMapBuffer_SetDimension(AL_TBuffer* pBuf, AL_TDimension tDim);

/*************************************************************************//*!
   \brief Get the FourCC of the frame stored in the AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \return Returns the FourCC of the frame if successfull, 0 otherwise
*****************************************************************************/
TFourCC AL_PixMapBuffer_GetFourCC(AL_TBuffer const* pBuf);

/*************************************************************************//*!
   \brief Set the FourCC of the frame stored in the AL_TBuffer
   \param[in] pBuf Pointer to the AL_TBuffer
   \param[in] tFourCC The new FourCC
   \return Returns true if FourCC was correctly set, false otherwise
*****************************************************************************/
bool AL_PixMapBuffer_SetFourCC(AL_TBuffer* pBuf, TFourCC tFourCC);

/*@*/

