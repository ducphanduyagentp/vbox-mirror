/* $Id$ */
/** @file
 * IPRT - __divmoddi4 implementation
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#include <iprt/stdint.h>
#include <iprt/uint64.h>

int64_t __divmoddi4(int64_t i64A, int64_t i64B, int64_t *pi64R);
uint64_t __udivmoddi4(uint64_t u64A, uint64_t u64B, uint64_t *pu64R);

/**
 * __divmoddi4() implementation to satisfy external references from 32-bit code
 * generated by gcc-7 or later (more likely with gcc-11).
 *
 * @param   i64A        The divident value.
 * @param   i64B        The divisor value.
 * @param   pi64R       A pointer to the reminder. May be NULL.
 * @returns i64A / i64B
 */
int64_t __divmoddi4(int64_t i64A, int64_t i64B, int64_t *pi64R)
{
    int64_t i64Ret;
    if (i64A >= 0)
    {
        /* Dividing two non-negative numbers is the same as unsigned division. */
        if (i64B >= 0)
            i64Ret = (int64_t)__udivmoddi4((uint64_t)i64A, (uint64_t)i64B, (uint64_t *)pi64R);
        /* Dividing a non-negative number by a negative one yields a negative
           result and positive remainder. */
        else
            i64Ret = -(int64_t)__udivmoddi4((uint64_t)i64A, (uint64_t)-i64B, (uint64_t *)pi64R);
    }
    else
    {
        uint64_t u64R;

        /* Dividing a negative number by a non-negative one yields a negative
           result and negative remainder. */
        if (i64B >= 0)
            i64Ret = -(int64_t)__udivmoddi4((uint64_t)-i64A, (uint64_t)i64B, &u64R);
        /* Dividing two negative numbers yields a positive result and a
           negative remainder. */
        else
            i64Ret = (int64_t)__udivmoddi4((uint64_t)-i64A, (uint64_t)-i64B, &u64R);

        if (pi64R)
            *pi64R = -(int64_t)u64R;
    }

    return i64Ret;
}

