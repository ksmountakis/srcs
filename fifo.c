#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fifo.h"

void
fifo_init(fifo_t *f)
{
	f->head = f->tail = NULL;
}

void
fifo_free(fifo_t *f)
{
	while(!fifo_empty(f))
		(void)fifo_pop(f);
}

int
fifo_push(fifo_t *f, int v)
{
	fifonode_t *new = calloc(sizeof(fifonode_t), 1);
	assert(new);
	
	new->next = f->head;
	new->prev = NULL;
	new->v = v;
	
	if (f->head)
		f->head->prev = new;
	else
		f->tail = new;
		
	f->head = new;
	
	return v;
}

int
fifo_pop(fifo_t *f)
{
	int v;
	fifonode_t *oldtail;
	
	if(!f->tail) {
		assert(!f->head);
		return 0;
	}
	
	v = f->tail->v;
	oldtail = f->tail;
		
	if (f->head == oldtail) {
 		f->tail = f->head = NULL;
 		free(oldtail);
 		return v;
 	}
	free(oldtail);
	
	f->tail->prev->next = NULL;
	f->tail = f->tail->prev;
		
	return v;
}

int
fifo_empty(fifo_t *f)
{
	return f->head == NULL;
}

#if 0
int
main(void)
{
	unsigned int i;
	fifo_t f;
	
	fifo_init(&f);
	
	for (i=0; i < 10; i++)
		fifo_push(&f, i);
		
	for (i=0; i < 5; i++)
		printf("pop[%d]: %d \n", i, (int)fifo_pop(&f));
	puts("");
	
	for (i=11; i < 20; i++)
		fifo_push(&f, i);
	
	while(!fifo_empty(&f))
		printf("pop[%d]: %d \n", i++, (int)fifo_pop(&f));
		
	puts("");
		
	for (i=11; i < 20; i++)
		fifo_push(&f, i);
	
	while(!fifo_empty(&f))
		printf("pop[%d]: %d \n", i++, (int)fifo_pop(&f));	
	
	fifo_free(&f);
	return 0;
}
#endif
