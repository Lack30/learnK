#include <linux/init.h>
#include <linux/module.h>

int add_integer(int a, int b) { return a + b; }
EXPORT_SYMBOL_GPL(add_integer);

int sub_integer(int a, int b) { return a - b; }
EXPORT_SYMBOL_GPL(sub_integer);

MODULE_LICENSE("GPL");