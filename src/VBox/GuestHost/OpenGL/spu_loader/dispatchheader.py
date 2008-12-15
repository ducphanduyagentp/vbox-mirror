# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This script generates the spu_dispatch_table.h file from gl_header.parsed

import sys, string

import apiutil


apiutil.CopyrightC()

print """
/* DO NOT EDIT - THIS FILE GENERATED BY THE dispatchheader.py SCRIPT */

#ifndef CR_SPU_DISPATCH_TABLE_H
#define CR_SPU_DISPATCH_TABLE_H

#ifdef WINDOWS
#define SPU_APIENTRY __stdcall
#else
#define SPU_APIENTRY
#endif

#include "chromium.h"
#include "state/cr_statetypes.h"
"""

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")


print '/* Offsets of each function within the dispatch table */'
offset = 0
for func_name in keys:
	print '#define DISPATCH_OFFSET_%s %d' % (func_name, offset)
	offset += 1
print ''

print '/* Function typedefs */'
for func_name in keys:
	return_type = apiutil.ReturnType(func_name)
	params = apiutil.Parameters(func_name)

	print 'typedef %s (SPU_APIENTRY *%sFunc_t)(%s);' % (return_type, func_name, apiutil.MakePrototypeString(params))
print ''

print 'struct _copy_list_node;'
print ''
print '/* The SPU dispatch table */'
print 'typedef struct _spu_dispatch_table {'

for func_name in keys:
	print "\t%sFunc_t %s; " % ( func_name, func_name )

print """
	struct _copy_list_node *copyList;
	struct _spu_dispatch_table *copy_of;
	int mark;
	void *server;		
} SPUDispatchTable;

struct _copy_list_node {
    SPUDispatchTable *copy;
    struct _copy_list_node *next;
};


#endif /* CR_SPU_DISPATCH_TABLE_H */
"""
