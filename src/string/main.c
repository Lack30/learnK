#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

#define STRING_CAP 256

struct string_view {
	char *str;
	unsigned int len;
	unsigned long cap;
};

typedef struct string_view *string_t;

static string_t string_new(const char *str)
{
	string_t s = kmalloc(sizeof(string_t), GFP_KERNEL);
	if (!s)
		return NULL;

	s->len = strlen(str);
	s->cap = STRING_CAP;
	s->str = kmalloc(s->cap, GFP_KERNEL);
	if (!s->str) {
		kfree(s);
		return NULL;
	}

	strncpy(s->str, str, s->len);
	s->str[s->len] = '\0';
	return s;
}

static string_t string_new_cap(const char *str, unsigned long cap)
{
	string_t s;
	int len = strlen(str);
	if (len > cap) {
		pr_err("string length exceeds capacity\n");
		return NULL;
	}

	s = kmalloc(sizeof(string_t), GFP_KERNEL);
	if (!s)
		return NULL;

	s->len = len;
	s->cap = cap;
	s->str = kmalloc(s->cap, GFP_KERNEL);
	if (!s->str) {
		kfree(s);
		return NULL;
	}

	strncpy(s->str, str, s->len);
	s->str[s->len] = '\0';
	return s;
}

static void string_free(string_t s)
{
	if (s) {
		if (s->str) {
			kfree(s->str);
			s->str = NULL;
		}
		kfree(s);
	}
}

static void string_growup(string_t s, unsigned long cap)
{
	bool need_realloc = false;
	unsigned long factor = 0;
	while (cap >= s->cap) {
		if (s->cap <= 4096)
			factor = s->cap;
		else if (s->cap <= 8192)
			factor = do_div(s->cap, 2);
		else
			factor = do_div(s->cap, 4);
		s->cap += factor;
		need_realloc = true;
	}

	if (need_realloc)
		s->str = krealloc(s->str, s->cap, GFP_KERNEL);
}

static void string_join(string_t s, const char *str)
{
	unsigned int len = strlen(str);
	if (s->len + len >= s->cap)
		string_growup(s, s->len + len);

	strncat(s->str, str, s->cap - s->len);
	s->len += len;
	s->str[s->len] = '\0';
}

static int string_index(string_t s, const char *str)
{
	int i;
	int len = strlen(str);
	if (len > s->len)
		return -1;

	for (i = 0; i <= s->len - len; i++) {
		if (strncmp(s->str + i, str, len) == 0)
			return i;
	}

	return -1;
}

static int string_last_index(string_t s, const char *str)
{
	int i;
	int len = strlen(str);
	if (len > s->len)
		return -1;

	for (i = s->len - len; i >= 0; i--) {
		if (strncmp(s->str + i, str, len) == 0)
			return i;
	}

	return -1;
}

static int string_len(string_t s)
{
	return s->len;
}

static const char *string_char(string_t s)
{
	char *str = kmalloc(s->len + 1, GFP_KERNEL);
	if (!str)
		return NULL;
	strncpy(str, s->str, s->len);
	str[s->len] = '\0';
	return str;
}

static bool string_has_prefix(string_t s, const char *prefix)
{
	int len = strlen(prefix);
	if (len > s->len)
		return false;

	return strncmp(s->str, prefix, len) == 0;
}

static bool string_has_suffix(string_t s, const char *suffix)
{
	int len = strlen(suffix);
	if (len > s->len)
		return false;

	return strncmp(s->str + s->len - len, suffix, len) == 0;
}

static bool string_has_substring(string_t s, const char *substr)
{
	int len = strlen(substr);
	if (len > s->len)
		return false;

	return strstr(s->str, substr) != NULL;
}

static const char *string_substring(string_t s, int start, int end)
{
	char *substr = NULL;
	if (start < 0 || end > s->len || start >= end)
		return NULL;

	substr = kmalloc(end - start + 1, GFP_KERNEL);
	if (!substr)
		return NULL;

	strncpy(substr, s->str + start, end - start);
	substr[end - start] = '\0';
	return substr;
}

static void string_print(string_t s)
{
	printk(KERN_INFO "%s\n", s->str);
}

// 模块注册函数，在模块被加载时调用
static int __init __do_init(void)
{
	char *str = kmalloc(64, GFP_KERNEL);
	if (str)
		kfree(str);
	string_t s = string_new_cap("a", 2);
	const char *substr = NULL;
	if (!s) {
		printk(KERN_ERR "Failed to create string\n");
		return -ENOMEM;
	}

	string_join(s, "bc");
	string_print(s);

	string_join(s, "!!");

	pr_info("string length: %d\n", string_len(s));
	substr = string_char(s);
	pr_info("string: %s\n", substr);
	kfree(substr);

	if (string_has_prefix(s, "a")) {
		pr_info("string has prefix a\n");
	} else {
		pr_info("string does not have prefix a\n");
	}

	if (string_has_suffix(s, "!!")) {
		pr_info("string has suffix !!\n");
	} else {
		pr_info("string does not have suffix !!\n");
	}

	string_join(s, "hello world, this is a test");
	string_print(s);

	pr_info("string index: %d\n", string_index(s, "bc"));
	pr_info("string last index: %d\n", string_last_index(s, "is"));

	if (string_has_substring(s, "bc")) {
		pr_info("string has substring bc\n");
	} else {
		pr_info("string does not have substring bc\n");
	}

	substr = string_substring(s, 1, 3);
	pr_info("substring: %s\n", substr);
	kfree(substr);

	string_free(s);

	return -EINVAL;
}

// 模块注销函数，在模块被移除时调用
static void __exit __do_exit(void)
{
}

module_init(__do_init); // 注册模块启动函数
module_exit(__do_exit); // 注册模块注销函数

MODULE_LICENSE("GPL"); // 模块采用的协议
MODULE_AUTHOR("Lack"); // 模块作者