#include "node_list.h"

struct listNode *add_node(int value)
{
	struct listNode *node = kmalloc(sizeof(struct listNode *), GFP_KERNEL);
	INIT_LIST_HEAD(&node->head);
	node->value = value;
	return node;
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