/******************************************************************************
 * Copyright 2008-2012 by Aerospike.  All rights reserved.
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE.  THE COPYRIGHT NOTICE
 * ABOVE DOES NOT EVIDENCE ANY ACTUAL OR INTENDED PUBLICATION.
 ******************************************************************************/

#pragma once

/**
 * Custome allocation size for clients
 */

#ifdef CF_QUEUE_ALLOCSZ
#undef CF_QUEUE_ALLOCSZ
#endif
#define CF_QUEUE_ALLOCSZ 64

#include "../cf_queue.h"