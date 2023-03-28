/** @file
 * IPRT - ARMv8 (AArch64 and AArch32) Structures and Definitions.
 */

/*
 * Copyright (C) 2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_armv8_h
#define IPRT_INCLUDED_armv8_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef VBOX_FOR_DTRACE_LIB
# include <iprt/types.h>
# include <iprt/assert.h>
#else
# pragma D depends_on library vbox-types.d
#endif

/** @defgroup grp_rt_armv8   ARMv8 Types and Definitions
 * @ingroup grp_rt
 * @{
 */

/** @name The AArch64 register encoding.
 * @{ */
#define ARMV8_AARCH64_REG_X0                        0
#define ARMV8_AARCH64_REG_W0                        ARMV8_AARCH64_REG_X0
#define ARMV8_AARCH64_REG_X1                        1
#define ARMV8_AARCH64_REG_W1                        ARMV8_AARCH64_REG_X1
#define ARMV8_AARCH64_REG_X2                        2
#define ARMV8_AARCH64_REG_W2                        ARMV8_AARCH64_REG_X2
#define ARMV8_AARCH64_REG_X3                        3
#define ARMV8_AARCH64_REG_W3                        ARMV8_AARCH64_REG_X3
#define ARMV8_AARCH64_REG_X4                        4
#define ARMV8_AARCH64_REG_W4                        ARMV8_AARCH64_REG_X4
#define ARMV8_AARCH64_REG_X5                        5
#define ARMV8_AARCH64_REG_W5                        ARMV8_AARCH64_REG_X5
#define ARMV8_AARCH64_REG_X6                        6
#define ARMV8_AARCH64_REG_W6                        ARMV8_AARCH64_REG_X6
#define ARMV8_AARCH64_REG_X7                        7
#define ARMV8_AARCH64_REG_W7                        ARMV8_AARCH64_REG_X7
#define ARMV8_AARCH64_REG_X8                        8
#define ARMV8_AARCH64_REG_W8                        ARMV8_AARCH64_REG_X8
#define ARMV8_AARCH64_REG_X9                        9
#define ARMV8_AARCH64_REG_W9                        ARMV8_AARCH64_REG_X9
#define ARMV8_AARCH64_REG_X10                       10
#define ARMV8_AARCH64_REG_W10                       ARMV8_AARCH64_REG_X10
#define ARMV8_AARCH64_REG_X11                       11
#define ARMV8_AARCH64_REG_W11                       ARMV8_AARCH64_REG_X11
#define ARMV8_AARCH64_REG_X12                       12
#define ARMV8_AARCH64_REG_W12                       ARMV8_AARCH64_REG_X12
#define ARMV8_AARCH64_REG_X13                       13
#define ARMV8_AARCH64_REG_W13                       ARMV8_AARCH64_REG_X13
#define ARMV8_AARCH64_REG_X14                       14
#define ARMV8_AARCH64_REG_W14                       ARMV8_AARCH64_REG_X14
#define ARMV8_AARCH64_REG_X15                       15
#define ARMV8_AARCH64_REG_W15                       ARMV8_AARCH64_REG_X15
#define ARMV8_AARCH64_REG_X16                       16
#define ARMV8_AARCH64_REG_W16                       ARMV8_AARCH64_REG_X16
#define ARMV8_AARCH64_REG_X17                       17
#define ARMV8_AARCH64_REG_W17                       ARMV8_AARCH64_REG_X17
#define ARMV8_AARCH64_REG_X18                       18
#define ARMV8_AARCH64_REG_W18                       ARMV8_AARCH64_REG_X18
#define ARMV8_AARCH64_REG_X19                       19
#define ARMV8_AARCH64_REG_W19                       ARMV8_AARCH64_REG_X19
#define ARMV8_AARCH64_REG_X20                       20
#define ARMV8_AARCH64_REG_W20                       ARMV8_AARCH64_REG_X20
#define ARMV8_AARCH64_REG_X21                       21
#define ARMV8_AARCH64_REG_W21                       ARMV8_AARCH64_REG_X21
#define ARMV8_AARCH64_REG_X22                       22
#define ARMV8_AARCH64_REG_W22                       ARMV8_AARCH64_REG_X22
#define ARMV8_AARCH64_REG_X23                       23
#define ARMV8_AARCH64_REG_W23                       ARMV8_AARCH64_REG_X23
#define ARMV8_AARCH64_REG_X24                       24
#define ARMV8_AARCH64_REG_W24                       ARMV8_AARCH64_REG_X24
#define ARMV8_AARCH64_REG_X25                       25
#define ARMV8_AARCH64_REG_W25                       ARMV8_AARCH64_REG_X25
#define ARMV8_AARCH64_REG_X26                       26
#define ARMV8_AARCH64_REG_W26                       ARMV8_AARCH64_REG_X26
#define ARMV8_AARCH64_REG_X27                       27
#define ARMV8_AARCH64_REG_W27                       ARMV8_AARCH64_REG_X27
#define ARMV8_AARCH64_REG_X28                       28
#define ARMV8_AARCH64_REG_W28                       ARMV8_AARCH64_REG_X28
#define ARMV8_AARCH64_REG_X29                       29
#define ARMV8_AARCH64_REG_W29                       ARMV8_AARCH64_REG_X29
#define ARMV8_AARCH64_REG_X30                       30
#define ARMV8_AARCH64_REG_W30                       ARMV8_AARCH64_REG_X30
/** The zero register. */
#define ARMV8_AARCH64_REG_ZR                        31
/** @} */


/** @name System register encoding.
 * @{
 */
/** Mask for the op0 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP0_MASK               (RT_BIT_32(19) | RT_BIT_32(20))
/** Shift for the op0 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP0_SHIFT              19
/** Returns the op0 part of the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_OP0_GET(a_MsrMrsInsn)  (((a_MsrMrsInsn) & ARMV8_AARCH64_SYSREG_OP0_MASK) >> ARMV8_AARCH64_SYSREG_OP0_SHIFT)
/** Mask for the op1 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP1_MASK               (RT_BIT_32(16) | RT_BIT_32(17) | RT_BIT_32(18))
/** Shift for the op1 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP1_SHIFT              16
/** Returns the op1 part of the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_OP1_GET(a_MsrMrsInsn)  (((a_MsrMrsInsn) & ARMV8_AARCH64_SYSREG_OP1_MASK) >> ARMV8_AARCH64_SYSREG_OP1_SHIFT)
/** Mask for the CRn part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_CRN_MASK               (  RT_BIT_32(12) | RT_BIT_32(13) | RT_BIT_32(14) \
                                                     | RT_BIT_32(15) )
/** Shift for the CRn part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_CRN_SHIFT              12
/** Returns the CRn part of the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_CRN_GET(a_MsrMrsInsn)  (((a_MsrMrsInsn) & ARMV8_AARCH64_SYSREG_CRN_MASK) >> ARMV8_AARCH64_SYSREG_CRN_SHIFT)
/** Mask for the CRm part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_CRM_MASK               (  RT_BIT_32(8) | RT_BIT_32(9) | RT_BIT_32(10) \
                                                     | RT_BIT_32(11) )
/** Shift for the CRm part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_CRM_SHIFT              8
/** Returns the CRn part of the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_CRM_GET(a_MsrMrsInsn)  (((a_MsrMrsInsn) & ARMV8_AARCH64_SYSREG_CRM_MASK) >> ARMV8_AARCH64_SYSREG_CRM_SHIFT)
/** Mask for the op2 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP2_MASK               (RT_BIT_32(5) | RT_BIT_32(6) | RT_BIT_32(7))
/** Shift for the op2 part of an MSR/MRS instruction */
#define ARMV8_AARCH64_SYSREG_OP2_SHIFT              5
/** Returns the op2 part of the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_OP2_GET(a_MsrMrsInsn)  (((a_MsrMrsInsn) & ARMV8_AARCH64_SYSREG_OP2_MASK) >> ARMV8_AARCH64_SYSREG_OP2_SHIFT)
/** Mask for all system register encoding relevant fields in an MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_MASK                   (  ARMV8_AARCH64_SYSREG_OP0_MASK | ARMV8_AARCH64_SYSREG_OP1_MASK \
                                                     | ARMV8_AARCH64_SYSREG_CRN_MASK | ARMV8_AARCH64_SYSREG_CRN_MASK \
                                                     | ARMV8_AARCH64_SYSREG_OP2_MASK)

/** @name Mapping of op0:op1:CRn:CRm:op2 to a system register ID. This is
 * IPRT specific and not part of the ARMv8 specification. */
#define ARMV8_AARCH64_SYSREG_ID_CREATE(a_Op0, a_Op1, a_CRn, a_CRm, a_Op2) \
    UINT16_C(  (((a_Op1) & 0x3) << 15) \
             | (((a_Op1) & 0x7) << 12) \
             | (((a_CRn) & 0xf) <<  7) \
             | (((a_CRm) & 0xf) <<  3) \
             |  ((a_Op2) & 0x7))
/** Returns the internal system register ID from the given MRS/MSR instruction. */
#define ARMV8_AARCH64_SYSREG_ID_FROM_MRS_MSR(a_MsrMrsInsn) \
    ARMV8_AARCH64_SYSREG_ID_CREATE(ARMV8_AARCH64_SYSREG_OP0_GET(a_MsrMrsInsn), \
                                   ARMV8_AARCH64_SYSREG_OP1_GET(a_MsrMrsInsn), \
                                   ARMV8_AARCH64_SYSREG_CRN_GET(a_MsrMrsInsn), \
                                   ARMV8_AARCH64_SYSREG_CRM_GET(a_MsrMrsInsn), \
                                   ARMV8_AARCH64_SYSREG_OP2_GET(a_MsrMrsInsn))
/** Encodes the given system register ID in the given MSR/MRS instruction. */
#define ARMV8_AARCH64_SYSREG_ID_ENCODE_IN_MRS_MSR(a_MsrMrsInsn, a_SysregId) \
    ((a_MsrMrsInsn) = ((a_MsrMrsInsn) & ~ARMV8_AARCH64_SYSREG_MASK) | (a_SysregId << ARMV8_AARCH64_SYSREG_OP2_SHIFT))
/** @} */


/** @name System register IDs.
 * @{ */
/** MIDR_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_MIDR_EL1               ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 0, 0)
/** MIPDR_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_MPIDR_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 0, 5)
/** REVIDR_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_REVIDR_EL1             ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 0, 6)
/** ID_PFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_PFR0_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 0)
/** ID_PFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_PFR1_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 1)
/** ID_DFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_DFR0_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 2)
/** ID_AFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AFR0_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 3)
/** ID_MMFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR0_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 4)
/** ID_MMFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR1_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 5)
/** ID_MMFR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR2_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 6)
/** ID_MMFR3_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR3_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 1, 7)

/** ID_ISAR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR0_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 0)
/** ID_ISAR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR1_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 1)
/** ID_ISAR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR2_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 2)
/** ID_ISAR3_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR3_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 3)
/** ID_ISAR4_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR4_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 4)
/** ID_ISAR5_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR5_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 5)
/** ID_MMFR4_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR4_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 6)
/** ID_ISAR6_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_ISAR6_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 2, 7)

/** MVFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_MVFR0_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 0)
/** MVFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_MVFR1_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 1)
/** MVFR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_MVFR2_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 2)
/** ID_PFR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_PFR2_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 4)
/** ID_DFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_DFR1_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 5)
/** ID_MMFR5_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_MMFR5_EL1           ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 3, 6)

/** ID_AA64PFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64PFR0_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 4, 0)
/** ID_AA64PFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64PFR1_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 4, 1)
/** ID_AA64ZFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64ZFR0_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 4, 4)
/** ID_AA64SMFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64SMFR0_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 4, 5)

/** ID_AA64DFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64DFR0_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 5, 0)
/** ID_AA64DFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64DFR1_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 5, 1)
/** ID_AA64AFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64AFR0_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 5, 4)
/** ID_AA64AFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64AFR1_EL1        ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 5, 5)

/** ID_AA64ISAR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64ISAR0_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 6, 0)
/** ID_AA64ISAR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64ISAR1_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 6, 1)
/** ID_AA64ISAR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64ISAR2_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 6, 2)

/** ID_AA64MMFR0_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64MMFR0_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 7, 0)
/** ID_AA64MMFR1_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64MMFR1_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 7, 1)
/** ID_AA64MMFR2_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ID_AA64MMFR2_EL1       ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 0, 7, 2)

/** SCTRL_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_SCTRL_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 0, 0)
/** ACTRL_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_ACTRL_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 0, 1)
/** CPACR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_CPACR_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 0, 2)
/** RGSR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_RGSR_EL1               ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 0, 5)
/** GCR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_GCR_EL1                ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 0, 6)

/** ZCR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_ZCR_EL1                ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 2, 0)
/** TRFCR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_TRFCR_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 2, 1)
/** SMPRI_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_SMPRI_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 2, 4)
/** SMCR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_SMCR_EL1               ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 1, 2, 6)

/** TTBR0_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_TTBR0_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 2, 0, 0)
/** TTBR1_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_TTBR1_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 2, 0, 1)
/** TCR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_TCR_EL1                ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 2, 0, 2)

/** @todo APIA,APIB,APDA,APDB,APGA registers. */

/** SPSR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_SPSR_EL1               ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 0, 0)
/** ELR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_ELR_EL1                ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 0, 1)

/** SP_EL0 register - RW. */
#define ARMV8_AARCH64_SYSREG_SP_EL0                 ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 1, 0)

/** PSTATE.SPSel value. */
#define ARMV8_AARCH64_SYSREG_SPSEL                  ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 2, 0)
/** PSTATE.CurrentEL value. */
#define ARMV8_AARCH64_SYSREG_CURRENTEL              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 2, 2)
/** PSTATE.PAN value. */
#define ARMV8_AARCH64_SYSREG_PAN                    ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 2, 3)
/** PSTATE.UAO value. */
#define ARMV8_AARCH64_SYSREG_UAO                    ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 2, 4)

/** PSTATE.ALLINT value. */
#define ARMV8_AARCH64_SYSREG_ALLINT                 ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 4, 3, 0)


/** AFSR0_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_AFSR0_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 5, 1, 0)
/** AFSR1_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_AFSR1_EL1              ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 5, 1, 1)

/** ESR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_ESR_EL1                ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 5, 2, 0)

/** ERRIDR_EL1 register - RO. */
#define ARMV8_AARCH64_SYSREG_ERRIDR_EL1             ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 5, 3, 0)
/** ERRSELR_EL1 register - RW. */
#define ARMV8_AARCH64_SYSREG_ERRSELR_EL1            ARMV8_AARCH64_SYSREG_ID_CREATE(3, 0, 5, 3, 1)

/** @} */

/** @} */


/**
 * SPSR_EL2 (according to chapter C5.2.19)
 */
typedef union ARMV8SPSREL2
{
    /** The plain unsigned view. */
    uint64_t        u;
    /** The 8-bit view. */
    uint8_t         au8[8];
    /** The 16-bit view. */
    uint16_t        au16[4];
    /** The 32-bit view. */
    uint32_t        au32[2];
    /** The 64-bit view. */
    uint64_t        u64;
} ARMV8SPSREL2;
/** Pointer to SPSR_EL2. */
typedef ARMV8SPSREL2 *PARMV8SPSREL2;
/** Pointer to const SPSR_EL2. */
typedef const ARMV8SPSREL2 *PCXARMV8SPSREL2;


/** @name SPSR_EL2 (When exception is taken from AArch64 state)
 * @{
 */
/** Bit 0 - 3 - M - AArch64 Exception level and selected stack pointer. */
#define ARMV8_SPSR_EL2_AARCH64_M                    (RT_BIT_64(0) | RT_BIT_64(1) | RT_BIT_64(2) | RT_BIT_64(3))
#define ARMV8_SPSR_EL2_AARCH64_GET_M(a_Spsr)        ((a_Spsr) & ARMV8_SPSR_EL2_AARCH64_M)
/** Bit 0 - SP - Selected stack pointer. */
#define ARMV8_SPSR_EL2_AARCH64_SP                   RT_BIT_64(0)
#define ARMV8_SPSR_EL2_AARCH64_SP_BIT               0
/** Bit 1 - Reserved (read as zero). */
#define ARMV8_SPSR_EL2_AARCH64_RSVD_1               RT_BIT_64(1)
/** Bit 2 - 3 - EL - Exception level. */
#define ARMV8_SPSR_EL2_AARCH64_EL                   (RT_BIT_64(2) | RT_BIT_64(3))
#define ARMV8_SPSR_EL2_AARCH64_EL_SHIFT             2
#define ARMV8_SPSR_EL2_AARCH64_GET_EL(a_Spsr)       (((a_Spsr) >> ARMV8_SPSR_EL2_AARCH64_EL_SHIFT) & 3)
#define ARMV8_SPSR_EL2_AARCH64_SET_EL(a_El)         ((a_El) << ARMV8_SPSR_EL2_AARCH64_EL_SHIFT)
/** Bit 4 - M[4] - Execution state (0 means AArch64, when 1 this contains a AArch32 state). */
#define ARMV8_SPSR_EL2_AARCH64_M4                   RT_BIT_64(4)
#define ARMV8_SPSR_EL2_AARCH64_M4_BIT               4
/** Bit 5 - Reserved (read as zero). */
#define ARMV8_SPSR_EL2_AARCH64_RSVD_5               RT_BIT_64(5)
/** Bit 6 - I - FIQ interrupt mask. */
#define ARMV8_SPSR_EL2_AARCH64_F                    RT_BIT_64(6)
#define ARMV8_SPSR_EL2_AARCH64_F_BIT                6
/** Bit 7 - I - IRQ interrupt mask. */
#define ARMV8_SPSR_EL2_AARCH64_I                    RT_BIT_64(7)
#define ARMV8_SPSR_EL2_AARCH64_I_BIT                7
/** Bit 8 - A - SError interrupt mask. */
#define ARMV8_SPSR_EL2_AARCH64_A                    RT_BIT_64(8)
#define ARMV8_SPSR_EL2_AARCH64_A_BIT                8
/** Bit 9 - D - Debug Exception mask. */
#define ARMV8_SPSR_EL2_AARCH64_D                    RT_BIT_64(9)
#define ARMV8_SPSR_EL2_AARCH64_D_BIT                9
/** Bit 10 - 11 - BTYPE - Branch Type indicator. */
#define ARMV8_SPSR_EL2_AARCH64_BYTPE                (RT_BIT_64(10) | RT_BIT_64(11))
#define ARMV8_SPSR_EL2_AARCH64_BYTPE_SHIFT          10
#define ARMV8_SPSR_EL2_AARCH64_GET_BYTPE(a_Spsr)    (((a_Spsr) >> ARMV8_SPSR_EL2_AARCH64_BYTPE_SHIFT) & 3)
/** Bit 12 - SSBS - Speculative Store Bypass. */
#define ARMV8_SPSR_EL2_AARCH64_SSBS                 RT_BIT_64(12)
#define ARMV8_SPSR_EL2_AARCH64_SSBS_BIT             12
/** Bit 13 - ALLINT - All IRQ or FIQ interrupts mask. */
#define ARMV8_SPSR_EL2_AARCH64_ALLINT               RT_BIT_64(13)
#define ARMV8_SPSR_EL2_AARCH64_ALLINT_BIT           13
/** Bit 14 - 19 - Reserved (read as zero). */
#define ARMV8_SPSR_EL2_AARCH64_RSVD_14_19           (  RT_BIT_64(14) | RT_BIT_64(15) | RT_BIT_64(16) \
                                                     | RT_BIT_64(17) | RT_BIT_64(18) | RT_BIT_64(19))
/** Bit 20 - IL - Illegal Execution State flag. */
#define ARMV8_SPSR_EL2_AARCH64_IL                   RT_BIT_64(20)
#define ARMV8_SPSR_EL2_AARCH64_IL_BIT               20
/** Bit 21 - SS - Software Step flag. */
#define ARMV8_SPSR_EL2_AARCH64_SS                   RT_BIT_64(21)
#define ARMV8_SPSR_EL2_AARCH64_SS_BIT               21
/** Bit 22 - PAN - Privileged Access Never flag. */
#define ARMV8_SPSR_EL2_AARCH64_PAN                  RT_BIT_64(25)
#define ARMV8_SPSR_EL2_AARCH64_PAN_BIT              22
/** Bit 23 - UAO - User Access Override flag. */
#define ARMV8_SPSR_EL2_AARCH64_UAO                  RT_BIT_64(23)
#define ARMV8_SPSR_EL2_AARCH64_UAO_BIT              23
/** Bit 24 - DIT - Data Independent Timing flag. */
#define ARMV8_SPSR_EL2_AARCH64_DIT                  RT_BIT_64(24)
#define ARMV8_SPSR_EL2_AARCH64_DIT_BIT              24
/** Bit 25 - TCO - Tag Check Override flag. */
#define ARMV8_SPSR_EL2_AARCH64_TCO                  RT_BIT_64(25)
#define ARMV8_SPSR_EL2_AARCH64_TCO_BIT              25
/** Bit 26 - 27 - Reserved (read as zero). */
#define ARMV8_SPSR_EL2_AARCH64_RSVD_26_27           (RT_BIT_64(26) | RT_BIT_64(27))
/** Bit 28 - V - Overflow condition flag. */
#define ARMV8_SPSR_EL2_AARCH64_V                    RT_BIT_64(28)
#define ARMV8_SPSR_EL2_AARCH64_V_BIT                28
/** Bit 29 - C - Carry condition flag. */
#define ARMV8_SPSR_EL2_AARCH64_C                    RT_BIT_64(29)
#define ARMV8_SPSR_EL2_AARCH64_C_BIT                29
/** Bit 30 - Z - Zero condition flag. */
#define ARMV8_SPSR_EL2_AARCH64_Z                    RT_BIT_64(30)
#define ARMV8_SPSR_EL2_AARCH64_Z_BIT                30
/** Bit 31 - N - Negative condition flag. */
#define ARMV8_SPSR_EL2_AARCH64_N                    RT_BIT_64(31)
#define ARMV8_SPSR_EL2_AARCH64_N_BIT                31
/** Bit 32 - 63 - Reserved (read as zero). */
#define ARMV8_SPSR_EL2_AARCH64_RSVD_32_63           (UINT64_C(0xffffffff00000000))
/** Checks whether the given SPSR value contains a AARCH64 execution state. */
#define ARMV8_SPSR_EL2_IS_AARCH64_STATE(a_Spsr)     (!((a_Spsr) & ARMV8_SPSR_EL2_AARCH64_M4))
/** @} */

/** @name Aarch64 Exception levels
 * @{ */
/** Exception Level 0 - User mode. */
#define ARMV8_AARCH64_EL_0                          0
/** Exception Level 1 - Supervisor mode. */
#define ARMV8_AARCH64_EL_1                          1
/** Exception Level 2 - Hypervisor mode. */
#define ARMV8_AARCH64_EL_2                          2
/** @} */


/** @name ESR_EL2 (Exception Syndrome Register, EL2)
 * @{
 */
/** Bit 0 - 24 - ISS - Instruction Specific Syndrome, encoding depends on the exception class. */
#define ARMV8_ESR_EL2_ISS                           UINT64_C(0x1ffffff)
#define ARMV8_ESR_EL2_ISS_GET(a_Esr)                ((a_Esr) & ARMV8_ESR_EL2_ISS)
/** Bit 25 - IL - Instruction length for synchronous exception (0 means 16-bit instruction, 1 32-bit instruction). */
#define ARMV8_ESR_EL2_IL                            RT_BIT_64(25)
#define ARMV8_ESR_EL2_IL_BIT                        25
#define ARMV8_ESR_EL2_IL_IS_32BIT(a_Esr)            RT_BOOL((a_Esr) & ARMV8_ESR_EL2_IL)
#define ARMV8_ESR_EL2_IL_IS_16BIT(a_Esr)            (!((a_Esr) & ARMV8_ESR_EL2_IL))
/** Bit 26 - 31 - EC - Exception class, indicates reason for the exception that this register holds information about. */
#define ARMV8_ESR_EL2_EC                            (  RT_BIT_64(26) | RT_BIT_64(27) | RT_BIT_64(28) \
                                                     | RT_BIT_64(29) | RT_BIT_64(30) | RT_BIT_64(31))
#define ARMV8_ESR_EL2_EC_GET(a_Esr)                 (((a_Esr) & ARMV8_ESR_EL2_EC) >> 26)
/** Bit 32 - 36 - ISS2 - Only valid when FEAT_LS64_V and/or FEAT_LS64_ACCDATA is present. */
#define ARMV8_ESR_EL2_ISS2                          (  RT_BIT_64(32) | RT_BIT_64(33) | RT_BIT_64(34) \
                                                     | RT_BIT_64(35) | RT_BIT_64(36))
#define ARMV8_ESR_EL2_ISS2_GET(a_Esr)               (((a_Esr) & ARMV8_ESR_EL2_ISS2) >> 32)
/*+ @} */


/** @name ESR_EL2 Exception Classes (EC)
 * @{ */
/** Unknown exception reason. */
#define ARMV8_ESR_EL2_EC_UNKNOWN                                UINT32_C(0)
/** Trapped WF* instruction. */
#define ARMV8_ESR_EL2_EC_TRAPPED_WFX                            UINT32_C(1)
/** AArch32 - Trapped MCR or MRC access (coproc == 0b1111) not reported through ARMV8_ESR_EL2_EC_UNKNOWN. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_MCR_MRC_COPROC_15      UINT32_C(3)
/** AArch32 - Trapped MCRR or MRRC access (coproc == 0b1111) not reported through ARMV8_ESR_EL2_EC_UNKNOWN. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_MCRR_MRRC_COPROC15     UINT32_C(4)
/** AArch32 - Trapped MCR or MRC access (coproc == 0b1110). */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_MCR_MRC_COPROC_14      UINT32_C(5)
/** AArch32 - Trapped LDC or STC access. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_LDC_STC                UINT32_C(6)
/** AArch32 - Trapped access to SME, SVE or Advanced SIMD or floating point fnunctionality. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_SME_SVE_NEON           UINT32_C(7)
/** AArch32 - Trapped VMRS access not reported using ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_SME_SVE_NEON. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_VMRS                   UINT32_C(8)
/** AArch32 - Trapped pointer authentication instruction. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_PA_INSN                UINT32_C(9)
/** FEAT_LS64 - Exception from LD64B or ST64B instruction. */
#define ARMV8_ESR_EL2_EC_LS64_EXCEPTION                         UINT32_C(10)
/** AArch32 - Trapped MRRC access (coproc == 0b1110). */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_MRRC_COPROC14          UINT32_C(12)
/** FEAT_BTI - Branch Target Exception. */
#define ARMV8_ESR_EL2_EC_BTI_BRANCH_TARGET_EXCEPTION            UINT32_C(13)
/** Illegal Execution State. */
#define ARMV8_ESR_EL2_ILLEGAL_EXECUTION_STATE                   UINT32_C(14)
/** AArch32 - SVC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH32_SVC_INSN                       UINT32_C(17)
/** AArch32 - HVC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH32_HVC_INSN                       UINT32_C(18)
/** AArch32 - SMC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH32_SMC_INSN                       UINT32_C(19)
/** AArch64 - SVC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH64_SVC_INSN                       UINT32_C(21)
/** AArch64 - HVC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH64_HVC_INSN                       UINT32_C(22)
/** AArch64 - SMC instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH64_SMC_INSN                       UINT32_C(23)
/** AArch64 - Trapped MSR, MRS or System instruction execution in AArch64 state. */
#define ARMV8_ESR_EL2_EC_AARCH64_TRAPPED_SYS_INSN               UINT32_C(24)
/** FEAT_SVE - Access to SVE vunctionality not reported using ARMV8_ESR_EL2_EC_UNKNOWN. */
#define ARMV8_ESR_EL2_EC_SVE_TRAPPED                            UINT32_C(25)
/** FEAT_PAuth and FEAT_NV - Trapped ERET, ERETAA or ERTAB instruction. */
#define ARMV8_ESR_EL2_EC_PAUTH_NV_TRAPPED_ERET_ERETAA_ERETAB    UINT32_C(26)
/** FEAT_TME - Exception from TSTART instruction. */
#define ARMV8_ESR_EL2_EC_TME_TSTART_INSN_EXCEPTION              UINT32_C(27)
/** FEAT_FPAC - Exception from a Pointer Authentication instruction failure. */
#define ARMV8_ESR_EL2_EC_FPAC_PA_INSN_FAILURE_EXCEPTION         UINT32_C(28)
/** FEAT_SME - Access to SME functionality trapped. */
#define ARMV8_ESR_EL2_EC_SME_TRAPPED_SME_ACCESS                 UINT32_C(29)
/** FEAT_RME - Exception from Granule Protection Check. */
#define ARMV8_ESR_EL2_EC_RME_GRANULE_PROT_CHECK_EXCEPTION       UINT32_C(30)
/** Instruction Abort from a lower Exception level. */
#define ARMV8_ESR_EL2_INSN_ABORT_FROM_LOWER_EL                  UINT32_C(32)
/** Instruction Abort from the same Exception level. */
#define ARMV8_ESR_EL2_INSN_ABORT_FROM_EL2                       UINT32_C(33)
/** PC alignment fault exception. */
#define ARMV8_ESR_EL2_PC_ALIGNMENT_EXCEPTION                    UINT32_C(34)
/** Data Abort from a lower Exception level. */
#define ARMV8_ESR_EL2_DATA_ABORT_FROM_LOWER_EL                  UINT32_C(36)
/** Data Abort from the same Exception level (or access associated with VNCR_EL2). */
#define ARMV8_ESR_EL2_DATA_ABORT_FROM_EL2                       UINT32_C(37)
/** SP alignment fault exception. */
#define ARMV8_ESR_EL2_SP_ALIGNMENT_EXCEPTION                    UINT32_C(38)
/** FEAT_MOPS - Memory Operation Exception. */
#define ARMV8_ESR_EL2_EC_MOPS_EXCEPTION                         UINT32_C(39)
/** AArch32 - Trapped floating point exception. */
#define ARMV8_ESR_EL2_EC_AARCH32_TRAPPED_FP_EXCEPTION           UINT32_C(40)
/** AArch64 - Trapped floating point exception. */
#define ARMV8_ESR_EL2_EC_AARCH64_TRAPPED_FP_EXCEPTION           UINT32_C(44)
/** SError interrupt. */
#define ARMV8_ESR_EL2_SERROR_INTERRUPT                          UINT32_C(47)
/** Breakpoint Exception from a lower Exception level. */
#define ARMV8_ESR_EL2_BKPT_EXCEPTION_FROM_LOWER_EL              UINT32_C(48)
/** Breakpoint Exception from the same Exception level. */
#define ARMV8_ESR_EL2_BKPT_EXCEPTION_FROM_EL2                   UINT32_C(49)
/** Software Step Exception from a lower Exception level. */
#define ARMV8_ESR_EL2_SS_EXCEPTION_FROM_LOWER_EL                UINT32_C(50)
/** Software Step Exception from the same Exception level. */
#define ARMV8_ESR_EL2_SS_EXCEPTION_FROM_EL2                     UINT32_C(51)
/** Watchpoint Exception from a lower Exception level. */
#define ARMV8_ESR_EL2_WATCHPOINT_EXCEPTION_FROM_LOWER_EL        UINT32_C(52)
/** Watchpoint Exception from the same Exception level. */
#define ARMV8_ESR_EL2_WATCHPOINT_EXCEPTION_FROM_EL2             UINT32_C(53)
/** AArch32 - BKPT instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH32_BKPT_INSN                      UINT32_C(56)
/** AArch32 - Vector Catch exception. */
#define ARMV8_ESR_EL2_EC_AARCH32_VEC_CATCH_EXCEPTION            UINT32_C(58)
/** AArch64 - BRK instruction execution. */
#define ARMV8_ESR_EL2_EC_AARCH64_BRK_INSN                       UINT32_C(60)
/** @} */


/** @name ISS encoding for Data Abort exceptions.
 * @{ */
/** Bit 0 - 5 - DFSC - Data Fault Status Code. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC                             (  RT_BIT_32(0) | RT_BIT_32(1) | RT_BIT_32(2) \
                                                                 | RT_BIT_32(3) | RT_BIT_32(4) | RT_BIT_32(5))
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_GET(a_Iss)                  ((a_Iss) & ARMV8_EC_ISS_DATA_ABRT_DFSC)
/** Bit 6 - WnR - Write not Read. */
#define ARMV8_EC_ISS_DATA_ABRT_WNR                              RT_BIT_32(6)
#define ARMV8_EC_ISS_DATA_ABRT_WNR_BIT                          6
/** Bit 7 - S1PTW - Stage 2 translation fault for an access made for a stage 1 translation table walk. */
#define ARMV8_EC_ISS_DATA_ABRT_S1PTW                            RT_BIT_32(7)
#define ARMV8_EC_ISS_DATA_ABRT_S1PTW_BIT                        7
/** Bit 8 - CM - Cache maintenance instruction. */
#define ARMV8_EC_ISS_DATA_ABRT_CM                               RT_BIT_32(8)
#define ARMV8_EC_ISS_DATA_ABRT_CM_BIT                           8
/** Bit 9 - EA - External abort type. */
#define ARMV8_EC_ISS_DATA_ABRT_EA                               RT_BIT_32(9)
#define ARMV8_EC_ISS_DATA_ABRT_EA_BIT                           9
/** Bit 10 - FnV - FAR not Valid. */
#define ARMV8_EC_ISS_DATA_ABRT_FNV                              RT_BIT_32(10)
#define ARMV8_EC_ISS_DATA_ABRT_FNV_BIT                          10
/** Bit 11 - 12 - LST - Load/Store Type. */
#define ARMV8_EC_ISS_DATA_ABRT_LST                              (RT_BIT_32(11) | RT_BIT_32(12))
#define ARMV8_EC_ISS_DATA_ABRT_LST_GET(a_Iss)                   (((a_Iss) & ARMV8_EC_ISS_DATA_ABRT_LST) >> 11)
/** Bit 13 - VNCR - Fault came from use of VNCR_EL2 register by EL1 code. */
#define ARMV8_EC_ISS_DATA_ABRT_VNCR                             RT_BIT_32(13)
#define ARMV8_EC_ISS_DATA_ABRT_VNCR_BIT                         13
/** Bit 14 - AR - Acquire/Release semantics. */
#define ARMV8_EC_ISS_DATA_ABRT_AR                               RT_BIT_32(14)
#define ARMV8_EC_ISS_DATA_ABRT_AR_BIT                           14
/** Bit 15 - SF - Sixty Four bit general-purpose register transfer (only when ISV is 1). */
#define ARMV8_EC_ISS_DATA_ABRT_SF                               RT_BIT_32(15)
#define ARMV8_EC_ISS_DATA_ABRT_SF_BIT                           15
/** Bit 16 - 20 - SRT - Syndrome Register Transfer. */
#define ARMV8_EC_ISS_DATA_ABRT_SRT                              (  RT_BIT_32(16) | RT_BIT_32(17) | RT_BIT_32(18) \
                                                                 | RT_BIT_32(19) | RT_BIT_32(20))
#define ARMV8_EC_ISS_DATA_ABRT_SRT_GET(a_Iss)                   (((a_Iss) & ARMV8_EC_ISS_DATA_ABRT_SRT) >> 16)
/** Bit 21 - SSE - Syndrome Sign Extend. */
#define ARMV8_EC_ISS_DATA_ABRT_SSE                              RT_BIT_32(21)
#define ARMV8_EC_ISS_DATA_ABRT_SSE_BIT                          21
/** Bit 22 - 23 - SAS - Syndrome Access Size. */
#define ARMV8_EC_ISS_DATA_ABRT_SAS                              (RT_BIT_32(22) | RT_BIT_32(23))
#define ARMV8_EC_ISS_DATA_ABRT_SAS_GET(a_Iss)                   (((a_Iss) & ARMV8_EC_ISS_DATA_ABRT_SAS) >> 22)
/** Bit 24 - ISV - Instruction Syndrome Valid. */
#define ARMV8_EC_ISS_DATA_ABRT_ISV                              RT_BIT_32(24)
#define ARMV8_EC_ISS_DATA_ABRT_ISV_BIT                          24


/** @name Data Fault Status Code (DFSC).
 * @{ */
/** Address size fault, level 0 of translation or translation table base register. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ADDR_SIZE_FAULT_LVL0        0
/** Address size fault, level 1. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ADDR_SIZE_FAULT_LVL1        1
/** Address size fault, level 2. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ADDR_SIZE_FAULT_LVL2        2
/** Address size fault, level 3. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ADDR_SIZE_FAULT_LVL3        3
/** Translation fault, level 0. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_TRANSLATION_FAULT_LVL0      4
/** Translation fault, level 1. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_TRANSLATION_FAULT_LVL1      5
/** Translation fault, level 2. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_TRANSLATION_FAULT_LVL2      6
/** Translation fault, level 3. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_TRANSLATION_FAULT_LVL3      7
/** FEAT_LPA2 - Access flag fault, level 0. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ACCESS_FLAG_FAULT_LVL0      8
/** Access flag fault, level 1. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ACCESS_FLAG_FAULT_LVL1      9
/** Access flag fault, level 2. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ACCESS_FLAG_FAULT_LVL2      10
/** Access flag fault, level 3. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_ACCESS_FLAG_FAULT_LVL3      11
/** FEAT_LPA2 - Permission fault, level 0. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_PERMISSION_FAULT_LVL0       12
/** Permission fault, level 1. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_PERMISSION_FAULT_LVL1       13
/** Permission fault, level 2. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_PERMISSION_FAULT_LVL2       14
/** Permission fault, level 3. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_PERMISSION_FAULT_LVL3       15
/** Synchronous External abort, not a translation table walk or hardware update of translation table. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_SYNC_EXTERNAL               16
/** FEAT_MTE2 - Synchronous Tag Check Fault. */
#define ARMV8_EC_ISS_DATA_ABRT_DFSC_MTE2_SYNC_TAG_CHK_FAULT     17
/** @todo Do the rest (lazy developer). */
/** @} */


/** @name SAS encoding. */
/** Byte access. */
#define ARMV8_EC_ISS_DATA_ABRT_SAS_BYTE                         0
/** Halfword access (uint16_t). */
#define ARMV8_EC_ISS_DATA_ABRT_SAS_HALFWORD                     1
/** Word access (uint32_t). */
#define ARMV8_EC_ISS_DATA_ABRT_SAS_WORD                         2
/** Doubleword access (uint64_t). */
#define ARMV8_EC_ISS_DATA_ABRT_SAS_DWORD                        3
/** @} */

/** @} */


/** @name ISS encoding for trapped MSR, MRS or System instruction exceptions.
 * @{ */
/** Bit 0 - Direction flag. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_DIRECTION         RT_BIT_32(0)
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_DIRECTION_IS_READ(a_Iss) RT_BOOL((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_DIRECTION)
/** Bit 1 - 4 - CRm value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRM               (  RT_BIT_32(1) | RT_BIT_32(2) | RT_BIT_32(3) \
                                                                 | RT_BIT_32(4))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRM_GET(a_Iss)    (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRM) >> 10)
/** Bit 5 - 9 - Rt value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_RT                (  RT_BIT_32(5) | RT_BIT_32(6) | RT_BIT_32(7) \
                                                                 | RT_BIT_32(8) | RT_BIT_32(9))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_RT_GET(a_Iss)     (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_RT) >> 5)
/** Bit 10 - 13 - CRn value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRN               (  RT_BIT_32(10) | RT_BIT_32(11) | RT_BIT_32(12) \
                                                                 | RT_BIT_32(13))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRN_GET(a_Iss)    (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_CRN) >> 10)
/** Bit 14 - 16 - Op2 value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP1               (RT_BIT_32(14) | RT_BIT_32(15) | RT_BIT_32(16))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP1_GET(a_Iss)    (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP1) >> 14)
/** Bit 17 - 19 - Op2 value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP2               (RT_BIT_32(17) | RT_BIT_32(18) | RT_BIT_32(19))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP2_GET(a_Iss)    (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP2) >> 17)
/** Bit 20 - 21 - Op0 value from the instruction. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP0               (RT_BIT_32(20) | RT_BIT_32(21))
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP0_GET(a_Iss)    (((a_Iss) & ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_OP0) >> 20)
/** Bit 22 - 24 - Reserved. */
#define ARMV8_EC_ISS_AARCH64_TRAPPED_SYS_INSN_RSVD              (RT_BIT_32(22) | RT_BIT_32(23) | RT_BIT_32(24))
/** @} */

/** @} */

#endif /* !IPRT_INCLUDED_armv8_h */

