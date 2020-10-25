/* $Id$ */
/** @file
 * DBGF - Debugger Facility, R0 breakpoint management part.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGF
#include "DBGFInternal.h"
#include <VBox/vmm/gvm.h>
#include <VBox/vmm/gvmm.h>
#include <VBox/vmm/vmm.h>

#include <VBox/log.h>
#include <VBox/sup.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/ctype.h>
#include <iprt/mem.h>
#include <iprt/memobj.h>
#include <iprt/process.h>
#include <iprt/string.h>

#include "dtrace/VBoxVMM.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/**
 * Used by DBGFR0InitPerVM() to initialize the breakpoint manager.
 *
 * @returns nothing.
 * @param   pGVM        The global (ring-0) VM structure.
 */
DECLHIDDEN(void) dbgfR0BpInit(PGVM pGVM)
{
    for (uint32_t i = 0; i < RT_ELEMENTS(pGVM->dbgfr0.s.aBpChunks); i++)
    {
        PDBGFBPCHUNKR0 pBpChunk = &pGVM->dbgfr0.s.aBpChunks[i];

        pBpChunk->hMemObj          = NIL_RTR0MEMOBJ;
        pBpChunk->hMapObj          = NIL_RTR0MEMOBJ;
        //pBpChunk->paBpBaseSharedR0 = NULL;
        //pBpChunk->paBpBaseR0Only   = NULL;
    }

    pGVM->dbgfr0.s.hMemObjBpLocL1 = NIL_RTR0MEMOBJ;
    //pGVM->dbgfr0.s.paBpLocL1R0    = NULL;
    //pGVM->dbgfr0.s.fInit          = false;
}


/**
 * Used by DBGFR0CleanupVM to destroy the breakpoint manager.
 *
 * This is done during VM cleanup so that we're sure there are no active threads
 * using the breakpoint code.
 *
 * @param   pGVM        The global (ring-0) VM structure.
 */
DECLHIDDEN(void) dbgfR0BpDestroy(PGVM pGVM)
{
    if (pGVM->dbgfr0.s.fInit)
    {
        Assert(pGVM->dbgfr0.s.hMemObjBpLocL1 != NIL_RTR0MEMOBJ);
        AssertPtr(pGVM->dbgfr0.s.paBpLocL1R0);

        /*
         * Free all allocated memory and ring-3 mapping objects.
         */
        RTR0MEMOBJ hMemObj = pGVM->dbgfr0.s.hMemObjBpLocL1;
        pGVM->dbgfr0.s.hMemObjBpLocL1 = NIL_RTR0MEMOBJ;
        pGVM->dbgfr0.s.paBpLocL1R0    = NULL;
        RTR0MemObjFree(hMemObj, true);

        for (uint32_t i = 0; i < RT_ELEMENTS(pGVM->dbgfr0.s.aBpChunks); i++)
        {
            PDBGFBPCHUNKR0 pBpChunk = &pGVM->dbgfr0.s.aBpChunks[i];

            if (pBpChunk->hMemObj != NIL_RTR0MEMOBJ)
            {
                Assert(pBpChunk->hMapObj != NIL_RTR0MEMOBJ);

                pBpChunk->paBpBaseSharedR0 = NULL;
                pBpChunk->paBpBaseR0Only   = NULL;

                hMemObj = pBpChunk->hMapObj;
                pBpChunk->hMapObj = NIL_RTR0MEMOBJ;
                RTR0MemObjFree(hMemObj, true);

                hMemObj = pBpChunk->hMemObj;
                pBpChunk->hMemObj = NIL_RTR0MEMOBJ;
                RTR0MemObjFree(hMemObj, true);
            }
        }

        pGVM->dbgfr0.s.fInit = false;
    }
#ifdef RT_STRICT
    else
    {
        Assert(pGVM->dbgfr0.s.hMemObjBpLocL1 == NIL_RTR0MEMOBJ);
        Assert(!pGVM->dbgfr0.s.paBpLocL1R0);

        for (uint32_t i = 0; i < RT_ELEMENTS(pGVM->dbgfr0.s.aBpChunks); i++)
        {
            PDBGFBPCHUNKR0 pBpChunk = &pGVM->dbgfr0.s.aBpChunks[i];

            Assert(pBpChunk->hMemObj == NIL_RTR0MEMOBJ);
            Assert(pBpChunk->hMapObj == NIL_RTR0MEMOBJ);
            Assert(!pBpChunk->paBpBaseSharedR0);
            Assert(!pBpChunk->paBpBaseR0Only);
        }
    }
#endif
}


/**
 * Worker for DBGFR0BpInitReqHandler() that does the actual initialization.
 *
 * @returns VBox status code.
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   ppBpLocL1R3     Where to return the ring-3 L1 lookup table address on success.
 * @thread  EMT(0)
 */
static int dbgfR0BpInitWorker(PGVM pGVM, R3PTRTYPE(volatile uint32_t *) *ppaBpLocL1R3)
{
    /*
     * Figure out how much memory we need for the L1 lookup table and allocate it.
     */
    uint32_t const cbL1Loc = RT_ALIGN_32(UINT16_MAX * sizeof(uint32_t), PAGE_SIZE);

    RTR0MEMOBJ hMemObj;
    int rc = RTR0MemObjAllocPage(&hMemObj, cbL1Loc, false /*fExecutable*/);
    if (RT_FAILURE(rc))
        return rc;
    RT_BZERO(RTR0MemObjAddress(hMemObj), cbL1Loc);

    /* Map it. */
    RTR0MEMOBJ hMapObj;
    rc = RTR0MemObjMapUserEx(&hMapObj, hMemObj, (RTR3PTR)-1, 0, RTMEM_PROT_READ | RTMEM_PROT_WRITE, RTR0ProcHandleSelf(),
                             0 /*offSub*/, cbL1Loc);
    if (RT_SUCCESS(rc))
    {
        pGVM->dbgfr0.s.hMemObjBpLocL1 = hMemObj;
        pGVM->dbgfr0.s.hMapObjBpLocL1 = hMapObj;
        pGVM->dbgfr0.s.paBpLocL1R0    = (volatile uint32_t *)RTR0MemObjAddress(hMemObj);

        /*
         * We're done.
         */
        *ppaBpLocL1R3 = RTR0MemObjAddressR3(hMapObj);
        pGVM->dbgfr0.s.fInit = true;
        return rc;
    }

    RTR0MemObjFree(hMemObj, true);
    return rc;
}


/**
 * Worker for DBGFR0BpChunkAllocReqHandler() that does the actual chunk allocation.
 *
 * Allocates a memory object and divides it up as follows:
 * @verbatim
   --------------------------------------
   ring-0 chunk data
   --------------------------------------
   page alignment padding
   --------------------------------------
   shared chunk data
   --------------------------------------
   @endverbatim
 *
 * @returns VBox status code.
 * @param   pGVM            The global (ring-0) VM structure.
 * @param   idChunk         The chunk ID to allocate.
 * @param   ppBpChunkBaseR3 Where to return the ring-3 chunk base address on success.
 * @thread  EMT(0)
 */
static int dbgfR0BpChunkAllocWorker(PGVM pGVM, uint32_t idChunk, R3PTRTYPE(void *) *ppBpChunkBaseR3)
{
    /*
     * Figure out how much memory we need for the chunk and allocate it.
     */
    uint32_t const cbRing0  = RT_ALIGN_32(DBGF_BP_COUNT_PER_CHUNK * sizeof(DBGFBPINTR0), PAGE_SIZE);
    uint32_t const cbShared = RT_ALIGN_32(DBGF_BP_COUNT_PER_CHUNK * sizeof(DBGFBPINT), PAGE_SIZE);
    uint32_t const cbTotal  = cbRing0 + cbShared;

    RTR0MEMOBJ hMemObj;
    int rc = RTR0MemObjAllocPage(&hMemObj, cbTotal, false /*fExecutable*/);
    if (RT_FAILURE(rc))
        return rc;
    RT_BZERO(RTR0MemObjAddress(hMemObj), cbTotal);

    /* Map it. */
    RTR0MEMOBJ hMapObj;
    rc = RTR0MemObjMapUserEx(&hMapObj, hMemObj, (RTR3PTR)-1, 0, RTMEM_PROT_READ | RTMEM_PROT_WRITE, RTR0ProcHandleSelf(),
                             cbRing0 /*offSub*/, cbTotal - cbRing0);
    if (RT_SUCCESS(rc))
    {
        PDBGFBPCHUNKR0 pBpChunkR0 = &pGVM->dbgfr0.s.aBpChunks[idChunk];

        pBpChunkR0->hMemObj          = hMemObj;
        pBpChunkR0->hMapObj          = hMapObj;
        pBpChunkR0->paBpBaseR0Only   = (PDBGFBPINTR0)RTR0MemObjAddress(hMemObj);
        pBpChunkR0->paBpBaseSharedR0 = (PDBGFBPINT)&pBpChunkR0->paBpBaseR0Only[DBGF_BP_COUNT_PER_CHUNK];

        /*
         * We're done.
         */
        *ppBpChunkBaseR3 = RTR0MemObjAddressR3(hMapObj);
        return rc;
    }

    RTR0MemObjFree(hMemObj, true);
    return rc;
}


/**
 * Used by ring-3 DBGF to fully initialize the breakpoint manager for operation.
 *
 * @returns VBox status code.
 * @param   pGVM    The global (ring-0) VM structure.
 * @param   pReq    Pointer to the request buffer.
 * @thread  EMT(0)
 */
VMMR0_INT_DECL(int) DBGFR0BpInitReqHandler(PGVM pGVM, PDBGFBPINITREQ pReq)
{
    LogFlow(("DBGFR0BpInitReqHandler:\n"));

    /*
     * Validate the request.
     */
    AssertReturn(pReq->Hdr.cbReq == sizeof(*pReq), VERR_INVALID_PARAMETER);

    int rc = GVMMR0ValidateGVMandEMT(pGVM, 0);
    AssertRCReturn(rc, rc);

    AssertReturn(!pGVM->dbgfr0.s.fInit, VERR_WRONG_ORDER);

    return dbgfR0BpInitWorker(pGVM, &pReq->paBpLocL1R3);
}


/**
 * Used by ring-3 DBGF to allocate a given chunk in the global breakpoint table.
 *
 * @returns VBox status code.
 * @param   pGVM    The global (ring-0) VM structure.
 * @param   pReq    Pointer to the request buffer.
 * @thread  EMT(0)
 */
VMMR0_INT_DECL(int) DBGFR0BpChunkAllocReqHandler(PGVM pGVM, PDBGFBPCHUNKALLOCREQ pReq)
{
    LogFlow(("DBGFR0BpChunkAllocReqHandler:\n"));

    /*
     * Validate the request.
     */
    AssertReturn(pReq->Hdr.cbReq == sizeof(*pReq), VERR_INVALID_PARAMETER);

    uint32_t const idChunk = pReq->idChunk;
    AssertReturn(idChunk < DBGF_BP_CHUNK_COUNT, VERR_INVALID_PARAMETER);

    int rc = GVMMR0ValidateGVMandEMT(pGVM, 0);
    AssertRCReturn(rc, rc);

    AssertReturn(pGVM->dbgfr0.s.fInit, VERR_WRONG_ORDER);
    AssertReturn(pGVM->dbgfr0.s.aBpChunks[idChunk].hMemObj == NIL_RTR0MEMOBJ, VERR_INVALID_PARAMETER);

    return dbgfR0BpChunkAllocWorker(pGVM, idChunk, &pReq->pChunkBaseR3);
}

