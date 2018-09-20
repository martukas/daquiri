/*******************************************************************************
 * Copyright (c) PLX Technology, Inc.
 *
 * PLX Technology Inc. licenses this source file under the GNU Lesser General Public
 * License (LGPL) version 2.  This source file may be modified or redistributed
 * under the terms of the LGPL and without express permission from PLX Technology.
 *
 * PLX Technology, Inc. provides this software AS IS, WITHOUT ANY WARRANTY,
 * EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  PLX makes no guarantee
 * or representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL PLX BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * File Name:
 *
 *      PlxDefCk.h
 *
 * Description:
 *
 *      Verifies definitions required by the PLX API
 *
 * Revision:
 *
 *      05-01-12 : PLX SDK v7.00
 *
 ******************************************************************************/




/**********************************************
*       Automatic selection for Windows
**********************************************/
#if defined(_WIN32) || defined(_WIN64)
    #if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
        #define PLX_LITTLE_ENDIAN
    #endif

    #if defined(_WIN64)
        #define PLX_CPU_BITS    64
    #else
        #define PLX_CPU_BITS    32
    #endif

    #define PLX_MSWINDOWS
#endif


#if defined(PLX_LINUX)
    #if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
        #define PLX_LITTLE_ENDIAN
    #endif
#endif



/**********************************************
*               Error Checks
**********************************************/
#if !defined(PLX_LITTLE_ENDIAN) && !defined(PLX_BIG_ENDIAN)
    #error ERROR: Either PLX_LITTLE_ENDIAN or PLX_BIG_ENDIAN must be defined.
#endif
