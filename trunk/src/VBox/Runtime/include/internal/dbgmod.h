/* $Id$ */
/** @file
 * IPRT - Internal Header for RTDbgMod and the associated interpreters.
 */

/*
 * Copyright (C) 2008-2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___internal_dbgmod_h
#define ___internal_dbgmod_h

#include <iprt/types.h>
#include <iprt/critsect.h>
#include "internal/magics.h"

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_dbgmod     RTDbgMod - Debug Module Interperter
 * @ingroup grp_rt
 * @internal
 * @{
 */


/** Pointer to the internal module structure. */
typedef struct RTDBGMODINT *PRTDBGMODINT;

/**
 * Virtual method table for executable image interpreters.
 */
typedef struct RTDBGMODVTIMG
{
    /** Magic number (RTDBGMODVTIMG_MAGIC). */
    uint32_t    u32Magic;
    /** Mask of supported executable image types, see grp_rt_exe_img_type.
     * Used to speed up the search for a suitable interpreter. */
    uint32_t    fSupports;
    /** The name of the interpreter. */
    const char *pszName;

    /**
     * Try open the image.
     *
     * This combines probing and opening.
     *
     * @returns VBox status code. No informational returns defined.
     *
     * @param   pMod        Pointer to the module that is being opened.
     *
     *                      The RTDBGMOD::pszDbgFile member will point to
     *                      the filename of any debug info we're aware of
     *                      on input. Also, or alternatively, it is expected
     *                      that the interpreter will look for debug info in
     *                      the executable image file when present and that it
     *                      may ask the image interpreter for this when it's
     *                      around.
     *
     *                      Upon successful return the method is expected to
     *                      initialize pDbgOps and pvDbgPriv.
     */
    DECLCALLBACKMEMBER(int, pfnTryOpen)(PRTDBGMODINT pMod);

    /**
     * Close the interpreter, freeing all associated resources.
     *
     * The caller sets the pDbgOps and pvDbgPriv RTDBGMOD members
     * to NULL upon return.
     *
     * @param   pMod        Pointer to the module structure.
     */
    DECLCALLBACKMEMBER(int, pfnClose)(PRTDBGMODINT pMod);

} RTDBGMODVTIMG;
/** Pointer to a const RTDBGMODVTIMG. */
typedef RTDBGMODVTIMG const *PCRTDBGMODVTIMG;


/**
 * Virtual method table for debug info interpreters.
 */
typedef struct RTDBGMODVTDBG
{
    /** Magic number (RTDBGMODVTDBG_MAGIC). */
    uint32_t    u32Magic;
    /** Mask of supported debug info types, see grp_rt_dbg_type.
     * Used to speed up the search for a suitable interpreter. */
    uint32_t    fSupports;
    /** The name of the interpreter. */
    const char *pszName;

    /**
     * Try open the image.
     *
     * This combines probing and opening.
     *
     * @returns VBox status code. No informational returns defined.
     *
     * @param   pMod        Pointer to the module that is being opened.
     *
     *                      The RTDBGMOD::pszDbgFile member will point to
     *                      the filename of any debug info we're aware of
     *                      on input. Also, or alternatively, it is expected
     *                      that the interpreter will look for debug info in
     *                      the executable image file when present and that it
     *                      may ask the image interpreter for this when it's
     *                      around.
     *
     *                      Upon successful return the method is expected to
     *                      initialize pDbgOps and pvDbgPriv.
     */
    DECLCALLBACKMEMBER(int, pfnTryOpen)(PRTDBGMODINT pMod);

    /**
     * Close the interpreter, freeing all associated resources.
     *
     * The caller sets the pDbgOps and pvDbgPriv RTDBGMOD members
     * to NULL upon return.
     *
     * @param   pMod        Pointer to the module structure.
     */
    DECLCALLBACKMEMBER(int, pfnClose)(PRTDBGMODINT pMod);

    /**
     * Converts an image relative virtual address address to a segmented address.
     *
     * @returns Segment index on success, NIL_RTDBGSEGIDX on failure.
     * @param   pMod        Pointer to the module structure.
     * @param   uRva        The image relative address to convert.
     * @param   poffSeg     Where to return the segment offset. Optional.
     */
    DECLCALLBACKMEMBER(RTDBGSEGIDX, pfnRvaToSegOff)(PRTDBGMODINT pMod, RTUINTPTR uRva, PRTUINTPTR poffSeg);

    /**
     * Adds a symbol to the module (optional).
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_SUPPORTED if the interpreter doesn't support this feature.
     *
     * @param   pMod        Pointer to the module structure.
     * @param   pszSymbol   The symbol name.
     * @param   cchSymbol   The length for the symbol name.
     * @param   iSeg        The segment number (0-based). RTDBGMOD_SEG_RVA can be used.
     * @param   off         The offset into the segment.
     * @param   cb          The area covered by the symbol. 0 is fine.
     */
    DECLCALLBACKMEMBER(int, pfnSymbolAdd)(PRTDBGMODINT pMod, const char *pszSymbol, size_t cchSymbol,
                                          uint32_t iSeg, RTUINTPTR off, RTUINTPTR cb, uint32_t fFlags);

    /**
     * Queries symbol information by symbol name.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS on success, no informational status code.
     * @retval  VERR_RTDBGMOD_NO_SYMBOLS if there aren't any symbols.
     * @retval  VERR_SYMBOL_NOT_FOUND if no suitable symbol was found.
     *
     * @param   pMod        Pointer to the module structure.
     * @param   pszSymbol   The symbol name.
     * @para    pSymbol     Where to store the symbol information.
     */
    DECLCALLBACKMEMBER(int, pfnSymbolByName)(PRTDBGMODINT pMod, const char *pszSymbol, PRTDBGSYMBOL pSymbol);

    /**
     * Queries symbol information by address.
     *
     * The returned symbol is what the debug info interpreter consideres the symbol
     * most applicable to the specified address. This usually means a symbol with an
     * address equal or lower than the requested.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS on success, no informational status code.
     * @retval  VERR_RTDBGMOD_NO_SYMBOLS if there aren't any symbols.
     * @retval  VERR_SYMBOL_NOT_FOUND if no suitable symbol was found.
     *
     * @param   pMod        Pointer to the module structure.
     * @param   iSeg        The segment number (0-based) or RTDBGSEGIDX_ABS.
     * @param   off         The offset into the segment.
     * @param   poffDisp    Where to store the distance between the specified address
     *                      and the returned symbol. Optional.
     * @param   pSymbol     Where to store the symbol information.
     */
    DECLCALLBACKMEMBER(int, pfnSymbolByAddr)(PRTDBGMODINT pMod, uint32_t iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGSYMBOL pSymbol);

    /**
     * Adds a line number to the module (optional).
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_SUPPORTED if the interpreter doesn't support this feature.
     *
     * @param   pMod        Pointer to the module structure.
     * @param   pszFile     The filename.
     * @param   cchFile     The length of the filename.
     * @param   iSeg        The segment number (0-based).
     * @param   off         The offset into the segment.
     */
    DECLCALLBACKMEMBER(int, pfnLineAdd)(PRTDBGMODINT pMod, const char *pszFile, size_t cchFile, uint32_t uLineNo, uint32_t iSeg, RTUINTPTR off);

    /**
     * Queries line number information by address.
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS on success, no informational status code.
     * @retval  VERR_RTDBGMOD_NO_LINE_NUMBERS if there aren't any line numbers.
     * @retval  VERR_RTDBGMOD_LINE_NOT_FOUND if no suitable line number was found.
     *
     * @param   pMod        Pointer to the module structure.
     * @param   iSeg        The segment number (0-based) or RTDBGSEGIDX_ABS.
     * @param   off         The offset into the segment.
     * @param   poffDisp    Where to store the distance between the specified address
     *                      and the returned line number. Optional.
     * @param   pLine       Where to store the information about the closest line number.
     */
    DECLCALLBACKMEMBER(int, pfnLineByAddr)(PRTDBGMODINT pMod, uint32_t iSeg, RTUINTPTR off, PRTINTPTR poffDisp, PRTDBGLINE pLine);

    /** For catching initialization errors (RTDBGMODVTDBG_MAGIC). */
    uint32_t    u32EndMagic;
} RTDBGMODVTDBG;
/** Pointer to a const RTDBGMODVTDBG. */
typedef RTDBGMODVTDBG const *PCRTDBGMODVTDBG;


/**
 * Debug module structure.
 */
typedef struct RTDBGMODINT
{
    /** Magic value (RTDBGMOD_MAGIC). */
    uint32_t            u32Magic;
    /** The number of reference there are to this module.
     * This is used to perform automatic cleanup and sharing. */
    uint32_t volatile   cRefs;
    /** The module name (short). */
    char               *pszName;
    /** The module filename. Can be NULL. */
    char               *pszImgFile;
    /** The debug info file (if external). Can be NULL. */
    char               *pszDbgFile;

    /** Critical section serializing access to the module. */
    RTCRITSECT          CritSect;

    /** The method table for the executable image interpreter. */
    PCRTDBGMODVTIMG     pImgVt;
    /** Pointer to the private data of the executable image interpreter. */
    void               *pvImgPriv;

    /** The method table for the debug info interpreter. */
    PCRTDBGMODVTDBG     pDbgVt;
    /** Pointer to the private data of the debug info interpreter. */
    void               *pvDbgPriv;

} RTDBGMODINT;
/** Pointer to an debug module structure.  */
typedef RTDBGMODINT *PRTDBGMODINT;


extern DECLHIDDEN(RTSTRCACHE) g_hDbgModStrCache;


int rtDbgModContainerCreate(PRTDBGMODINT pMod, RTUINTPTR cb);

/** @} */

RT_C_DECLS_END

#endif


