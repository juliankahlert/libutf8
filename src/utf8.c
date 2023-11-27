
// SPDX-License-Identifier: MIT
/*
 * Simple UTF8 Library for C purists.
 *
 * MIT License
 *
 * Copyright (c) 2023 Julian Kahlert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <string.h>

#include "utf8.h"

//#include <stdlib.h>

#define INLINE __attribute__((always_inline)) inline

/* Taken from Google's Chromium project */
#define COUNT_OF(x) \
	((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

// clang-format off
#define ACCEPT 0
#define REJECT 12

#define BLOCK       0
#define ALTERNATING 1

#define ASCII       0x041
#define LATIN_BLK1  0x0C0
#define LATIN_BLK2  0x0D8
#define LATIN_BLK3  0x100
#define LATIN_BLK4  0x139
#define LATIN_BLK5  0x14A
#define LATIN_BLK6  0x179
#define LATIN_BLK7  0x182
#define LATIN_BLK8  0x187
#define LATIN_BLK9  0x18B
#define LATIN_BLK10 0x191
#define LATIN_BLK11 0x198
#define LATIN_BLK12 0x19D
// clang-format on

enum char_type {
	UTF8_1_CHAR = 1,
	UTF8_2_CHAR = 2,
	UTF8_3_CHAR = 3,
	UTF8_4_CHAR = 4,
};

struct case_shift_data {
	const uint16_t type : 1;
	const uint16_t len : 6;
	const int16_t dshift : 9;
};

struct case_conversion_block {
	const uint32_t start;
	const struct case_shift_data shift;
};

/*
 * Important: Blocks need to be ordered by '.start' in acending order
 *
 * @TODO Write a generator for this, case conversion in utf8 sucks
 */
static const struct case_conversion_block g_ccb[] = {
    {
        .start = ASCII,
        .shift =
            {
                .type = BLOCK,
                .len = 26,
                .dshift = +6,
            },
    },
    {
        .start = LATIN_BLK1,
        .shift =
            {
                .type = BLOCK,
                .len = 23,
                .dshift = +9,
            },
    },
    {
        .start = LATIN_BLK2,
        .shift =
            {
                .type = BLOCK,
                .len = 7,
                .dshift = +25,
            },
    },
    {
        .start = LATIN_BLK3,
        .shift =
            {
                .type = ALTERNATING,
                .len = 55,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK4,
        .shift =
            {
                .type = ALTERNATING,
                .len = 15,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK5,
        .shift =
            {
                .type = ALTERNATING,
                .len = 45,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK6,
        .shift =
            {
                .type = ALTERNATING,
                .len = 6,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK7,
        .shift =
            {
                .type = ALTERNATING,
                .len = 4,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK8,
        .shift =
            {
                .type = ALTERNATING,
                .len = 2,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK9,
        .shift =
            {
                .type = ALTERNATING,
                .len = 2,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK10,
        .shift =
            {
                .type = ALTERNATING,
                .len = 2,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK11,
        .shift =
            {
                .type = ALTERNATING,
                .len = 2,
                .dshift = +1,
            },
    },
    {
        .start = LATIN_BLK12,
        .shift =
            {
                .type = BLOCK,
                .len = 1,
                .dshift = +213,
            },
    },
};

/*
 * Original Idea of this state machine:
 * Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 */
// clang-format off
static const uint8_t g_utf8d[] = {
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	 8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	 0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
	12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
	12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
	12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
	12,36,12,12,12,12,12,12,12,12,12,12,
};
// clang-format on

INLINE static uint32_t decode(uint32_t *const state, uint32_t *const codep,
			      const uint8_t byte)
{
	uint8_t type;

	type = g_utf8d[byte];

	*codep = (*state != ACCEPT) ? (byte & 0x3fu) | (*codep << 6) :
				      (0xff >> type) & (byte);

	*state = g_utf8d[256 + *state + type];

	return *state;
}

INLINE static enum char_type encoded_type(const uint32_t codepoint)
{
	if (codepoint < 0x80)
		return UTF8_1_CHAR;
	else if (codepoint < 0x800)
		return UTF8_2_CHAR;
	else if (codepoint < 0x10000)
		return UTF8_3_CHAR;

	return UTF8_4_CHAR;
}

INLINE static size_t encode1(const uint32_t codepoint, void *const buf)
{
	uint8_t *const s = buf;

	s[0] = (uint8_t)codepoint;

	return 1;
}

INLINE static size_t encode2(const uint32_t codepoint, void *const buf)
{
	uint8_t *const s = buf;

	s[0] = (uint8_t)((codepoint >> 6) | 0xC0);
	s[1] = (uint8_t)((codepoint & 0x3F) | 0x80);

	return 2;
}

INLINE static size_t encode3(const uint32_t codepoint, void *const buf)
{
	uint8_t *const s = buf;

	s[0] = (uint8_t)((codepoint >> 12) | 0xE0);
	s[1] = (uint8_t)(((codepoint >> 6) & 0x3F) | 0x80);
	s[2] = (uint8_t)((codepoint & 0x3F) | 0x80);

	return 3;
}

INLINE static size_t encode4(const uint32_t codepoint, void *const buf)
{
	uint8_t *const s = buf;

	s[0] = (uint8_t)((codepoint >> 18) | 0xF0);
	s[1] = (uint8_t)(((codepoint >> 12) & 0x3F) | 0x80);
	s[2] = (uint8_t)(((codepoint >> 6) & 0x3F) | 0x80);
	s[3] = (uint8_t)((codepoint & 0x3F) | 0x80);

	return 4;
}

INLINE static size_t encode(const uint32_t codepoint, void *const buf)
{
	const enum char_type type = encoded_type(codepoint);

	switch (type) {
	case UTF8_1_CHAR:
		return encode1(codepoint, buf);
	case UTF8_2_CHAR:
		return encode2(codepoint, buf);
	case UTF8_3_CHAR:
		return encode3(codepoint, buf);
	case UTF8_4_CHAR:
		return encode4(codepoint, buf);
	}

	return 0;
}

INLINE static uint32_t downcase_block(const uint32_t codepoint,
				      const struct case_shift_data shift)
{
	return codepoint + shift.len + shift.dshift;
}

INLINE static uint32_t downcase_alt(const uint32_t codepoint,
				    const struct case_shift_data shift)
{
	return codepoint + shift.dshift;
}

INLINE static uint32_t downcase(const uint32_t codepoint)
{
	uint32_t end;
	size_t i;

	for (i = 0; i < COUNT_OF(g_ccb); i++) {
		if (codepoint < g_ccb[i].start)
			break;

		end = g_ccb[i].start + g_ccb[i].shift.len;

		if (codepoint >= end)
			continue;

		if (g_ccb[i].shift.type == BLOCK)
			return downcase_block(codepoint, g_ccb[i].shift);

		if ((g_ccb[i].start % 2) == (codepoint % 2))
			return downcase_alt(codepoint, g_ccb[i].shift);
	}

	return codepoint;
}

INLINE int utf8_decode(const void *buf,
		       const struct utf8_codepoint_listener *const cpl)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;

	if (!buf)
		return -EINVAL;

	s = buf;
	state = ACCEPT;

	while (*s) {
		decode(&state, &codepoint, *s++);
		if (state == ACCEPT)
			cpl->callback(cpl, codepoint);
	}

	return 0;
}

INLINE int utf8_check(const void *buf)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;

	if (!buf)
		return -EINVAL;

	s = buf;
	state = ACCEPT;

	while (*s)
		decode(&state, &codepoint, *s++);

	return !(state == ACCEPT);
}

INLINE int utf8_ncheck(const void *buf, const size_t len)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;
	size_t i;

	if (!buf || !len)
		return -EINVAL;

	s = buf;
	state = ACCEPT;

	for (i = 0; i < len && s[i]; i++)
		decode(&state, &codepoint, s[i]);

	return !(state == ACCEPT);
}

INLINE ssize_t utf8_count(const void *buf)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;
	ssize_t cnt;

	if (!buf)
		return -EINVAL;

	s = buf;
	cnt = 0;
	state = ACCEPT;

	while (*s) {
		decode(&state, &codepoint, *s++);
		cnt += (state == ACCEPT);
	}

	return cnt;
}

INLINE ssize_t utf8_ncount(const void *buf, const size_t len)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;
	ssize_t cnt;
	size_t i;

	if (!buf || !len)
		return -EINVAL;

	s = buf;
	cnt = 0;
	state = ACCEPT;

	for (i = 0; i < len && s[i]; i++) {
		decode(&state, &codepoint, s[i]);
		cnt += (state == ACCEPT);
	}

	return cnt;
}

INLINE ssize_t utf8_len(const void *buf)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;
	ssize_t bc;
	ssize_t i;

	if (!buf)
		return -EINVAL;

	bc = 0;
	s = buf;
	state = ACCEPT;

	for (i = 0; s[i]; i++) {
		decode(&state, &codepoint, s[i]);
		if (state == ACCEPT)
			bc = i;
	}

	return bc;
}

INLINE ssize_t utf8_nlen(const void *buf, const size_t len)
{
	uint32_t codepoint;
	const uint8_t *s;
	uint32_t state;
	ssize_t bc;
	ssize_t i;

	if (!buf)
		return -EINVAL;

	bc = 0;
	s = buf;
	state = ACCEPT;

	for (i = 0; i < len && s[i]; i++) {
		decode(&state, &codepoint, s[i]);
		if (state == ACCEPT)
			bc = i;
	}

	return bc;
}

INLINE int utf8_term(void *buf)
{
	ssize_t bc;
	uint8_t *s;

	bc = utf8_len(buf);
	if (bc < 0)
		return -EINVAL;

	bc++;
	s = buf;

	s[bc] = '\0';

	return 0;
}

INLINE int utf8_nterm(void *buf, const size_t len)
{
	uint8_t *s;
	ssize_t bc;

	bc = utf8_nlen(buf, len);
	if (bc < 0)
		return -EINVAL;

	bc++;

	if (bc >= len)
		return -ENOMEM;

	s = buf;
	s[bc] = '\0';

	return 0;
}

INLINE int utf8_stripinval(void *buf)
{
	uint32_t codepoint;
	uint32_t state;
	ssize_t cur;
	ssize_t dst;
	uint8_t *s;
	ssize_t i;
	size_t l;
	int move;

	if (!buf)
		return -EINVAL;

	s = buf;
	move = 0;
	state = ACCEPT;

	for (i = 0, l = 1, cur = 0, dst = 0; s[i]; i++, l++) {
		decode(&state, &codepoint, s[i]);
		if (state == ACCEPT) {
			if (move)
				memmove(&s[dst], &s[cur], l);

			dst += l;
			cur += l;
			l = 0;
			continue;
		}

		if (state == REJECT) {
			move = 1;
			cur += l;
			l = 0;
			state = ACCEPT;
		}
	}

	if (move)
		s[dst] = '\0';

	return 0;
}

INLINE int utf8_nstripinval(void *buf, const size_t len)
{
	uint32_t codepoint;
	uint32_t state;
	ssize_t cur;
	ssize_t dst;
	uint8_t *s;
	ssize_t i;
	size_t l;
	int move;

	if (!buf)
		return -EINVAL;

	s = buf;
	move = 0;
	state = ACCEPT;

	for (i = 0, l = 1, cur = 0, dst = 0; i < len && s[i]; i++, l++) {
		decode(&state, &codepoint, s[i]);
		if (state == ACCEPT) {
			if (move)
				memmove(&s[dst], &s[cur], l);

			dst += l;
			cur += l;
			l = 0;
			continue;
		}

		if (state == REJECT) {
			move = 1;
			cur += l;
			l = 0;
			state = ACCEPT;
		}
	}

	if (move)
		s[dst] = '\0';

	return 0;
}

INLINE int utf8_downcase(void *buf)
{
	uint32_t codepoint;
	uint32_t state;
	uint8_t *s;
	uint8_t *p;

	if (!buf)
		return -EINVAL;

	state = ACCEPT;
	s = buf;
	p = s;

	while (*s) {
		decode(&state, &codepoint, *s++);
		if (state == ACCEPT)
			p += encode(downcase(codepoint), p);
	}

	return 0;
}
