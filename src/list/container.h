#ifndef __NODE_LIST_H
#define __NODE_LIST_H

#include <linux/list.h>
#include <linux/slab.h>

struct listNode {
	struct list_head head;
	int value;
};

struct LinkList {
	struct list_head head;
	int count;
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

char *to_string(struct list_head *head);

struct LinkList *add_link_list(void);

void link_list_get(struct LinkList *l, int index, int *ret);

void link_list_push(struct LinkList *l, int value);

void link_list_insert(struct LinkList *i, int index, int value);

void link_list_remove(struct LinkList *l, int index);

void link_list_pop(struct LinkList *l, int *ret);

int link_list_len(struct LinkList *l);

void del_link_list(struct LinkList *l);

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