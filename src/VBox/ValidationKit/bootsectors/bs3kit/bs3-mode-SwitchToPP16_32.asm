; $Id$
;; @file
; BS3Kit - Bs3SwitchToPP16_32
;

;
; Copyright (C) 2007-2022 Oracle and/or its affiliates.
;
; This file is part of VirtualBox base platform packages, as
; available from https://www.virtualbox.org.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation, in version 3 of the
; License.
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses>.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
; in the VirtualBox distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;
; SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
;

%include "bs3kit-template-header.mac"


;;
; Switch to 32-bit code under 16-bit paged protected mode sys/tss from any other mode.
;
; @cproto   BS3_DECL(void) Bs3SwitchToPP16_32(void);
;
; @uses     Nothing (except high 32-bit register parts).
;
; @remarks  Obviously returns to 32-bit mode, even if the caller was
;           in 16-bit or 64-bit mode.
;
; @remarks  Does not require 20h of parameter scratch space in 64-bit mode.
;
BS3_GLOBAL_NAME_EX TMPL_NM(Bs3SwitchToPP16_32_Safe), function, 0
BS3_PROC_BEGIN_MODE Bs3SwitchToPP16_32, BS3_PBC_NEAR
%ifdef TMPL_PP16_32
        ret

%else
        ;
        ; Make sure we're in the 16-bit segment and then call Bs3SwitchToPP16.
        ;
 %if TMPL_BITS != 16
        jmp     .sixteen_bit_segment
BS3_BEGIN_TEXT16
        BS3_SET_BITS TMPL_BITS
BS3_GLOBAL_LOCAL_LABEL .sixteen_bit_segment
 %endif
        extern  TMPL_NM(Bs3SwitchToPP16)
        call    TMPL_NM(Bs3SwitchToPP16)
        BS3_SET_BITS 16

        ;
        ; Switch to 32-bit mode.
        ;
        extern  _Bs3SwitchTo32Bit_c16
 %if TMPL_BITS == 16
        jmp     _Bs3SwitchTo32Bit_c16
 %else
        call    _Bs3SwitchTo32Bit_c16
        BS3_SET_BITS 32
  %if TMPL_BITS == 32
        ret
  %else
        ret     4                       ; Return and pop 4 bytes of "parameters" (unused return address).
  %endif
 %endif
%endif
BS3_PROC_END_MODE   Bs3SwitchToPP16_32


%if TMPL_BITS == 16
;;
; Custom far stub.
BS3_BEGIN_TEXT16_FARSTUBS
BS3_PROC_BEGIN_MODE Bs3SwitchToPP16_32, BS3_PBC_FAR
        inc         bp
        push        bp
        mov         bp, sp

        ; Call the real thing.
        call        TMPL_NM(Bs3SwitchToPP16_32)
        BS3_SET_BITS 32

        ; Jmp to common code for the tedious conversion.
 %if BS3_MODE_IS_RM_OR_V86(TMPL_MODE)
        extern      _Bs3SwitchHlpConvRealModeRetfPopBpDecBpAndReturn_c32
        jmp         _Bs3SwitchHlpConvRealModeRetfPopBpDecBpAndReturn_c32
 %else
        extern      _Bs3SwitchHlpConvProtModeRetfPopBpDecBpAndReturn_c32
        jmp         _Bs3SwitchHlpConvProtModeRetfPopBpDecBpAndReturn_c32
 %endif
        BS3_SET_BITS 16
BS3_PROC_END_MODE   Bs3SwitchToPP16_32
%endif

