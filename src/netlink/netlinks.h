#ifndef __NETLINK__
#define __NETLINK__

struct setup_param {
	int id;
	char *dev;
};

struct reload_param {
	int id;
	char bdev[64];
};

struct my_param {
	struct setup_param *setup;
};

#endif