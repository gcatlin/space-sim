#ifndef RINGBUF_C

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/pervognsen/bitwise/blob/master/ion/common.c
// https://github.com/nothings/stb/blob/master/stb_ds.h

typedef struct {
    size_t len;
    size_t cap;
    size_t idx;
    char buf[];
} rbuf_hdr_t;

#define rbuf__hdr(b) ((rbuf_hdr_t *)((char *)(b) - offsetof(rbuf_hdr_t, buf)))
#define rbuf__len(b) (rbuf__hdr(b)->len)
#define rbuf__cap(b) (rbuf__hdr(b)->cap)
#define rbuf__idx(b) (rbuf__hdr(b)->idx)
#define rbuf__off(b, i) (rbuf__len(b) == rbuf__cap(b) ? (rbuf__idx(b) + (i)) % rbuf__len(b) : rbuf__idx(b) + (i))

#define rbuf_len(b) ((b) ? rbuf__len(b) : 0)
#define rbuf_cap(b) ((b) ? rbuf__cap(b) : 0)
#define rbuf_idx(b) ((b) ? rbuf__idx(b) : 0)
// TODO set flag once len==cap so rbuf__off() is fast? use high bit of idx/cap?
#define rbuf__advance(b) (rbuf__len(b) < rbuf__cap(b) ? rbuf__len(b)++ : (rbuf__idx(b) = (rbuf__idx(b) + 1) % rbuf__len(b)))

// TODO support negative offsets relative to the last element?
#define rbuf_ptr(b, i) ((b) ? (b) + rbuf__off(b, i) : NULL)
#define rbuf_get(b, i) ((i) >= 0 ? *((b) + rbuf__off(b, i)) : *((b) + rbuf__off(b, rbuf__len(b) + (i))))
#define rbuf_push(b, ...) ((b) ? (b)[rbuf__off(b, rbuf__len(b))] = (__VA_ARGS__), rbuf__advance(b) : 0)
// TODO add tests for rbuf_empty and rbuf_full
#define rbuf_empty(b) (rbuf_len(b) == 0)
#define rbuf_full(b) ((b) && (rbuf__len(b) == rbuf__cap(b)))

// TODO rbuf_grow() for explicitly growing the buffer??? needs to use some logic from ion's buf__grow()
#define rbuf_init(b, n) ((b) && (n) <= rbuf__cap(b) ? 0 : ((b) = rbuf__malloc((b), (n), sizeof(*(b)))))
#define rbuf_reset(b) ((b) ? rbuf__len(b) = 0, rbuf__idx(b) = 0 : 0)
#define rbuf_free(b) ((b) ? (free(rbuf__hdr(b)), (b) = NULL) : 0)

void *rbuf__malloc(const void *buf, size_t cap, size_t elem_size)
{
    assert(cap <= (SIZE_MAX - offsetof(rbuf_hdr_t, buf))/elem_size);
    size_t size = offsetof(rbuf_hdr_t, buf) + cap*elem_size;
    rbuf_hdr_t *hdr = malloc(size);
    hdr->len = 0;
    hdr->cap = cap;
    hdr->idx = 0;
    return hdr->buf;
}

void *rbuf_export(const void *restrict buf, void *restrict dest, size_t count)
{
    size_t len = rbuf_len(buf);
    size_t idx = rbuf_idx(buf);
    size_t offset = len - idx;
    memcpy(dest, (void *)(buf + idx), offset);
    memcpy(dest + offset, (void *)buf, idx);
    return dest;
}

void rbuf_test(void)
{
    // NULL
    int *buf = NULL;
    assert(rbuf_len(buf) == 0);
    assert(rbuf_cap(buf) == 0);
    assert(rbuf_idx(buf) == 0);

    // Initialize
    int n = 1024;
    rbuf_init(buf, n);
    assert(rbuf_len(buf) == 0);
    assert(rbuf_cap(buf) == n);
    assert(rbuf_idx(buf) == 0);
    assert(rbuf_ptr(buf, 0) == buf);
    assert(rbuf_get(buf, 0) == buf[0]);

    // Fill to capacity
    for (int i = 0; i < n; i++) {
        rbuf_push(buf, i);
    }
    for (size_t i = 0; i < rbuf_len(buf); i++) {
        assert(buf[i] == i);
        assert(rbuf_get(buf, i) == i);
    }
    assert(rbuf_len(buf) == n);
    assert(rbuf_cap(buf) == n);
    assert(rbuf_idx(buf) == 0);
    assert(rbuf_ptr(buf, 0) == &buf[0]);
    assert(rbuf_ptr(buf, n - 1) == &buf[n - 1]);
    assert(rbuf_get(buf, 0) == buf[0]);
    assert(rbuf_get(buf, n - 1) == buf[n - 1]);
    assert(rbuf_get(buf, -1) == buf[n - 1]);

    // Fill beyond capacity
    rbuf_push(buf, n);
    assert(rbuf_len(buf) == n);
    assert(rbuf_cap(buf) == n);
    assert(rbuf_idx(buf) == 1);
    assert(rbuf_ptr(buf, 0) == &buf[1]);
    assert(rbuf_ptr(buf, n - 1) == &buf[0]);
    assert(rbuf_get(buf, 0) == buf[1]);
    assert(rbuf_get(buf, n - 1) == buf[0]);
    assert(rbuf_get(buf, n - 1) == n);
    for (size_t i = 0; i < rbuf_len(buf); i++) {
        assert(rbuf_get(buf, i) == i + 1);
    }

    // Reset
    rbuf_reset(buf);
    assert(rbuf_len(buf) == 0);
    assert(rbuf_cap(buf) == n);
    assert(rbuf_idx(buf) == 0);
    assert(rbuf_ptr(buf, 0) == buf);
    assert(rbuf_get(buf, 0) == buf[0]);

    // Free
    rbuf_free(buf);
    assert(buf == NULL);
    assert(rbuf_len(buf) == 0);
    assert(rbuf_cap(buf) == 0);
    assert(rbuf_idx(buf) == 0);
    assert(rbuf_ptr(buf, 0) == NULL);
}

#define RINGBUF_C
#endif
