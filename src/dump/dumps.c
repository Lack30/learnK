#include "dumps.h"

char *marshal(struct params *params)
{
	char *buf;
	int len;
	int offset;

	len = sizeof(unsigned int) + strlen(params->name) + 1;
	buf = dump_alloc(len);
	if (!buf)
		return NULL;

	memcpy(buf, &params->id, sizeof(unsigned int));
	offset = sizeof(unsigned int);
	strcpy(buf + offset, params->name);

	return buf;
}

int unmarshal(char *buf, struct params *params)
{
	int offset;

	memcpy(&params->id, buf, sizeof(unsigned int));
	offset = sizeof(unsigned int);
	params->name = dump_alloc(strlen(buf + offset) + 1);
	if (!params->name)
		return -1;

	strcpy(params->name, buf + offset);

	return 0;
}
