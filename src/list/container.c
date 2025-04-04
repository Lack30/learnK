#include "container.h"

struct listNode *add_node(int value)
{
	struct listNode *node = kmalloc(sizeof(struct listNode *), GFP_KERNEL);
	INIT_LIST_HEAD(&node->head);
	node->value = value;
	return node;
}

void build_string(struct list_head *head, struct list_head *pos, char *buffer,
				  int *offset)
{
	if (list_is_head(pos, head))
		return;

	build_string(head, pos, buffer, offset); // 递归父目录

	// 拼接当前目录名
	int len = snprintf(buffer + *offset, PATH_MAX - *offset, ",%d",
					   list_entry(pos, struct listNode, head)->value);

	pos = pos->next;
	*offset += len;
}

char *to_string(struct list_head *head)
{
	struct list_head *pos;
	int written = 1;
	char *ptr = NULL;
	char *buffer = NULL;
	buffer = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buffer)
		return NULL;

	ptr = buffer;

	// 起始符 '['
	*ptr++ = '[';

	// 遍历链表（内核式遍历宏）
	list_for_each (pos, head) {
		struct listNode *node = list_entry(pos, struct listNode, head);

		// 添加逗号（非首元素）
		if (!list_is_first(pos, head)) {
			*ptr++ = ',';
		}

		// 写入数值（动态计算剩余空间）
		written = snprintf(ptr,
						   (buffer + PATH_MAX) - ptr, // 假设缓冲区足够大
						   "%d", node->value);
		ptr += written;
	}

	// 结束符 ']'
	*ptr++ = ']';

	*ptr = '\0'; // 终止符

	return buffer;
}

struct LinkList *add_link_list(void)
{
	struct LinkList *l = kmalloc(sizeof(struct LinkList *), GFP_KERNEL);
	INIT_LIST_HEAD(&l->head);
	l->count = 0;
	return l;
}

void link_list_get(struct LinkList *l, int index, int *ret)
{
	struct list_head *pos;
	struct listNode *node;
	if (!l->count || list_empty(&l->head)) {
		return;
	}

	if (index >= l->count) {
		return;
	}

	list_for_each (pos, &l->head) {
		node = list_entry(pos, struct listNode, head);
		*ret = node->value;
		if (!index)
			break;
		index -= 1;
	}
}

void link_list_insert(struct LinkList *l, int index, int value)
{
	struct list_head *pos;
	struct list_head *idx;
	struct listNode *node;
	if (!l->count || list_empty(&l->head)) {
		return;
	}

	if (index >= l->count) {
		return;
	}

	list_for_each_safe (pos, idx, &l->head) {
		if (!index) {
			node = add_node(value);
			list_add(&node->head, idx);
			break;
		}
		index -= 1;
	}
}

void link_list_remove(struct LinkList *l, int index)
{
	struct list_head *pos;
	struct list_head *idx;
	if (!l->count || list_empty(&l->head)) {
		return;
	}

	if (index >= l->count) {
		return;
	}

	list_for_each_safe (pos, idx, &l->head) {
		if (!index) {
			list_del(idx);
			break;
		}
		index -= 1;
	}
}

void link_list_push(struct LinkList *l, int value)
{
	struct listNode *node = add_node(value);
	list_add_tail(&node->head, &l->head);
	l->count += 1;
}

void link_list_pop(struct LinkList *l, int *ret)
{
	struct list_head *ptr;
	struct listNode *node;
	if (!l->count || list_empty(&l->head))
		return;

	ptr = l->head.next;
	if (ptr) {
		list_del(ptr);
		node = list_entry(ptr, struct listNode, head);
		*ret = node->value;
		kfree(node);
		l->count -= 1;
	}
}

int link_list_len(struct LinkList *l)
{
	return l->count;
}

void del_link_list(struct LinkList *l)
{
	struct list_head *idx;
	struct list_head *pos;
	list_for_each_safe (pos, idx, &l->head) {
		struct listNode *node = list_entry(pos, struct listNode, head);
		list_del(&node->head);
	}
	kfree(l);
	l = NULL;
}

struct Queue *add_queue(void)
{
	struct Queue *q = kmalloc(sizeof(struct Queue *), GFP_KERNEL);
	INIT_LIST_HEAD(&q->head);
	q->count = 0;
	return q;
}

void enqueue(struct Queue *q, int value)
{
	struct listNode *node = add_node(value);
	list_add_tail(&node->head, &q->head);
	q->count += 1;
}

void dequeue(struct Queue *q, int *ret)
{
	struct list_head *ptr;
	struct listNode *p;

	if (!q->count || list_empty(&q->head)) {
		return;
	}

	ptr = q->head.next;
	list_del(ptr);
	q->count -= 1;

	p = container_of(ptr, struct listNode, head);

	*ret = p->value;
	kfree(p);
}

int queue_len(struct Queue *q)
{
	return q->count;
}

void del_queue(struct Queue *q)
{
	struct list_head *idx;
	struct list_head *pos;
	list_for_each_safe (pos, idx, &q->head) {
		struct listNode *node = list_entry(pos, struct listNode, head);
		list_del(&node->head);
	}
	kfree(q);
	q = NULL;
}

struct Stack *add_stack(void)
{
	struct Stack *s = kmalloc(sizeof(struct Stack *), GFP_KERNEL);
	INIT_LIST_HEAD(&s->head);
	s->count = 0;
	return s;
}

void stack_push(struct Stack *s, int value)
{
	struct listNode *node = add_node(value);
	list_add(&node->head, &s->head);
	s->count += 1;
}

void stack_pop(struct Stack *s, int *ret)
{
	struct list_head *ptr;
	struct listNode *p;

	if (!s->count || list_empty(&s->head)) {
		return;
	}

	ptr = s->head.next;
	list_del(ptr);
	s->count -= 1;

	p = container_of(ptr, struct listNode, head);

	*ret = p->value;
	kfree(p);
}

int stack_len(struct Stack *s)
{
	return s->count;
}

void del_stack(struct Stack *s)
{
	struct list_head *idx;
	struct list_head *pos;
	list_for_each_safe (pos, idx, &s->head) {
		struct listNode *node = list_entry(pos, struct listNode, head);
		list_del(&node->head);
	}
	kfree(s);
	s = NULL;
}