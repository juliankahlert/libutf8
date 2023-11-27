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

#ifndef _LIB_UTF8_
#define _LIB_UTF8_

#include <stdint.h>
#include <sys/types.h>

/*
 * Some concepts:
 *  - Callbacks/user-data:
 *    - call backs re wrapped in a struct and will receive a pointer to said
 *      struct as first param
 +    - retrieve user-data via 'container_of'
 *  - Return codes:
 *    - Functions returning 'int':
 *      - will return 0 on success
 *      - a positive UTF8 fault
 *      - a negative errno
 *    - Functions returning 'ssize_t':
 *      - will return 0 or a positive size on success
 *      - a negative errno
 *  - Arguments:
 *    - Functions taking a 'void *buf' may modify its content
 *    - Functions taking a 'const void *buf' will not modify its content
 *  - Naming:
 *    - Functions are downcase
 *    - Macros are upcase
 *    - Prefix is 'utf8_' (upcase if macro)
 *    - Functions prefixed with 'utf8_n' are a size checked variant of their
 *      non 'n' counterparts
 */

#define UTF8_FAULT_INVALID 1

struct utf8_codepoint_listener {
	void (*callback)(const struct utf8_codepoint_listener *const, uint32_t);
};

/* check if a buffer is a utf8 string */
int utf8_check(const void *buf);
int utf8_ncheck(const void *buf, const size_t len);

/* count a buffer lenght up to the first invalid utf8 character or NULL */
ssize_t utf8_len(const void *buf);
ssize_t utf8_nlen(const void *buf, const size_t len);

/* terminate a buffer at the first invalid utf8 character */
int utf8_term(void *buf);
int utf8_nterm(void *buf, const size_t len);

/* count utf8 charcters in a buffer */
ssize_t utf8_count(const void *buf);
ssize_t utf8_ncount(const void *buf, const size_t len);

/* strip invalid utf8 chars */
int utf8_stripinval(void *buf);
int utf8_nstripinval(void *buf, const size_t len);

/* decode utf8 to unicode codepoint */
int utf8_decode(const void *buf,
		const struct utf8_codepoint_listener *const cpl);

/* downcase/upcase, this lib is optimized for downcase */
int utf8_downcase(void *buf);

#endif
