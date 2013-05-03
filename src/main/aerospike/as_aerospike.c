#include "as_aerospike.h"

#include <stdlib.h>
#include <cf_alloc.h>

#include "internal.h"


/******************************************************************************
 * INLINE FUNCTIONS
 ******************************************************************************/

extern inline int as_aerospike_rec_create(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_update(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_exists(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_rec_remove(const as_aerospike * a, const as_rec * r);
extern inline int as_aerospike_log(const as_aerospike * a, const char * name, 
    const int line, const int lvl, const char * msg);

extern inline as_rec *as_aerospike_crec_create(const as_aerospike * a, const as_rec * r);
extern inline as_rec *as_aerospike_crec_open(const as_aerospike * a, const as_rec *r, const char *);
extern inline int as_aerospike_crec_update(const as_aerospike * a, const as_rec * r, const as_rec *cr);
extern inline int as_aerospike_crec_close(const as_aerospike * a, const as_rec * r, const as_rec *cr);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

as_aerospike * as_aerospike_init(as_aerospike * a, void * s, const as_aerospike_hooks * h) {
	a->is_rcalloc = false;
    a->source = s;
    a->hooks = h;
    return a;
}

as_aerospike * as_aerospike_new(void * s, const as_aerospike_hooks * h) {
    as_aerospike * a = (as_aerospike *) cf_rc_alloc(sizeof(as_aerospike));
    a->is_rcalloc = true;
    a->source = s;
    a->hooks = h;
    return a;
}

void as_aerospike_destroy(as_aerospike * a) {
    if (a->is_rcalloc)
	   cf_rc_releaseandfree(a);
}
