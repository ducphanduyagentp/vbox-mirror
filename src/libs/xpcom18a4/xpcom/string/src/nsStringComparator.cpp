/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#include <iprt/string.h>

#include <ctype.h>
#include "nsAString.h"

  // define nsStringComparator
#include "string-template-def-unichar.h"
#include "nsTStringComparator.cpp"
#include "string-template-undef.h"

  // define nsCStringComparator
#include "string-template-def-char.h"
#include "nsTStringComparator.cpp"
#include "string-template-undef.h"


int
nsCaseInsensitiveCStringComparator::operator()( const char_type* lhs, const char_type* rhs, PRUint32 aLength ) const
  {
    PRInt32 result=PRInt32(RTStrNICmp(lhs, rhs, aLength));
    //Egads. PL_strncasecmp is returning *very* negative numbers.
    //Some folks expect -1,0,1, so let's temper its enthusiasm.
    if (result<0) 
      result=-1;
    return result;
  }

int
nsCaseInsensitiveCStringComparator::operator()( char lhs, char rhs ) const
  {
    if (lhs == rhs) return 0;
    
    lhs = tolower(lhs);
    rhs = tolower(rhs);

    return lhs - rhs;
  }
