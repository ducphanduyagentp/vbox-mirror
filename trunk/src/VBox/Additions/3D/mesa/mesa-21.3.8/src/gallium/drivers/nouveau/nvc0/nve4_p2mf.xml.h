#ifndef NVE4_P2MF_XML
#define NVE4_P2MF_XML

/* Autogenerated file, DO NOT EDIT manually!

This file was generated by the rules-ng-ng headergen tool in this git repository:
http://github.com/envytools/envytools/
git clone https://github.com/envytools/envytools.git

The rules-ng-ng source files this header was generated from are:
- rnndb/graph/gf100_3d.xml   (  60037 bytes, from 2014-12-31 02:40:43)
- rnndb/copyright.xml        (   6456 bytes, from 2014-12-31 02:13:31)
- rnndb/nv_defs.xml          (   4399 bytes, from 2013-09-07 03:32:45)
- rnndb/graph/nv_3ddefs.xml  (  16390 bytes, from 2014-09-25 06:32:11)
- rnndb/fifo/nv_object.xml   (  15326 bytes, from 2014-09-25 06:32:11)
- rnndb/nvchipsets.xml       (   2759 bytes, from 2014-10-05 01:51:02)
- rnndb/g80_defs.xml         (  18175 bytes, from 2014-09-25 06:32:11)
- rnndb/graph/gk104_p2mf.xml (   2376 bytes, from 2014-09-25 06:32:11)

Copyright (C) 2006-2014 by the following authors:
- Artur Huillet <arthur.huillet@free.fr> (ahuillet)
- Ben Skeggs (darktama, darktama_)
- B. R. <koala_br@users.sourceforge.net> (koala_br)
- Carlos Martin <carlosmn@users.sf.net> (carlosmn)
- Christoph Bumiller <e0425955@student.tuwien.ac.at> (calim, chrisbmr)
- Dawid Gajownik <gajownik@users.sf.net> (gajownik)
- Dmitry Baryshkov
- Dmitry Eremin-Solenikov <lumag@users.sf.net> (lumag)
- EdB <edb_@users.sf.net> (edb_)
- Erik Waling <erikwailing@users.sf.net> (erikwaling)
- Francisco Jerez <currojerez@riseup.net> (curro)
- Ilia Mirkin <imirkin@alum.mit.edu> (imirkin)
- jb17bsome <jb17bsome@bellsouth.net> (jb17bsome)
- Jeremy Kolb <kjeremy@users.sf.net> (kjeremy)
- Laurent Carlier <lordheavym@gmail.com> (lordheavy)
- Luca Barbieri <luca@luca-barbieri.com> (lb, lb1)
- Maarten Maathuis <madman2003@gmail.com> (stillunknown)
- Marcin Kościelnicki <koriakin@0x04.net> (mwk, koriakin)
- Mark Carey <mark.carey@gmail.com> (careym)
- Matthieu Castet <matthieu.castet@parrot.com> (mat-c)
- nvidiaman <nvidiaman@users.sf.net> (nvidiaman)
- Patrice Mandin <patmandin@gmail.com> (pmandin, pmdata)
- Pekka Paalanen <pq@iki.fi> (pq, ppaalanen)
- Peter Popov <ironpeter@users.sf.net> (ironpeter)
- Richard Hughes <hughsient@users.sf.net> (hughsient)
- Rudi Cilibrasi <cilibrar@users.sf.net> (cilibrar)
- Serge Martin
- Simon Raffeiner
- Stephane Loeuillet <leroutier@users.sf.net> (leroutier)
- Stephane Marchesin <stephane.marchesin@gmail.com> (marcheu)
- sturmflut <sturmflut@users.sf.net> (sturmflut)
- Sylvain Munaut <tnt@246tNt.com>
- Victor Stinner <victor.stinner@haypocalc.com> (haypo)
- Wladmir van der Laan <laanwj@gmail.com> (miathan6)
- Younes Manton <younes.m@gmail.com> (ymanton)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/




#define NVE4_P2MF_UNK0144					0x00000144

#define NVE4_P2MF_UPLOAD					0x00000000

#define NVE4_P2MF_UPLOAD_LINE_LENGTH_IN			0x00000180

#define NVE4_P2MF_UPLOAD_LINE_COUNT				0x00000184

#define NVE4_P2MF_UPLOAD_DST_ADDRESS_HIGH			0x00000188

#define NVE4_P2MF_UPLOAD_DST_ADDRESS_LOW			0x0000018c

#define NVE4_P2MF_UPLOAD_DST_PITCH				0x00000190

#define NVE4_P2MF_UPLOAD_DST_TILE_MODE				0x00000194

#define NVE4_P2MF_UPLOAD_DST_WIDTH				0x00000198

#define NVE4_P2MF_UPLOAD_DST_HEIGHT				0x0000019c

#define NVE4_P2MF_UPLOAD_DST_DEPTH				0x000001a0

#define NVE4_P2MF_UPLOAD_DST_Z					0x000001a4

#define NVE4_P2MF_UPLOAD_DST_X					0x000001a8

#define NVE4_P2MF_UPLOAD_DST_Y					0x000001ac

#define NVE4_P2MF_UPLOAD_EXEC					0x000001b0
#define NVE4_P2MF_UPLOAD_EXEC_LINEAR				0x00000001
#define NVE4_P2MF_UPLOAD_EXEC_UNK1__MASK			0x0000007e
#define NVE4_P2MF_UPLOAD_EXEC_UNK1__SHIFT			1
#define NVE4_P2MF_UPLOAD_EXEC_BUF_NOTIFY			0x00000300
#define NVE4_P2MF_UPLOAD_EXEC_UNK12__MASK			0x0000f000
#define NVE4_P2MF_UPLOAD_EXEC_UNK12__SHIFT			12

#define NVE4_P2MF_UPLOAD_DATA					0x000001b4

#define NVE4_P2MF_UPLOAD_QUERY_ADDRESS_HIGH			0x000001dc

#define NVE4_P2MF_UPLOAD_QUERY_ADDRESS_LOW			0x000001e0

#define NVE4_P2MF_UPLOAD_QUERY_SEQUENCE			0x000001e4

#define NVE4_P2MF_UPLOAD_UNK01F0				0x000001f0

#define NVE4_P2MF_UPLOAD_UNK01F4				0x000001f4

#define NVE4_P2MF_UPLOAD_UNK01F8				0x000001f8

#define NVE4_P2MF_UPLOAD_UNK01FC				0x000001fc

#define NVE4_P2MF_FIRMWARE(i0)				       (0x00000200 + 0x4*(i0))
#define NVE4_P2MF_FIRMWARE__ESIZE				0x00000004
#define NVE4_P2MF_FIRMWARE__LEN				0x00000020

#define NVE4_P2MF_COND_ADDRESS_HIGH				0x00001550

#define NVE4_P2MF_COND_ADDRESS_LOW				0x00001554

#define NVE4_P2MF_COND_MODE					0x00001558
#define NVE4_P2MF_COND_MODE_NEVER				0x00000000
#define NVE4_P2MF_COND_MODE_ALWAYS				0x00000001
#define NVE4_P2MF_COND_MODE_RES_NON_ZERO			0x00000002
#define NVE4_P2MF_COND_MODE_EQUAL				0x00000003
#define NVE4_P2MF_COND_MODE_NOT_EQUAL				0x00000004

#define NVE4_P2MF_UNK1944					0x00001944


#endif /* NVE4_P2MF_XML */
