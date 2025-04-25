
#ifndef _DUMP_H_
#define _DUMP_H_

#ifdef __KERNEL__

#include <linux/string.h>
#include <linux/slab.h>

#define dump_alloc(size) kmalloc(size, GFP_KERNEL)
#define dump_free(p)	 kfree(p)

#else

#include <stdlib.h>
#include <string.h>

#define dump_alloc(size) malloc(size)
#define dump_free(p)	 free(p)

#endif

struct params {
	unsigned int id;
	char *name;
};

char *marshal(struct params *params);

int unmarshal(char *buf, struct params *params);

#endif