/* $Id: tstMicro.cpp 37408 2008-10-03 22:22:37Z bird $ */
/** @file
 * Micro Testcase, checking emulation of certain instructions
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <VBox/vm.h>
#include <VBox/err.h>
#include <VBox/em.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/runtime.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>


int main(int argc, char **argv)
{
    int     rcRet = 0;                  /* error count. */

    uint32_t eax, edx, ebx, ecx, eflags;
    uint64_t val;

    val = UINT64_C(0xffffffffffff);
    eax = 0xffffffff;
    edx = 0xffff;
    ebx = 0x1;
    ecx = 0x2;
    eflags = EMEmulateLockCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    !(eflags & X86_EFL_ZF)
        ||  val != UINT64_C(0x200000001))
    {
        printf("Lock cmpxchg8b failed the equal case! (val=%x%x)\n", val);
        return -1;
    }
    eflags = EMEmulateLockCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (eflags & X86_EFL_ZF)
    {
        printf("Lock cmpxchg8b failed the non-equal case! (val=%x%x)\n", val);
        return -1;
    }
    printf("Testing lock cmpxchg instruction emulation - SUCCESS\n");

    val = UINT64_C(0xffffffffffff);
    eax = 0xffffffff;
    edx = 0xffff;
    ebx = 0x1;
    ecx = 0x2;
    eflags = EMEmulateCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (    !(eflags & X86_EFL_ZF)
        ||  val != UINT64_C(0x200000001))
    {
        printf("Cmpxchg8b failed the equal case! (val=%x%x)\n", val);
        return -1;
    }
    eflags = EMEmulateCmpXchg8b(&val, &eax, &edx, ebx, ecx);
    if (eflags & X86_EFL_ZF)
    {
        printf("Cmpxchg8b failed the non-equal case! (val=%x%x)\n", val);
        return -1;
    }
    printf("Testing cmpxchg instruction emulation - SUCCESS\n");

    return rcRet;
}
