/* $Id$ */
/** @file
 * innotek Portable Runtime Testcase - Simple RTTime tests.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/time.h>
#include <iprt/stream.h>
#include <iprt/initterm.h>
#include <iprt/thread.h>
#include <VBox/sup.h>


int main()
{
    unsigned cErrors = 0;
    int i;
    RTR3Init();
    RTPrintf("tstTime: TESTING...\n");

    /*
     * RTNanoTimeTS() shall never return something which
     * is less or equal to the return of the previous call.
     */

    RTTimeSystemNanoTS(); RTTimeNanoTS(); RTThreadYield();
    uint64_t u64RTStartTS = RTTimeNanoTS();
    uint64_t u64OSStartTS = RTTimeSystemNanoTS();

    uint64_t u64Prev = RTTimeNanoTS();
    for (i = 0; i < 100*_1M; i++)
    {
        uint64_t u64 = RTTimeNanoTS();
        if (u64 <= u64Prev)
        {
            /** @todo wrapping detection. */
            RTPrintf("tstTime: error: i=%#010x u64=%#llx u64Prev=%#llx (1)\n", i, u64, u64Prev);
            cErrors++;
            RTThreadYield();
            u64 = RTTimeNanoTS();
        }
        else if (u64 - u64Prev > 1000000000 /* 1sec */)
        {
            RTPrintf("tstTime: error: i=%#010x u64=%#llx u64Prev=%#llx delta=%lld\n", i, u64, u64Prev, u64 - u64Prev);
            cErrors++;
            RTThreadYield();
            u64 = RTTimeNanoTS();
        }
        if (!(i & (_1M*2 - 1)))
        {
            RTPrintf("tstTime: i=%#010x u64=%#llx u64Prev=%#llx delta=%lld\n", i, u64, u64Prev, u64 - u64Prev);
            RTThreadYield();
            u64 = RTTimeNanoTS();
        }
        u64Prev = u64;
    }

    RTTimeSystemNanoTS(); RTTimeNanoTS(); RTThreadYield();
    uint64_t u64RTElapsedTS = RTTimeNanoTS();
    uint64_t u64OSElapsedTS = RTTimeSystemNanoTS();
    u64RTElapsedTS -= u64RTStartTS;
    u64OSElapsedTS -= u64OSStartTS;
    int64_t i64Diff = u64OSElapsedTS >= u64RTElapsedTS ? u64OSElapsedTS - u64RTElapsedTS : u64RTElapsedTS - u64OSElapsedTS;
    if (i64Diff > (int64_t)(u64OSElapsedTS / 1000))
    {
        RTPrintf("tstTime: error: total time differs too much! u64OSElapsedTS=%#llx u64RTElapsedTS=%#llx delta=%lld\n",
                 u64OSElapsedTS, u64RTElapsedTS, u64OSElapsedTS - u64RTElapsedTS);
        cErrors++;
    }
    else
        RTPrintf("tstTime: total time difference: u64OSElapsedTS=%#llx u64RTElapsedTS=%#llx delta=%lld\n",
                 u64OSElapsedTS, u64RTElapsedTS, u64OSElapsedTS - u64RTElapsedTS);

    RTPrintf("RTTimeDbgSteps   -> %u (%d ppt)\n", RTTimeDbgSteps(),   ((uint64_t)RTTimeDbgSteps() * 1000) / i);
    RTPrintf("RTTimeDbgExpired -> %u (%d ppt)\n", RTTimeDbgExpired(), ((uint64_t)RTTimeDbgExpired() * 1000) / i);
    RTPrintf("RTTimeDbgBad     -> %u (%d ppt)\n", RTTimeDbgBad(),     ((uint64_t)RTTimeDbgBad() * 1000) / i);
    RTPrintf("RTTimeDbgRaces   -> %u (%d ppt)\n", RTTimeDbgRaces(),   ((uint64_t)RTTimeDbgRaces() * 1000) / i);
    if (!cErrors)
        RTPrintf("tstTime: SUCCESS\n");
    else
        RTPrintf("tstTime: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
