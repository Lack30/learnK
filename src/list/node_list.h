#ifndef __NODE_LIST_H
#define __NODE_LIST_H

#include <linux/list.h>
#include <linux/slab.h>

struct listNode {
	struct list_head head;
	int value;
};

struct Queue {
	struct list_head head;
	int count;
};

struct Stack {
	struct list_head head;
	int count;
};

struct listNode *add_node(int value);

struct Queue *add_queue(void);

void enqueue(struct Queue *q, int value);

void dequeue(struct Queue *q, int *ret);

int queue_len(struct Queue *q);

void del_queue(struct Queue *q);

struct Stack *add_stack(void);

void stack_push(struct Stack *s, int value);

void stack_pop(struct Stack *s, int *ret);

int stack_len(struct Stack *s);

void del_stack(struct Stack *s);

#endif