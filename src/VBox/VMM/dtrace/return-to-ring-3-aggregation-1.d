/* $Id$ */
/** @file
 * DTracing VBox - return to ring-3 aggregation test \#1.
 */

/*
 * Copyright (C) 2012-2022 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma D option quiet


vboxvmm:::r0-vmm-return-to-ring3-hm
{
    @g_aHmRcs[args[2]] = count();
}

vboxvmm:::r0-vmm-return-to-ring3-rc
{
    @g_aRawRcs[args[2]] = count();
}

END
{
    printa(" rcHm=%04d   %@8u times\n", @g_aHmRcs);
    printa(" rcRaw=%04d   %@8u times\n", @g_aRawRcs);
}

