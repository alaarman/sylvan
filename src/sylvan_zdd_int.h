/*
 * Copyright 2016 Tom van Dijk, Johannes Kepler University Linz
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Internals for multi-terminal ZDDs
 *
 * Bits
 * 127        1 complement      complement bit of the high edge
 * 126        1 custom leaf     this is a custom leaf node
 * 125        1 map             is a MAP node, for compose etc
 * 124        1 mark            used for node marking
 * 123..104  20 unused          unused space
 * 103..64   40 true index      index of the true edge
 *  63..40   24 variable        variable of this node
 *  39..0    40 false index     index of the false edge
 *
 * Lil endian: TTTT TTTT TT** **x* | FFFF FFFF FFVV VVVV
 * Big endian: x*** **TT TTTT TTTT | VVVV VVFF FFFF FFFF
 * Leaf nodes: x*** **** TTTT TTTT | VVVV VVVV VVVV VVVV (big endian)
 */


#ifndef SYLVAN_ZDD_INT_H
#define SYLVAN_ZDD_INT_H

/**
 * ZDD node structure
 */
typedef struct __attribute__((packed)) zddnode {
    uint64_t a, b;
} * zddnode_t; // 16 bytes

/**
 * Macros to work with the ZDD type and the complement marks
 */
#define ZDD_GETINDEX(zdd)           ((zdd & 0x000000ffffffffff))
#define ZDD_GETNODE(zdd)            ((zddnode_t)llmsset_index_to_ptr(nodes, ZDD_GETINDEX(zdd)))
#define ZDD_SETINDEX(zdd, idx)      ((zdd & 0xffffff0000000000) | (idx & 0x000000ffffffffff))
#define ZDD_HASMARK(s)              (s&zdd_complement?1:0)
#define ZDD_TOGGLEMARK(s)           (s^zdd_complement)
#define ZDD_STRIPMARK(s)            (s&~zdd_complement)
#define ZDD_TRANSFERMARK(from, to)  (to ^ (from & zdd_complement))
// Equal under mark
#define ZDD_EQUALM(a, b)            ((((a)^(b))&(~zdd_complement))==0)

static inline int __attribute__((unused))
zddnode_isleaf(zddnode_t n)
{
    return n->a & 0x4000000000000000 ? 1 : 0;
}

static inline uint32_t __attribute__((unused))
zddnode_gettype(zddnode_t n)
{
    return (uint32_t)(n->a);
}

static inline uint64_t __attribute__((unused))
zddnode_getvalue(zddnode_t n)
{
    return n->b;
}

static inline int __attribute__((unused))
zddnode_getcomp(zddnode_t n)
{
    return n->a & 0x8000000000000800 ? 1 : 0;
}

static inline uint64_t __attribute__((unused))
zddnode_getlow(zddnode_t n)
{
    return n->b & 0x000000ffffffffff;
}

static inline uint64_t __attribute__((unused))
zddnode_gethigh(zddnode_t n)
{
    return n->a & 0x800000ffffffffff;
}

static inline uint32_t __attribute__((unused))
zddnode_getvariable(zddnode_t n)
{
    return (uint32_t)(n->b >> 40);
}

static inline int __attribute__((unused))
zddnode_getmark(zddnode_t n)
{
    return n->b & 0x1000000000000000 ? 1 : 0;
}

static inline void __attribute__((unused))
zddnode_setmark(zddnode_t n, int mark)
{
    if (mark) n->b |= 0x1000000000000000;
    else n->b &= 0xefffffffffffffff;
}

static inline void __attribute__((unused))
zddnode_makeleaf(zddnode_t n, uint32_t type, uint64_t value)
{
    n->a = 0x4000000000000000 | (uint64_t)type;
    n->b = value;
}

static inline void __attribute__((unused))
zddnode_makenode(zddnode_t n, uint32_t var, uint64_t low, uint64_t high)
{
    n->a = high;
    n->b = ((uint64_t)var)<<40 | low;
}

static inline void __attribute__((unused))
zddnode_makemapnode(zddnode_t n, uint32_t var, uint64_t low, uint64_t high)
{
    n->a = high | 0x2000000000000000;
    n->b = ((uint64_t)var)<<40 | low;
}

static inline int __attribute__((unused))
zddnode_ismapnode(zddnode_t n)
{
    return n->a & 0x2000000000000000 ? 1 : 0;
}

static ZDD __attribute__((unused))
zddnode_low(ZDD zdd, zddnode_t node)
{
    return ZDD_TRANSFERMARK(zdd, zddnode_getlow(node));
}

static ZDD __attribute__((unused))
zddnode_high(ZDD zdd, zddnode_t node)
{
    return zddnode_gethigh(node);
    (void)zdd;
}

#endif
