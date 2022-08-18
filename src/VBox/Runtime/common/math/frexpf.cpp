/* $Id$ */
/** @file
 * IPRT - No-CRT - frexpf().
 */

/*
 * Copyright (C) 2022 Oracle Corporation
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
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define IPRT_NO_CRT_FOR_3RD_PARTY
#include "internal/nocrt.h"
#include <iprt/nocrt/math.h>
#include <iprt/assertcompile.h>
#include <iprt/nocrt/limits.h>


/* Similar to the fxtract instruction. */
#undef frexpf
float RT_NOCRT(frexpf)(float rfValue, int *piExp)
{
    RTFLOAT32U Value;
    AssertCompile(sizeof(Value) == sizeof(rfValue));
    Value.r = rfValue;

    if (RTFLOAT32U_IS_NORMAL(&Value))
    {
        *piExp = (int)Value.s.uExponent - RTFLOAT32U_EXP_BIAS + 1;
        Value.s.uExponent = RTFLOAT32U_EXP_BIAS - 1;
    }
    else if (RTFLOAT32U_IS_ZERO(&Value))
    {
        *piExp = 0;
        return rfValue;
    }
    else if (RTFLOAT32U_IS_SUBNORMAL(&Value))
    {
        int      iExp      = -RTFLOAT32U_EXP_BIAS + 1;
        uint32_t uFraction = Value.s.uFraction;
        while (!(uFraction & RT_BIT_32(RTFLOAT32U_FRACTION_BITS)))
        {
            iExp--;
            uFraction <<= 1;
        }
        Value.s.uFraction = uFraction;
        Value.s.uExponent = RTFLOAT32U_EXP_BIAS - 1;
        *piExp = iExp + 1;
    }
    else  /* NaN, Inf */
    {
        *piExp = Value.s.fSign ? INT_MIN : INT_MAX;
        return rfValue;
    }
    return Value.r;
}
RT_ALIAS_AND_EXPORT_NOCRT_SYMBOL(frexpf);

