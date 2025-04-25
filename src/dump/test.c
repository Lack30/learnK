#include <stdio.h>
#include "dumps.h"

int main() {
	char *buf;
	int ret;
	struct params *target;
	struct params *p = dump_alloc(sizeof(struct params));
	p->id = 1;
	p->name = dump_alloc(32);
	strncpy(p->name, "hello world1", 32);

	buf = marshal(p);
	if (!buf) {
		printf("marshal failed\n");
		dump_free(p->name);
		dump_free(p);
		return -1;
	} else {
		target = dump_alloc(sizeof(struct params));
		unmarshal(buf, target);
		printf("id: %d, name: %s\n", target->id, target->name);
	}

	if (p)
		dump_free(p);
}