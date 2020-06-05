/* $Id$ */
/** @file
 * DBGF - Debugger Facility, CPU State Accessors.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#define VMCPU_INCL_CPUM_GST_CTX /* For CPUM_IMPORT_EXTRN_RET(). */
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/cpum.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <iprt/errcore.h>
#include <VBox/log.h>
#include <VBox/param.h>
#include <iprt/assert.h>


/**
 * Wrapper around CPUMGetGuestMode.
 *
 * @returns VINF_SUCCESS.
 * @param   pVM         The cross context VM structure.
 * @param   idCpu       The current CPU ID.
 * @param   penmMode    Where to return the mode.
 */
static DECLCALLBACK(int) dbgfR3CpuGetMode(PVM pVM, VMCPUID idCpu, CPUMMODE *penmMode)
{
    Assert(idCpu == VMMGetCpuId(pVM));
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    CPUM_IMPORT_EXTRN_RET(pVCpu, CPUMCTX_EXTRN_CR0 | CPUMCTX_EXTRN_EFER);
    *penmMode = CPUMGetGuestMode(pVCpu);
    return VINF_SUCCESS;
}


/**
 * Get the current CPU mode.
 *
 * @returns The CPU mode on success, CPUMMODE_INVALID on failure.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The target CPU ID.
 */
VMMR3DECL(CPUMMODE) DBGFR3CpuGetMode(PUVM pUVM, VMCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, CPUMMODE_INVALID);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, CPUMMODE_INVALID);
    AssertReturn(idCpu < pUVM->pVM->cCpus, CPUMMODE_INVALID);

    CPUMMODE enmMode;
    int rc = VMR3ReqPriorityCallWaitU(pUVM, idCpu, (PFNRT)dbgfR3CpuGetMode, 3, pUVM->pVM, idCpu, &enmMode);
    if (RT_FAILURE(rc))
        return CPUMMODE_INVALID;
    return enmMode;
}


/**
 * Wrapper around CPUMIsGuestIn64BitCode.
 *
 * @returns VINF_SUCCESS.
 * @param   pVM             The cross context VM structure.
 * @param   idCpu           The current CPU ID.
 * @param   pfIn64BitCode   Where to return the result.
 */
static DECLCALLBACK(int) dbgfR3CpuIn64BitCode(PVM pVM, VMCPUID idCpu, bool *pfIn64BitCode)
{
    Assert(idCpu == VMMGetCpuId(pVM));
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    CPUM_IMPORT_EXTRN_RET(pVCpu, CPUMCTX_EXTRN_CS | CPUMCTX_EXTRN_EFER);
    *pfIn64BitCode = CPUMIsGuestIn64BitCode(pVCpu);
    return VINF_SUCCESS;
}


/**
 * Checks if the given CPU is executing 64-bit code or not.
 *
 * @returns true / false accordingly.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The target CPU ID.
 */
VMMR3DECL(bool) DBGFR3CpuIsIn64BitCode(PUVM pUVM, VMCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, false);
    AssertReturn(idCpu < pUVM->pVM->cCpus, false);

    bool fIn64BitCode;
    int rc = VMR3ReqPriorityCallWaitU(pUVM, idCpu, (PFNRT)dbgfR3CpuIn64BitCode, 3, pUVM->pVM, idCpu, &fIn64BitCode);
    if (RT_FAILURE(rc))
        return false;
    return fIn64BitCode;
}


/**
 * Wrapper around CPUMIsGuestInV86Code.
 *
 * @returns VINF_SUCCESS.
 * @param   pVM             The cross context VM structure.
 * @param   idCpu           The current CPU ID.
 * @param   pfInV86Code     Where to return the result.
 */
static DECLCALLBACK(int) dbgfR3CpuInV86Code(PVM pVM, VMCPUID idCpu, bool *pfInV86Code)
{
    Assert(idCpu == VMMGetCpuId(pVM));
    PVMCPU pVCpu = VMMGetCpuById(pVM, idCpu);
    CPUM_IMPORT_EXTRN_RET(pVCpu, CPUMCTX_EXTRN_RFLAGS);
    *pfInV86Code = CPUMIsGuestInV86ModeEx(CPUMQueryGuestCtxPtr(pVCpu));
    return VINF_SUCCESS;
}


/**
 * Checks if the given CPU is executing V8086 code or not.
 *
 * @returns true / false accordingly.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The target CPU ID.
 */
VMMR3DECL(bool) DBGFR3CpuIsInV86Code(PUVM pUVM, VMCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, false);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, false);
    AssertReturn(idCpu < pUVM->pVM->cCpus, false);

    bool fInV86Code;
    int rc = VMR3ReqPriorityCallWaitU(pUVM, idCpu, (PFNRT)dbgfR3CpuInV86Code, 3, pUVM->pVM, idCpu, &fInV86Code);
    if (RT_FAILURE(rc))
        return false;
    return fInV86Code;
}


/**
 * Get the number of CPUs (or threads if you insist).
 *
 * @returns The number of CPUs
 * @param   pUVM        The user mode VM handle.
 */
VMMR3DECL(VMCPUID) DBGFR3CpuGetCount(PUVM pUVM)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, 1);
    return pUVM->cCpus;
}


/**
 * Returns the state of the given CPU as a human readable string.
 *
 * @returns Pointer to the human readable CPU state string.
 * @param   pUVM        The user mode VM handle.
 * @param   idCpu       The target CPU ID.
 */
VMMR3DECL(const char *) DBGFR3CpuGetState(PUVM pUVM, VMCPUID idCpu)
{
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, NULL);
    VM_ASSERT_VALID_EXT_RETURN(pUVM->pVM, NULL);
    AssertReturn(idCpu < pUVM->pVM->cCpus, NULL);

    PVMCPU pVCpu = VMMGetCpuById(pUVM->pVM, idCpu);
    VMCPUSTATE enmCpuState;
    ASMAtomicReadSize(&pVCpu->enmState, &enmCpuState);

    switch (enmCpuState)
    {
        case VMCPUSTATE_INVALID:                   return "<INVALID>";
        case VMCPUSTATE_STOPPED:                   return "Stopped";
        case VMCPUSTATE_STARTED:                   return "Started";
        case VMCPUSTATE_STARTED_HM:                return "Started (HM)";
        case VMCPUSTATE_STARTED_EXEC:              return "Started (Exec)";
        case VMCPUSTATE_STARTED_EXEC_NEM:          return "Started (Exec NEM)";
        case VMCPUSTATE_STARTED_EXEC_NEM_WAIT:     return "Started (Exec NEM Wait)";
        case VMCPUSTATE_STARTED_EXEC_NEM_CANCELED: return "Started (Exec NEM Canceled)";
        case VMCPUSTATE_STARTED_HALTED:            return "Started (Halted)";
        case VMCPUSTATE_END:                       return "END";
        default:
            AssertMsgFailedReturn(("Unknown CPU state %u\n", enmCpuState), "<UNKNOWN>");
    }

    /* This indicates a compiler bug. */
    return NULL;
}

