#pragma once

#include "as_internal.h"

#include "as_iterator.h"

#define AS_STREAM_END ((void *) 0)

/******************************************************************************
 * TYPES
 ******************************************************************************/

typedef struct as_stream_s as_stream;
typedef struct as_stream_hooks_s as_stream_hooks;

/**
 * Stream Structure
 * Contains pointer to the source of the stream and a pointer to the
 * hooks that interface with the source.
 *
 * @field source the source of the stream
 * @field hooks the interface to the source
 */
struct as_stream_s {
    void * source;
    const as_stream_hooks * hooks;
};

/**
 * Stream Interface
 * Provided functions that interface with the streams.
 */
struct as_stream_hooks_s {
    int (*free)(as_stream *);
    as_val * (*read)(const as_stream *);
};

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Creates an iterator from the stream
 *
 * @param s the stream to create an iterator from
 * @return a new iterator
 */
as_iterator * as_stream_iterator(as_stream *);

/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

inline int as_stream_init(as_stream * s, void * source, const as_stream_hooks * hooks) {
    s->source = source;
    s->hooks = hooks;
    return 0;
}

inline int as_stream_destroy(as_stream * s) {
    return 0;
}

/**
 * Creates a new stream for a given source and hooks.
 *
 * @param source the source feeding the stream
 * @param hooks the hooks that interface with the source
 */
inline as_stream * as_stream_new(void * source, const as_stream_hooks * hooks) {
    as_stream * s = (as_stream *) malloc(sizeof(as_stream));
    as_stream_init(s, source, hooks);
    return s;
}

/**
 * Frees the stream
 *
 * Proxies to `s->hooks->free(s)`
 *
 * @param s the stream to free
 * @return 0 on success, otherwise 1.
 */
inline int as_stream_free(as_stream * s) {
    return as_util_hook(free, 1, s);
}

/**
 * Get the source for the stream
 *
 * @param stream to get the source from
 * @return pointer to the source of the stream
 */
inline void * as_stream_source(const as_stream * s) {
    return (s ? s->source : NULL);
}

/**
 * Reads an element from the stream
 *
 * Proxies to `s->hooks->read(s)`
 *
 * @param s the read to be read.
 * @return the element read from the stream or STREAM_END
 */
inline as_val * as_stream_read(const as_stream * s) {
    return as_util_hook(read, NULL, s);
}


