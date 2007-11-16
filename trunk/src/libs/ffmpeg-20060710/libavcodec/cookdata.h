/*
 * COOK compatible decoder data
 * Copyright (c) 2003 Sascha Sommer
 * Copyright (c) 2005 Benjamin Larsson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/**
 * @file cookdata.h
 * Cook AKA RealAudio G2 compatible decoderdata
 */

/* various data tables */

static const int expbits_tab[8] = {
    52,47,43,37,29,22,16,0,
};

static const float dither_tab[8] = {
  0.0, 0.0, 0.0, 0.0, 0.0, 0.176777, 0.25, 0.707107,
};

static const float randsign[2] = {1.0, -1.0};

static const float quant_centroid_tab[7][14] = {
  { 0.000, 0.392, 0.761, 1.120, 1.477, 1.832, 2.183, 2.541, 2.893, 3.245, 3.598, 3.942, 4.288, 4.724 },
  { 0.000, 0.544, 1.060, 1.563, 2.068, 2.571, 3.072, 3.562, 4.070, 4.620, 0.000, 0.000, 0.000, 0.000 },
  { 0.000, 0.746, 1.464, 2.180, 2.882, 3.584, 4.316, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 },
  { 0.000, 1.006, 2.000, 2.993, 3.985, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 },
  { 0.000, 1.321, 2.703, 3.983, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 },
  { 0.000, 1.657, 3.491, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 },
  { 0.000, 1.964, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000 }
};

static const int invradix_tab[7] = {
    74899, 104858, 149797, 209716, 262144, 349526, 524288,
};

static const int kmax_tab[7] = {
    13, 9, 6, 4, 3, 2, 1,
};

static const int vd_tab[7] = {
    2, 2, 2, 4, 4, 5, 5,
};

static const int vpr_tab[7] = {
    10, 10, 10, 5, 5, 4, 4,
};



/* VLC data */

static const int vhsize_tab[7] = {
    191, 97, 48, 607, 246, 230, 32,
};

static const int vhvlcsize_tab[7] = {
    8, 7, 7, 10, 9, 9, 6,
};

static const uint8_t envelope_quant_index_huffbits[13][24] = {
    {  4,  6,  5,  5,  4, 4, 4, 4, 4, 4, 3, 3, 3, 4, 5, 7,  8,  9, 11, 11, 12, 12, 12, 12 },
    { 10,  8,  6,  5,  5, 4, 3, 3, 3, 3, 3, 3, 4, 5, 7, 9, 11, 12, 13, 15, 15, 15, 16, 16 },
    { 12, 10,  8,  6,  5, 4, 4, 4, 4, 4, 4, 3, 3, 3, 4, 4,  5,  5,  7,  9, 11, 13, 14, 14 },
    { 13, 10,  9,  9,  7, 7, 5, 5, 4, 3, 3, 3, 3, 3, 4, 4,  4,  5,  7,  9, 11, 13, 13, 13 },
    { 12, 13, 10,  8,  6, 6, 5, 5, 4, 4, 3, 3, 3, 3, 3, 4,  5,  5,  6,  7,  9, 11, 14, 14 },
    { 12, 11,  9,  8,  8, 7, 5, 4, 4, 3, 3, 3, 3, 3, 4, 4,  5,  5,  7,  8, 10, 13, 14, 14 },
    { 15, 16, 15, 12, 10, 8, 6, 5, 4, 3, 3, 3, 2, 3, 4, 5,  5,  7,  9, 11, 13, 16, 16, 16 },
    { 14, 14, 11, 10,  9, 7, 7, 5, 5, 4, 3, 3, 2, 3, 3, 4,  5,  7,  9,  9, 12, 14, 15, 15 },
    {  9,  9,  9,  8,  7, 6, 5, 4, 3, 3, 3, 3, 3, 3, 4, 5,  6,  7,  8, 10, 11, 12, 13, 13 },
    { 14, 12, 10,  8,  6, 6, 5, 4, 3, 3, 3, 3, 3, 3, 4, 5,  6,  8,  8,  9, 11, 14, 14, 14 },
    { 13, 10,  9,  8,  6, 6, 5, 4, 4, 4, 3, 3, 2, 3, 4, 5,  6,  8,  9,  9, 11, 12, 14, 14 },
    { 16, 13, 12, 11,  9, 6, 5, 5, 4, 4, 4, 3, 2, 3, 3, 4,  5,  7,  8, 10, 14, 16, 16, 16 },
    { 13, 14, 14, 14, 10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4,  5,  5,  7,  9, 11, 14, 14, 14 },
};

static const uint16_t envelope_quant_index_huffcodes[13][24] = {
    {0x0006, 0x003e, 0x001c, 0x001d, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x0000, 0x0001,
     0x0002, 0x000d, 0x001e, 0x007e, 0x00fe, 0x01fe, 0x07fc, 0x07fd, 0x0ffc, 0x0ffd, 0x0ffe, 0x0fff},
    {0x03fe, 0x00fe, 0x003e, 0x001c, 0x001d, 0x000c, 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005,
     0x000d, 0x001e, 0x007e, 0x01fe, 0x07fe, 0x0ffe, 0x1ffe, 0x7ffc, 0x7ffd, 0x7ffe, 0xfffe, 0xffff},
    {0x0ffe, 0x03fe, 0x00fe, 0x003e, 0x001c, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x0000,
     0x0001, 0x0002, 0x000c, 0x000d, 0x001d, 0x001e, 0x007e, 0x01fe, 0x07fe, 0x1ffe, 0x3ffe, 0x3fff},
    {0x1ffc, 0x03fe, 0x01fc, 0x01fd, 0x007c, 0x007d, 0x001c, 0x001d, 0x000a, 0x0000, 0x0001, 0x0002,
     0x0003, 0x0004, 0x000b, 0x000c, 0x000d, 0x001e, 0x007e, 0x01fe, 0x07fe, 0x1ffd, 0x1ffe, 0x1fff},
    {0x0ffe, 0x1ffe, 0x03fe, 0x00fe, 0x003c, 0x003d, 0x001a, 0x001b, 0x000a, 0x000b, 0x0000, 0x0001,
     0x0002, 0x0003, 0x0004, 0x000c, 0x001c, 0x001d, 0x003e, 0x007e, 0x01fe, 0x07fe, 0x3ffe, 0x3fff},
    {0x0ffe, 0x07fe, 0x01fe, 0x00fc, 0x00fd, 0x007c, 0x001c, 0x000a, 0x000b, 0x0000, 0x0001, 0x0002,
     0x0003, 0x0004, 0x000c, 0x000d, 0x001d, 0x001e, 0x007d, 0x00fe, 0x03fe, 0x1ffe, 0x3ffe, 0x3fff},
    {0x7ffc, 0xfffc, 0x7ffd, 0x0ffe, 0x03fe, 0x00fe, 0x003e, 0x001c, 0x000c, 0x0002, 0x0003, 0x0004,
     0x0000, 0x0005, 0x000d, 0x001d, 0x001e, 0x007e, 0x01fe, 0x07fe, 0x1ffe, 0xfffd, 0xfffe, 0xffff},
    {0x3ffc, 0x3ffd, 0x07fe, 0x03fe, 0x01fc, 0x007c, 0x007d, 0x001c, 0x001d, 0x000c, 0x0002, 0x0003,
     0x0000, 0x0004, 0x0005, 0x000d, 0x001e, 0x007e, 0x01fd, 0x01fe, 0x0ffe, 0x3ffe, 0x7ffe, 0x7fff},
    {0x01fc, 0x01fd, 0x01fe, 0x00fc, 0x007c, 0x003c, 0x001c, 0x000c, 0x0000, 0x0001, 0x0002, 0x0003,
     0x0004, 0x0005, 0x000d, 0x001d, 0x003d, 0x007d, 0x00fd, 0x03fe, 0x07fe, 0x0ffe, 0x1ffe, 0x1fff},
    {0x3ffc, 0x0ffe, 0x03fe, 0x00fc, 0x003c, 0x003d, 0x001c, 0x000c, 0x0000, 0x0001, 0x0002, 0x0003,
     0x0004, 0x0005, 0x000d, 0x001d, 0x003e, 0x00fd, 0x00fe, 0x01fe, 0x07fe, 0x3ffd, 0x3ffe, 0x3fff},
    {0x1ffe, 0x03fe, 0x01fc, 0x00fc, 0x003c, 0x003d, 0x001c, 0x000a, 0x000b, 0x000c, 0x0002, 0x0003,
     0x0000, 0x0004, 0x000d, 0x001d, 0x003e, 0x00fd, 0x01fd, 0x01fe, 0x07fe, 0x0ffe, 0x3ffe, 0x3fff},
    {0xfffc, 0x1ffe, 0x0ffe, 0x07fe, 0x01fe, 0x003e, 0x001c, 0x001d, 0x000a, 0x000b, 0x000c, 0x0002,
     0x0000, 0x0003, 0x0004, 0x000d, 0x001e, 0x007e, 0x00fe, 0x03fe, 0x3ffe, 0xfffd, 0xfffe, 0xffff},
    {0x1ffc, 0x3ffa, 0x3ffb, 0x3ffc, 0x03fe, 0x00fe, 0x007c, 0x007d, 0x001c, 0x000c, 0x0002, 0x0003,
     0x0000, 0x0004, 0x0005, 0x000d, 0x001d, 0x001e, 0x007e, 0x01fe, 0x07fe, 0x3ffd, 0x3ffe, 0x3fff},
};


static const uint8_t cvh_huffbits0[191] = {
    1, 4, 6, 6, 7, 7, 8, 8, 8, 9, 9, 10,
    11, 11, 4, 5, 6, 7, 7, 8, 8, 9, 9, 9,
    9, 10, 11, 11, 5, 6, 7, 8, 8, 9, 9, 9,
    9, 10, 10, 10, 11, 12, 6, 7, 8, 9, 9, 9,
    9, 10, 10, 10, 10, 11, 12, 13, 7, 7, 8, 9,
    9, 9, 10, 10, 10, 10, 11, 11, 12, 13, 8, 8,
    9, 9, 9, 10, 10, 10, 10, 11, 11, 12, 13, 14,
    8, 8, 9, 9, 10, 10, 11, 11, 11, 12, 12, 13,
    13, 15, 8, 8, 9, 9, 10, 10, 11, 11, 11, 12,
    12, 13, 14, 15, 9, 9, 9, 10, 10, 10, 11, 11,
    12, 13, 12, 14, 15, 16, 9, 9, 10, 10, 10, 10,
    11, 12, 12, 14, 14, 16, 16, 0, 9, 9, 10, 10,
    11, 11, 12, 13, 13, 14, 14, 15, 0, 0, 10, 10,
    10, 11, 11, 12, 12, 13, 15, 15, 16, 0, 0, 0,
    11, 11, 11, 12, 13, 13, 13, 15, 16, 16, 0, 0,
    0, 0, 11, 11, 12, 13, 13, 14, 15, 16, 16,
};

static const uint16_t cvh_huffcodes0[191] = {
    0x0000,0x0008,0x002c,0x002d,0x0062,0x0063,0x00d4,0x00d5,0x00d6,0x01c6,0x01c7,0x03ca,
    0x07d6,0x07d7,0x0009,0x0014,0x002e,0x0064,0x0065,0x00d7,0x00d8,0x01c8,0x01c9,0x01ca,
    0x01cb,0x03cb,0x07d8,0x07d9,0x0015,0x002f,0x0066,0x00d9,0x00da,0x01cc,0x01cd,0x01ce,
    0x01cf,0x03cc,0x03cd,0x03ce,0x07da,0x0fe4,0x0030,0x0067,0x00db,0x01d0,0x01d1,0x01d2,
    0x01d3,0x03cf,0x03d0,0x03d1,0x03d2,0x07db,0x0fe5,0x1fea,0x0068,0x0069,0x00dc,0x01d4,
    0x01d5,0x01d6,0x03d3,0x03d4,0x03d5,0x03d6,0x07dc,0x07dd,0x0fe6,0x1feb,0x00dd,0x00de,
    0x01d7,0x01d8,0x01d9,0x03d7,0x03d8,0x03d9,0x03da,0x07de,0x07df,0x0fe7,0x1fec,0x3ff2,
    0x00df,0x00e0,0x01da,0x01db,0x03db,0x03dc,0x07e0,0x07e1,0x07e2,0x0fe8,0x0fe9,0x1fed,
    0x1fee,0x7ff4,0x00e1,0x00e2,0x01dc,0x01dd,0x03dd,0x03de,0x07e3,0x07e4,0x07e5,0x0fea,
    0x0feb,0x1fef,0x3ff3,0x7ff5,0x01de,0x01df,0x01e0,0x03df,0x03e0,0x03e1,0x07e6,0x07e7,
    0x0fec,0x1ff0,0x0fed,0x3ff4,0x7ff6,0xfff8,0x01e1,0x01e2,0x03e2,0x03e3,0x03e4,0x03e5,
    0x07e8,0x0fee,0x0fef,0x3ff5,0x3ff6,0xfff9,0xfffa,0xfffa,0x01e3,0x01e4,0x03e6,0x03e7,
    0x07e9,0x07ea,0x0ff0,0x1ff1,0x1ff2,0x3ff7,0x3ff8,0x7ff7,0x7ff7,0xfffa,0x03e8,0x03e9,
    0x03ea,0x07eb,0x07ec,0x0ff1,0x0ff2,0x1ff3,0x7ff8,0x7ff9,0xfffb,0x3ff8,0x7ff7,0x7ff7,
    0x07ed,0x07ee,0x07ef,0x0ff3,0x1ff4,0x1ff5,0x1ff6,0x7ffa,0xfffc,0xfffd,0xfffb,0xfffb,
    0x3ff8,0x7ff7,0x07f0,0x07f1,0x0ff4,0x1ff7,0x1ff8,0x3ff9,0x7ffb,0xfffe,0xffff,
};


static const uint8_t cvh_huffbits1[97] = {
    1, 4, 5, 6, 7, 8, 8, 9, 10, 10, 4, 5,
    6, 7, 7, 8, 8, 9, 9, 11, 5, 5, 6, 7,
    8, 8, 9, 9, 10, 11, 6, 6, 7, 8, 8, 9,
    9, 10, 11, 12, 7, 7, 8, 8, 9, 9, 10, 11,
    11, 13, 8, 8, 8, 9, 9, 10, 10, 11, 12, 14,
    8, 8, 8, 9, 10, 11, 11, 12, 13, 15, 9, 9,
    9, 10, 11, 12, 12, 14, 14, 0, 9, 9, 9, 10,
    11, 12, 14, 16, 0, 0, 10, 10, 11, 12, 13, 14,
    16,
};


static const uint16_t cvh_huffcodes1[97] = {
    0x0000,0x0008,0x0014,0x0030,0x006a,0x00e2,0x00e3,0x01e4,0x03ec,0x03ed,0x0009,0x0015,
    0x0031,0x006b,0x006c,0x00e4,0x00e5,0x01e5,0x01e6,0x07f0,0x0016,0x0017,0x0032,0x006d,
    0x00e6,0x00e7,0x01e7,0x01e8,0x03ee,0x07f1,0x0033,0x0034,0x006e,0x00e8,0x00e9,0x01e9,
    0x01ea,0x03ef,0x07f2,0x0ff6,0x006f,0x0070,0x00ea,0x00eb,0x01eb,0x01ec,0x03f0,0x07f3,
    0x07f4,0x1ffa,0x00ec,0x00ed,0x00ee,0x01ed,0x01ee,0x03f1,0x03f2,0x07f5,0x0ff7,0x3ffa,
    0x00ef,0x00f0,0x00f1,0x01ef,0x03f3,0x07f6,0x07f7,0x0ff8,0x1ffb,0x7ffe,0x01f0,0x01f1,
    0x01f2,0x03f4,0x07f8,0x0ff9,0x0ffa,0x3ffb,0x3ffc,0x0000,0x01f3,0x01f4,0x01f5,0x03f5,
    0x07f9,0x0ffb,0x3ffd,0xfffe,0x0000,0x0000,0x03f6,0x03f7,0x07fa,0x0ffc,0x1ffc,0x3ffe,
    0xffff,
};

static const uint8_t cvh_huffbits2[48] = {
    1, 4, 5, 7, 8, 9, 10, 3, 4, 5, 7, 8,
    9, 10, 5, 5, 6, 7, 8, 10, 10, 7, 6, 7,
    8, 9, 10, 12, 8, 8, 8, 9, 10, 12, 14, 8,
    9, 9, 10, 11, 15, 16, 9, 10, 11, 12, 13, 16,
};

static const uint16_t cvh_huffcodes2[48] = {
    0x0000,0x000a,0x0018,0x0074,0x00f2,0x01f4,0x03f6,0x0004,0x000b,0x0019,0x0075,0x00f3,
    0x01f5,0x03f7,0x001a,0x001b,0x0038,0x0076,0x00f4,0x03f8,0x03f9,0x0077,0x0039,0x0078,
    0x00f5,0x01f6,0x03fa,0x0ffc,0x00f6,0x00f7,0x00f8,0x01f7,0x03fb,0x0ffd,0x3ffe,0x00f9,
    0x01f8,0x01f9,0x03fc,0x07fc,0x7ffe,0xfffe,0x01fa,0x03fd,0x07fd,0x0ffe,0x1ffe,0xffff,
};

static const uint8_t cvh_huffbits3[607] = {
    2, 4, 6, 8, 10, 5, 5, 6, 8, 10, 7, 8,
    8, 10, 12, 9, 9, 10, 12, 15, 10, 11, 13, 16,
    16, 5, 6, 8, 10, 11, 5, 6, 8, 10, 12, 7,
    7, 8, 10, 13, 9, 9, 10, 12, 15, 12, 11, 13,
    16, 16, 7, 9, 10, 12, 15, 7, 8, 10, 12, 13,
    9, 9, 11, 13, 16, 11, 11, 12, 14, 16, 12, 12,
    14, 16, 0, 9, 11, 12, 16, 16, 9, 10, 13, 15,
    16, 10, 11, 12, 16, 16, 13, 13, 16, 16, 16, 16,
    16, 15, 16, 0, 11, 13, 16, 16, 15, 11, 13, 15,
    16, 16, 13, 13, 16, 16, 0, 14, 16, 16, 16, 0,
    16, 16, 0, 0, 0, 4, 6, 8, 10, 13, 6, 6,
    8, 10, 13, 9, 8, 10, 12, 16, 10, 10, 11, 15,
    16, 13, 12, 14, 16, 16, 5, 6, 8, 11, 13, 6,
    6, 8, 10, 13, 8, 8, 9, 11, 14, 10, 10, 12,
    12, 16, 13, 12, 13, 15, 16, 7, 8, 9, 12, 16,
    7, 8, 10, 12, 14, 9, 9, 10, 13, 16, 11, 10,
    12, 15, 16, 13, 13, 16, 16, 0, 9, 11, 13, 16,
    16, 9, 10, 12, 15, 16, 10, 11, 13, 16, 16, 13,
    12, 16, 16, 16, 16, 16, 16, 16, 0, 11, 13, 16,
    16, 16, 11, 13, 16, 16, 16, 12, 13, 15, 16, 0,
    16, 16, 16, 16, 0, 16, 16, 0, 0, 0, 6, 8,
    11, 13, 16, 8, 8, 10, 12, 16, 11, 10, 11, 13,
    16, 12, 13, 13, 15, 16, 16, 16, 14, 16, 0, 6,
    8, 10, 13, 16, 8, 8, 10, 12, 16, 10, 10, 11,
    13, 16, 13, 12, 13, 16, 16, 14, 14, 14, 16, 0,
    8, 9, 11, 13, 16, 8, 9, 11, 16, 14, 10, 10,
    12, 15, 16, 12, 12, 13, 16, 16, 15, 16, 16, 16,
    0, 10, 12, 15, 16, 16, 10, 12, 12, 14, 16, 12,
    12, 13, 16, 16, 14, 15, 16, 16, 0, 16, 16, 16,
    0, 0, 12, 15, 15, 16, 0, 13, 13, 16, 16, 0,
    14, 16, 16, 16, 0, 16, 16, 16, 0, 0, 0, 0,
    0, 0, 0, 8, 10, 13, 15, 16, 10, 11, 13, 16,
    16, 13, 13, 14, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 0, 8, 10, 11, 15, 16, 9, 10, 12,
    16, 16, 12, 12, 15, 16, 16, 16, 14, 16, 16, 16,
    16, 16, 16, 16, 0, 9, 11, 14, 16, 16, 10, 11,
    13, 16, 16, 14, 13, 14, 16, 16, 16, 15, 15, 16,
    0, 16, 16, 16, 0, 0, 11, 13, 16, 16, 16, 11,
    13, 15, 16, 16, 13, 16, 16, 16, 0, 16, 16, 16,
    16, 0, 16, 16, 0, 0, 0, 15, 16, 16, 16, 0,
    14, 16, 16, 16, 0, 16, 16, 16, 0, 0, 16, 16,
    0, 0, 0, 0, 0, 0, 0, 0, 9, 13, 16, 16,
    16, 11, 13, 16, 16, 16, 14, 15, 16, 16, 0, 15,
    16, 16, 16, 0, 16, 16, 0, 0, 0, 9, 13, 15,
    15, 16, 12, 13, 14, 16, 16, 16, 15, 16, 16, 0,
    16, 16, 16, 16, 0, 16, 16, 0, 0, 0, 11, 13,
    15, 16, 0, 12, 14, 16, 16, 0, 16, 16, 16, 16,
    0, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 16,
    16, 16, 16, 0, 16, 16, 16, 16, 0, 16, 16, 16,
    0, 0, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0,
    16, 16, 0, 0, 0, 16, 16,
};


static const uint16_t cvh_huffcodes3[607] = {
    0x0000,0x0004,0x0022,0x00c6,0x03b0,0x000c,0x000d,0x0023,0x00c7,0x03b1,0x005c,0x00c8,
    0x00c9,0x03b2,0x0fa4,0x01c2,0x01c3,0x03b3,0x0fa5,0x7f72,0x03b4,0x07b2,0x1f9a,0xff24,
    0xff25,0x000e,0x0024,0x00ca,0x03b5,0x07b3,0x000f,0x0025,0x00cb,0x03b6,0x0fa6,0x005d,
    0x005e,0x00cc,0x03b7,0x1f9b,0x01c4,0x01c5,0x03b8,0x0fa7,0x7f73,0x0fa8,0x07b4,0x1f9c,
    0xff26,0xff27,0x005f,0x01c6,0x03b9,0x0fa9,0x7f74,0x0060,0x00cd,0x03ba,0x0faa,0x1f9d,
    0x01c7,0x01c8,0x07b5,0x1f9e,0xff28,0x07b6,0x07b7,0x0fab,0x3fa2,0xff29,0x0fac,0x0fad,
    0x3fa3,0xff2a,0x3fa2,0x01c9,0x07b8,0x0fae,0xff2b,0xff2c,0x01ca,0x03bb,0x1f9f,0x7f75,
    0xff2d,0x03bc,0x07b9,0x0faf,0xff2e,0xff2f,0x1fa0,0x1fa1,0xff30,0xff31,0xff32,0xff33,
    0xff34,0x7f76,0xff35,0xff31,0x07ba,0x1fa2,0xff36,0xff37,0x7f77,0x07bb,0x1fa3,0x7f78,
    0xff38,0xff39,0x1fa4,0x1fa5,0xff3a,0xff3b,0xff2e,0x3fa4,0xff3c,0xff3d,0xff3e,0xff31,
    0xff3f,0xff40,0xff30,0xff31,0xff31,0x0005,0x0026,0x00ce,0x03bd,0x1fa6,0x0027,0x0028,
    0x00cf,0x03be,0x1fa7,0x01cb,0x00d0,0x03bf,0x0fb0,0xff41,0x03c0,0x03c1,0x07bc,0x7f79,
    0xff42,0x1fa8,0x0fb1,0x3fa5,0xff43,0xff44,0x0010,0x0029,0x00d1,0x07bd,0x1fa9,0x002a,
    0x002b,0x00d2,0x03c2,0x1faa,0x00d3,0x00d4,0x01cc,0x07be,0x3fa6,0x03c3,0x03c4,0x0fb2,
    0x0fb3,0xff45,0x1fab,0x0fb4,0x1fac,0x7f7a,0xff46,0x0061,0x00d5,0x01cd,0x0fb5,0xff47,
    0x0062,0x00d6,0x03c5,0x0fb6,0x3fa7,0x01ce,0x01cf,0x03c6,0x1fad,0xff48,0x07bf,0x03c7,
    0x0fb7,0x7f7b,0xff49,0x1fae,0x1faf,0xff4a,0xff4b,0x7f7b,0x01d0,0x07c0,0x1fb0,0xff4c,
    0xff4d,0x01d1,0x03c8,0x0fb8,0x7f7c,0xff4e,0x03c9,0x07c1,0x1fb1,0xff4f,0xff50,0x1fb2,
    0x0fb9,0xff51,0xff52,0xff53,0xff54,0xff55,0xff56,0xff57,0xff52,0x07c2,0x1fb3,0xff58,
    0xff59,0xff5a,0x07c3,0x1fb4,0xff5b,0xff5c,0xff5d,0x0fba,0x1fb5,0x7f7d,0xff5e,0xff4f,
    0xff5f,0xff60,0xff61,0xff62,0xff52,0xff63,0xff64,0xff51,0xff52,0xff52,0x002c,0x00d7,
    0x07c4,0x1fb6,0xff65,0x00d8,0x00d9,0x03ca,0x0fbb,0xff66,0x07c5,0x03cb,0x07c6,0x1fb7,
    0xff67,0x0fbc,0x1fb8,0x1fb9,0x7f7e,0xff68,0xff69,0xff6a,0x3fa8,0xff6b,0x7f7e,0x002d,
    0x00da,0x03cc,0x1fba,0xff6c,0x00db,0x00dc,0x03cd,0x0fbd,0xff6d,0x03ce,0x03cf,0x07c7,
    0x1fbb,0xff6e,0x1fbc,0x0fbe,0x1fbd,0xff6f,0xff70,0x3fa9,0x3faa,0x3fab,0xff71,0xff6f,
    0x00dd,0x01d2,0x07c8,0x1fbe,0xff72,0x00de,0x01d3,0x07c9,0xff73,0x3fac,0x03d0,0x03d1,
    0x0fbf,0x7f7f,0xff74,0x0fc0,0x0fc1,0x1fbf,0xff75,0xff76,0x7f80,0xff77,0xff78,0xff79,
    0xff75,0x03d2,0x0fc2,0x7f81,0xff7a,0xff7b,0x03d3,0x0fc3,0x0fc4,0x3fad,0xff7c,0x0fc5,
    0x0fc6,0x1fc0,0xff7d,0xff7e,0x3fae,0x7f82,0xff7f,0xff80,0xff80,0xff81,0xff82,0xff83,
    0xff80,0xff80,0x0fc7,0x7f83,0x7f84,0xff84,0xff7a,0x1fc1,0x1fc2,0xff85,0xff86,0x3fad,
    0x3faf,0xff87,0xff88,0xff89,0xff7d,0xff8a,0xff8b,0xff8c,0xff80,0xff80,0x3fae,0x7f82,
    0xff7f,0xff80,0xff80,0x00df,0x03d4,0x1fc3,0x7f85,0xff8d,0x03d5,0x07ca,0x1fc4,0xff8e,
    0xff8f,0x1fc5,0x1fc6,0x3fb0,0xff90,0xff91,0xff92,0xff93,0xff94,0xff95,0xff96,0xff97,
    0xff98,0xff99,0xff9a,0xff95,0x00e0,0x03d6,0x07cb,0x7f86,0xff9b,0x01d4,0x03d7,0x0fc8,
    0xff9c,0xff9d,0x0fc9,0x0fca,0x7f87,0xff9e,0xff9f,0xffa0,0x3fb1,0xffa1,0xffa2,0xffa3,
    0xffa4,0xffa5,0xffa6,0xffa7,0xffa2,0x01d5,0x07cc,0x3fb2,0xffa8,0xffa9,0x03d8,0x07cd,
    0x1fc7,0xffaa,0xffab,0x3fb3,0x1fc8,0x3fb4,0xffac,0xffad,0xffae,0x7f88,0x7f89,0xffaf,
    0xffaf,0xffb0,0xffb1,0xffb2,0xffaf,0xffaf,0x07ce,0x1fc9,0xffb3,0xffb4,0xffb5,0x07cf,
    0x1fca,0x7f8a,0xffb6,0xffb7,0x1fcb,0xffb8,0xffb9,0xffba,0xffba,0xffbb,0xffbc,0xffbd,
    0xffbe,0xffbe,0xffbf,0xffc0,0xffbd,0xffbe,0xffbe,0x7f8b,0xffc1,0xffc2,0xffc3,0xffb4,
    0x3fb5,0xffc4,0xffc5,0xffc6,0xffb6,0xffc7,0xffc8,0xffc9,0xffba,0xffba,0xffca,0xffcb,
    0xffbd,0xffbe,0xffbe,0xffbb,0xffbc,0xffbd,0xffbe,0xffbe,0x01d6,0x1fcc,0xffcc,0xffcd,
    0xffce,0x07d0,0x1fcd,0xffcf,0xffd0,0xffd1,0x3fb6,0x7f8c,0xffd2,0xffd3,0xff90,0x7f8d,
    0xffd4,0xffd5,0xffd6,0xff95,0xffd7,0xffd8,0xff94,0xff95,0xff95,0x01d7,0x1fce,0x7f8e,
    0x7f8f,0xffd9,0x0fcb,0x1fcf,0x3fb7,0xffda,0xffdb,0xffdc,0x7f90,0xffdd,0xffde,0xff9e,
    0xffdf,0xffe0,0xffe1,0xffe2,0xffa2,0xffe3,0xffe4,0xffa1,0xffa2,0xffa2,0x07d1,0x1fd0,
    0x7f91,0xffe5,0xffa8,0x0fcc,0x3fb8,0xffe6,0xffe7,0xffaa,0xffe8,0xffe9,0xffea,0xffeb,
    0xffac,0xffec,0xffed,0xffee,0xffaf,0xffaf,0xffae,0x7f88,0x7f89,0xffaf,0xffaf,0xffef,
    0xfff0,0xfff1,0xfff2,0xffb4,0xfff3,0xfff4,0xfff5,0xfff6,0xffb6,0xfff7,0xfff8,0xfff9,
    0xffba,0xffba,0xfffa,0xfffb,0xffbd,0xffbe,0xffbe,0xffbb,0xffbc,0xffbd,0xffbe,0xffbe,
    0xfffc,0xfffd,0xffb3,0xffb4,0xffb4,0xfffe,0xffff,
};

static const uint8_t cvh_huffbits4[246] = {
    2, 4, 7, 10, 4, 5, 7, 10, 7, 8, 10, 14,
    11, 11, 15, 15, 4, 5, 9, 12, 5, 5, 8, 12,
    8, 7, 10, 15, 11, 11, 15, 15, 7, 9, 12, 15,
    8, 8, 12, 15, 10, 10, 13, 15, 14, 14, 15, 0,
    11, 13, 15, 15, 11, 13, 15, 15, 14, 15, 15, 0,
    15, 15, 0, 0, 4, 5, 9, 13, 5, 6, 9, 13,
    9, 9, 11, 15, 14, 13, 15, 15, 4, 6, 9, 12,
    5, 6, 9, 13, 9, 8, 11, 15, 13, 12, 15, 15,
    7, 9, 12, 15, 7, 8, 11, 15, 10, 10, 14, 15,
    14, 15, 15, 0, 10, 12, 15, 15, 11, 13, 15, 15,
    15, 15, 15, 0, 15, 15, 0, 0, 6, 9, 13, 14,
    8, 9, 12, 15, 12, 12, 15, 15, 15, 15, 15, 0,
    7, 9, 13, 15, 8, 9, 12, 15, 11, 12, 15, 15,
    15, 15, 15, 0, 9, 11, 15, 15, 9, 11, 15, 15,
    14, 14, 15, 0, 15, 15, 0, 0, 14, 15, 15, 0,
    14, 15, 15, 0, 15, 15, 0, 0, 0, 0, 0, 0,
    9, 12, 15, 15, 12, 13, 15, 15, 15, 15, 15, 0,
    15, 15, 0, 0, 10, 12, 15, 15, 12, 14, 15, 15,
    15, 15, 15, 0, 15, 15, 0, 0, 14, 15, 15, 0,
    15, 15, 15, 0, 15, 15, 0, 0, 0, 0, 0, 0,
    15, 15, 0, 0, 15, 15,
};


static const uint16_t cvh_huffcodes4[246] = {
    0x0000,0x0004,0x006c,0x03e6,0x0005,0x0012,0x006d,0x03e7,0x006e,0x00e8,0x03e8,0x3fc4,
    0x07e0,0x07e1,0x7fa4,0x7fa5,0x0006,0x0013,0x01e2,0x0fda,0x0014,0x0015,0x00e9,0x0fdb,
    0x00ea,0x006f,0x03e9,0x7fa6,0x07e2,0x07e3,0x7fa7,0x7fa8,0x0070,0x01e3,0x0fdc,0x7fa9,
    0x00eb,0x00ec,0x0fdd,0x7faa,0x03ea,0x03eb,0x1fd6,0x7fab,0x3fc5,0x3fc6,0x7fac,0x1fd6,
    0x07e4,0x1fd7,0x7fad,0x7fae,0x07e5,0x1fd8,0x7faf,0x7fb0,0x3fc7,0x7fb1,0x7fb2,0x1fd6,
    0x7fb3,0x7fb4,0x1fd6,0x1fd6,0x0007,0x0016,0x01e4,0x1fd9,0x0017,0x0032,0x01e5,0x1fda,
    0x01e6,0x01e7,0x07e6,0x7fb5,0x3fc8,0x1fdb,0x7fb6,0x7fb7,0x0008,0x0033,0x01e8,0x0fde,
    0x0018,0x0034,0x01e9,0x1fdc,0x01ea,0x00ed,0x07e7,0x7fb8,0x1fdd,0x0fdf,0x7fb9,0x7fba,
    0x0071,0x01eb,0x0fe0,0x7fbb,0x0072,0x00ee,0x07e8,0x7fbc,0x03ec,0x03ed,0x3fc9,0x7fbd,
    0x3fca,0x7fbe,0x7fbf,0x3fc9,0x03ee,0x0fe1,0x7fc0,0x7fc1,0x07e9,0x1fde,0x7fc2,0x7fc3,
    0x7fc4,0x7fc5,0x7fc6,0x3fc9,0x7fc7,0x7fc8,0x3fc9,0x3fc9,0x0035,0x01ec,0x1fdf,0x3fcb,
    0x00ef,0x01ed,0x0fe2,0x7fc9,0x0fe3,0x0fe4,0x7fca,0x7fcb,0x7fcc,0x7fcd,0x7fce,0x7fca,
    0x0073,0x01ee,0x1fe0,0x7fcf,0x00f0,0x01ef,0x0fe5,0x7fd0,0x07ea,0x0fe6,0x7fd1,0x7fd2,
    0x7fd3,0x7fd4,0x7fd5,0x7fd1,0x01f0,0x07eb,0x7fd6,0x7fd7,0x01f1,0x07ec,0x7fd8,0x7fd9,
    0x3fcc,0x3fcd,0x7fda,0x7fda,0x7fdb,0x7fdc,0x7fda,0x7fda,0x3fce,0x7fdd,0x7fde,0x7fd6,
    0x3fcf,0x7fdf,0x7fe0,0x7fd8,0x7fe1,0x7fe2,0x7fda,0x7fda,0x3fcc,0x3fcd,0x7fda,0x7fda,
    0x01f2,0x0fe7,0x7fe3,0x7fe4,0x0fe8,0x1fe1,0x7fe5,0x7fe6,0x7fe7,0x7fe8,0x7fe9,0x7fca,
    0x7fea,0x7feb,0x7fca,0x7fca,0x03ef,0x0fe9,0x7fec,0x7fed,0x0fea,0x3fd0,0x7fee,0x7fef,
    0x7ff0,0x7ff1,0x7ff2,0x7fd1,0x7ff3,0x7ff4,0x7fd1,0x7fd1,0x3fd1,0x7ff5,0x7ff6,0x7fd6,
    0x7ff7,0x7ff8,0x7ff9,0x7fd8,0x7ffa,0x7ffb,0x7fda,0x7fda,0x3fcc,0x3fcd,0x7fda,0x7fda,
    0x7ffc,0x7ffd,0x7fd6,0x7fd6,0x7ffe,0x7fff,
};


static const uint8_t cvh_huffbits5[230] = {
    2, 4, 8, 4, 5, 9, 9, 10, 14, 4, 6, 11,
    5, 6, 12, 10, 11, 15, 9, 11, 15, 10, 13, 15,
    14, 15, 0, 4, 6, 12, 6, 7, 12, 12, 12, 15,
    5, 7, 13, 6, 7, 13, 12, 13, 15, 10, 12, 15,
    11, 13, 15, 15, 15, 0, 8, 13, 15, 11, 12, 15,
    15, 15, 0, 10, 13, 15, 12, 15, 15, 15, 15, 0,
    15, 15, 0, 15, 15, 0, 0, 0, 0, 4, 5, 11,
    5, 7, 12, 11, 12, 15, 6, 7, 13, 7, 8, 14,
    12, 14, 15, 11, 13, 15, 12, 13, 15, 15, 15, 0,
    5, 6, 13, 7, 8, 15, 12, 14, 15, 6, 8, 14,
    7, 8, 15, 14, 15, 15, 12, 12, 15, 12, 13, 15,
    15, 15, 0, 9, 13, 15, 12, 13, 15, 15, 15, 0,
    11, 13, 15, 13, 13, 15, 15, 15, 0, 14, 15, 0,
    15, 15, 0, 0, 0, 0, 8, 10, 15, 11, 12, 15,
    15, 15, 0, 10, 12, 15, 12, 13, 15, 15, 15, 0,
    14, 15, 0, 15, 15, 0, 0, 0, 0, 8, 12, 15,
    12, 13, 15, 15, 15, 0, 11, 13, 15, 13, 15, 15,
    15, 15, 0, 15, 15, 0, 15, 15, 0, 0, 0, 0,
    14, 15, 0, 15, 15, 0, 0, 0, 0, 15, 15, 0,
    15, 15,
};



static const uint16_t cvh_huffcodes5[230] = {
    0x0000,0x0004,0x00f0,0x0005,0x0012,0x01f0,0x01f1,0x03e8,0x3fce,0x0006,0x0030,0x07de,
    0x0013,0x0031,0x0fd2,0x03e9,0x07df,0x7fb0,0x01f2,0x07e0,0x7fb1,0x03ea,0x1fd2,0x7fb2,
    0x3fcf,0x7fb3,0x0031,0x0007,0x0032,0x0fd3,0x0033,0x0070,0x0fd4,0x0fd5,0x0fd6,0x7fb4,
    0x0014,0x0071,0x1fd3,0x0034,0x0072,0x1fd4,0x0fd7,0x1fd5,0x7fb5,0x03eb,0x0fd8,0x7fb6,
    0x07e1,0x1fd6,0x7fb7,0x7fb8,0x7fb9,0x0072,0x00f1,0x1fd7,0x7fba,0x07e2,0x0fd9,0x7fbb,
    0x7fbc,0x7fbd,0x0070,0x03ec,0x1fd8,0x7fbe,0x0fda,0x7fbf,0x7fc0,0x7fc1,0x7fc2,0x0072,
    0x7fc3,0x7fc4,0x0071,0x7fc5,0x7fc6,0x0072,0x0034,0x0072,0x0072,0x0008,0x0015,0x07e3,
    0x0016,0x0073,0x0fdb,0x07e4,0x0fdc,0x7fc7,0x0035,0x0074,0x1fd9,0x0075,0x00f2,0x3fd0,
    0x0fdd,0x3fd1,0x7fc8,0x07e5,0x1fda,0x7fc9,0x0fde,0x1fdb,0x7fca,0x7fcb,0x7fcc,0x00f2,
    0x0017,0x0036,0x1fdc,0x0076,0x00f3,0x7fcd,0x0fdf,0x3fd2,0x7fce,0x0037,0x00f4,0x3fd3,
    0x0077,0x00f5,0x7fcf,0x3fd4,0x7fd0,0x7fd1,0x0fe0,0x0fe1,0x7fd2,0x0fe2,0x1fdd,0x7fd3,
    0x7fd4,0x7fd5,0x00f5,0x01f3,0x1fde,0x7fd6,0x0fe3,0x1fdf,0x7fd7,0x7fd8,0x7fd9,0x00f3,
    0x07e6,0x1fe0,0x7fda,0x1fe1,0x1fe2,0x7fdb,0x7fdc,0x7fdd,0x00f5,0x3fd5,0x7fde,0x00f4,
    0x7fdf,0x7fe0,0x00f5,0x0077,0x00f5,0x00f5,0x00f6,0x03ed,0x7fe1,0x07e7,0x0fe4,0x7fe2,
    0x7fe3,0x7fe4,0x0073,0x03ee,0x0fe5,0x7fe5,0x0fe6,0x1fe3,0x7fe6,0x7fe7,0x7fe8,0x00f2,
    0x3fd6,0x7fe9,0x0074,0x7fea,0x7feb,0x00f2,0x0075,0x00f2,0x00f2,0x00f7,0x0fe7,0x7fec,
    0x0fe8,0x1fe4,0x7fed,0x7fee,0x7fef,0x00f3,0x07e8,0x1fe5,0x7ff0,0x1fe6,0x7ff1,0x7ff2,
    0x7ff3,0x7ff4,0x00f5,0x7ff5,0x7ff6,0x00f4,0x7ff7,0x7ff8,0x00f5,0x0077,0x00f5,0x00f5,
    0x3fd7,0x7ff9,0x0036,0x7ffa,0x7ffb,0x00f3,0x0076,0x00f3,0x00f3,0x7ffc,0x7ffd,0x0000,
    0x7ffe,0x7fff,
};


static const uint8_t cvh_huffbits6[32] = {
     1,  4,  4,  6,  4,  6,  6,  8,  4,  6,  6,  8,
     6,  9,  8, 10,  4,  6,  7,  8,  6,  9,  8, 11,
     6,  9,  8, 10,  8, 10,  9,  11,
};

static const uint16_t cvh_huffcodes6[32] = {
    0x0000,0x0008,0x0009,0x0034,0x000a,0x0035,0x0036,0x00f6,0x000b,0x0037,0x0038,0x00f7,
    0x0039,0x01fa,0x00f8,0x03fc,0x000c,0x003a,0x007a,0x00f9,0x003b,0x01fb,0x00fa,0x07fe,
    0x003c,0x01fc,0x00fb,0x03fd,0x00fc,0x03fe,0x01fd,0x07ff,
};

static const uint16_t* cvh_huffcodes[7] = {
    cvh_huffcodes0, cvh_huffcodes1, cvh_huffcodes2, cvh_huffcodes3,
    cvh_huffcodes4, cvh_huffcodes5, cvh_huffcodes6,
};

static const uint8_t* cvh_huffbits[7] = {
    cvh_huffbits0, cvh_huffbits1, cvh_huffbits2, cvh_huffbits3,
    cvh_huffbits4, cvh_huffbits5, cvh_huffbits6,
};


static const uint16_t ccpl_huffcodes2[3] = {
    0x02,0x00,0x03,
};

static const uint16_t ccpl_huffcodes3[7] = {
    0x3e,0x1e,0x02,0x00,0x06,0x0e,0x3f,
};

static const uint16_t ccpl_huffcodes4[15] = {
    0xfc,0xfd,0x7c,0x3c,0x1c,0x0c,0x04,0x00,0x05,0x0d,0x1d,0x3d,
    0x7d,0xfe,0xff,
};

static const uint16_t ccpl_huffcodes5[31] = {
    0x03f8,0x03f9,0x03fa,0x03fb,0x01f8,0x01f9,0x00f8,0x00f9,0x0078,0x0079,0x0038,0x0039,
    0x0018,0x0019,0x0004,0x0000,0x0005,0x001a,0x001b,0x003a,0x003b,0x007a,0x007b,0x00fa,
    0x00fb,0x01fa,0x01fb,0x03fc,0x03fd,0x03fe,0x03ff,
};

static const uint16_t ccpl_huffcodes6[63] = {
    0x0004,0x0005,0x0005,0x0006,0x0006,0x0007,0x0007,0x0007,0x0007,0x0008,0x0008,0x0008,
    0x0008,0x0009,0x0009,0x0009,0x0009,0x000a,0x000a,0x000a,0x000a,0x000a,0x000b,0x000b,
    0x000b,0x000b,0x000c,0x000d,0x000e,0x000e,0x0010,0x0000,0x000a,0x0018,0x0019,0x0036,
    0x0037,0x0074,0x0075,0x0076,0x0077,0x00f4,0x00f5,0x00f6,0x00f7,0x01f5,0x01f6,0x01f7,
    0x01f8,0x03f6,0x03f7,0x03f8,0x03f9,0x03fa,0x07fa,0x07fb,0x07fc,0x07fd,0x0ffd,0x1ffd,
    0x3ffd,0x3ffe,0xffff,
};

static const uint8_t ccpl_huffbits2[3] = {
    2,1,2,
};

static const uint8_t ccpl_huffbits3[7] = {
    6,5,2,1,3,4,6,
};

static const uint8_t ccpl_huffbits4[15] = {
    8,8,7,6,5,4,3,1,3,4,5,6,7,8,8,
};

static const uint8_t ccpl_huffbits5[31] = {
    10,10,10,10,9,9,8,8,7,7,6,6,
    5,5,3,1,3,5,5,6,6,7,7,8,
    8,9,9,10,10,10,10,
};

static const uint8_t ccpl_huffbits6[63] = {
    16,15,14,13,12,11,11,11,11,10,10,10,
    10,9,9,9,9,9,8,8,8,8,7,7,
    7,7,6,6,5,5,3,1,4,5,5,6,
    6,7,7,7,7,8,8,8,8,9,9,9,
    9,10,10,10,10,10,11,11,11,11,12,13,
    14,14,16,
};

static const uint16_t* ccpl_huffcodes[5] = {
    ccpl_huffcodes2,ccpl_huffcodes3,
    ccpl_huffcodes4,ccpl_huffcodes5,ccpl_huffcodes6
};

static const uint8_t* ccpl_huffbits[5] = {
    ccpl_huffbits2,ccpl_huffbits3,
    ccpl_huffbits4,ccpl_huffbits5,ccpl_huffbits6
};


//Coupling tables

static const int cplband[51] = {
    0,1,2,3,4,5,6,7,8,9,
    10,11,11,12,12,13,13,14,14,14,
    15,15,15,15,16,16,16,16,16,17,
    17,17,17,17,17,18,18,18,18,18,
    18,18,19,19,19,19,19,19,19,19,
    19,
};

static const float cplscale2[3] = {
0.953020632266998,0.70710676908493,0.302905440330505,
};

static const float cplscale3[7] = {
0.981279790401459,0.936997592449188,0.875934481620789,0.70710676908493,
0.482430040836334,0.349335819482803,0.192587479948997,
};

static const float cplscale4[15] = {
0.991486728191376,0.973249018192291,0.953020632266998,0.930133521556854,
0.903453230857849,0.870746195316315,0.826180458068848,0.70710676908493,
0.563405573368073,0.491732746362686,0.428686618804932,0.367221474647522,
0.302905440330505,0.229752898216248,0.130207896232605,
};

static const float cplscale5[31] = {
0.995926380157471,0.987517595291138,0.978726446628571,0.969505727291107,
0.95979779958725,0.949531257152557,0.938616216182709,0.926936149597168,
0.914336204528809,0.900602877140045,0.885426938533783,0.868331849575043,
0.84851086139679,0.824381768703461,0.791833400726318,0.70710676908493,
0.610737144947052,0.566034197807312,0.529177963733673,0.495983630418777,
0.464778542518616,0.434642940759659,0.404955863952637,0.375219136476517,
0.344963222742081,0.313672333955765,0.280692428350449,0.245068684220314,
0.205169528722763,0.157508864998817,0.0901700109243393,
};

static const float cplscale6[63] = {
0.998005926609039,0.993956744670868,0.989822506904602,0.985598564147949,
0.981279790401459,0.976860702037811,0.972335040569305,0.967696130275726,
0.962936460971832,0.958047747612000,0.953020632266998,0.947844684123993,
0.942508161067963,0.936997592449188,0.931297719478607,0.925390899181366,
0.919256627559662,0.912870943546295,0.906205296516418,0.899225592613220,
0.891890347003937,0.884148240089417,0.875934481620789,0.867165684700012,
0.857730865478516,0.847477376461029,0.836184680461884,0.823513329029083,
0.808890223503113,0.791194140911102,0.767520070075989,0.707106769084930,
0.641024887561798,0.611565053462982,0.587959706783295,0.567296981811523,
0.548448026180267,0.530831515789032,0.514098942279816,0.498019754886627,
0.482430040836334,0.467206478118896,0.452251672744751,0.437485188245773,
0.422837972640991,0.408248275518417,0.393658757209778,0.379014074802399,
0.364258885383606,0.349335819482803,0.334183186292648,0.318732559680939,
0.302905440330505,0.286608695983887,0.269728302955627,0.252119421958923,
0.233590632677078,0.213876649737358,0.192587479948997,0.169101938605309,
0.142307326197624,0.109772264957428,0.0631198287010193,
};

static const float* cplscales[5] = {
    cplscale2, cplscale3, cplscale4, cplscale5, cplscale6,
};
