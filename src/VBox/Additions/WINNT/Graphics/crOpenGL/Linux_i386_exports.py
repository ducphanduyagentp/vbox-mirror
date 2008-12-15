# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


import sys

import apiutil


def GenerateEntrypoints():

	apiutil.CopyrightC()

	# Get sorted list of dispatched functions.
	# The order is very important - it must match cr_opcodes.h
	# and spu_dispatch_table.h
	keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

	for index in range(len(keys)):
		func_name = keys[index]
		if apiutil.Category(func_name) == "Chromium":
			continue

		print "\t.align 4"
		print ".globl gl%s" % func_name
		print "\t.type gl%s,@function" % func_name
		print "gl%s:" % func_name
		print "\tmovl glim+%d, %%eax" % (4*index)
		print "\tjmp *%eax"
		print ""


	print '/*'
	print '* Aliases'
	print '*/'

	# Now loop over all the functions and take care of any aliases
	allkeys = apiutil.GetAllFunctions(sys.argv[1]+"/APIspec.txt")
	for func_name in allkeys:
		if "omit" in apiutil.ChromiumProps(func_name):
			continue

		if func_name in keys:
			# we already processed this function earlier
			continue

		# alias is the function we're aliasing
		alias = apiutil.Alias(func_name)
		if alias:
			# this dict lookup should never fail (raise an exception)!
			index = keys.index(alias)
			print "\t.align 4"
			print ".globl gl%s" % func_name
			print "\t.type gl%s,@function" % func_name
			print "gl%s:" % func_name
			print "\tmovl glim+%d, %%eax" % (4*index)
			print "\tjmp *%eax"
			print ""


	print '/*'
	print '* No-op stubs'
	print '*/'

	# Now generate no-op stub functions
	for func_name in allkeys:
		if "stub" in apiutil.ChromiumProps(func_name):
			print "\t.align 4"
			print ".globl gl%s" % func_name
			print "\t.type gl%s,@function" % func_name
			print "gl%s:" % func_name
			print "\tleave"
			print "\tret"
			print ""


GenerateEntrypoints()

