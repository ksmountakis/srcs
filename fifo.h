#ifndef _FIFO_H
#define _FIFO_H

typedef struct fifonode {
	struct fifonode *next;
	struct fifonode *prev;
	int v;
}fifonode_t;

typedef struct fifo {
	fifonode_t *head;
	fifonode_t *tail;
}fifo_t;


void	fifo_init(fifo_t *f);

void	fifo_free(fifo_t *f);

int		fifo_push(fifo_t *f, int v);

int		fifo_pop(fifo_t *f);

int		fifo_empty(fifo_t *f);

#endif
