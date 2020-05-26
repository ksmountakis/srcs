#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "srcs.h"

struct itpair {
	real_t t;
	int i;
};

static void
itpair_sortadd(struct itpair *a, int a_len, int i, real_t t)
{
	int x;
	
 	for (x=0; x < a_len; x++)
		if (a[x].t > t)
 			break;
 			
	memmove(&a[x+1], &a[x], sizeof(struct itpair)*(a_len-x));
	
	a[x].i = i;
	a[x].t = t;
}

/* cutwalk_insert
 * a[0...j-1] has j elements
 * a1[0...j] has j+1 elements
 *
 * j elements 0...j-1 in the solution;
 * add j as the (l+2)-th member of sequence a[0 .. l-1], j, a[l+1 ... j-1]
 * l >= position of last predecessor of j in a[]
 */
 
struct {
	struct itpair *a1, *a2;
	int **w;
	int **g;
	matr_t *b, *q;
	real_t **smat1, **fmat1;
	real_t *s1, *f1;
	int simnum;
	matr_t *d;
} cutwalk_insert_arg;

static int
cutwalk_insert(plan_t *p, int k, plan_t *es, 
	struct itpair *a, 
	real_t *s, 
	int j, int l, real_t dt)
{
	int r;
	struct itpair *a1 = cutwalk_insert_arg.a1;
	struct itpair *a2 = cutwalk_insert_arg.a2;
	int **w = cutwalk_insert_arg.w; 
	int **g = cutwalk_insert_arg.g;
 	int x, y, sim, simnum = cutwalk_insert_arg.simnum;
	real_t t;
	matr_t *d = cutwalk_insert_arg.d;
	matr_t *q = cutwalk_insert_arg.q;
	matr_t *b = cutwalk_insert_arg.b;
 	real_t **fmat1 = cutwalk_insert_arg.fmat1;
 	real_t **smat1 = cutwalk_insert_arg.smat1;
	real_t *s1 = cutwalk_insert_arg.s1;
	real_t *f1 = cutwalk_insert_arg.f1;
	
  	int u;
	int last_l;
	
	if (l == j-1)
		last_l = 1;
	else
		last_l = 0;

	/* construct a1: copy first l+1 out of j, add j, copy last j-(l+1)
	 * l is a position (starts at 0)
	 * the number of elements before j is (l+1) */
	 
	memmove(a1, a, sizeof(struct itpair)*(l+1));
 	a1[l+1].i = j;
	a1[l+1].t = -1;
 	if (!last_l)
		memmove(&a1[l+2], &a[l+1], sizeof(struct itpair)*(j-(l+1)));
	
	/* Epsilon = E */
	srcs_plan_copy(es, p);
	
	/* initialize w[a1[0]][r] and g[a1[0]][r] */
	for (r=0; r < k; r++) {
		w[a1[0].i][r] = 0;
		g[a1[0].i][r] = b->cell[0][r]._int;
	}
 	 	
 	/* 
 	 * make a solution from scratch
 	 */
 	
	f1[0] = d->cell[0][0]._real;
	for (y=1; y < j+1; y++)
	{
		/* insert a1[y-1].i into a2[] with priority t */
 		itpair_sortadd(a2, y-1, a1[y-1].i, f1[a1[y-1].i]);
 				
		/* append a1[y].i by drawing resources from a2[x=0].i,..,a2[x=y-1].i */				
		for (r=0; r < k; r++)
		{
			g[a1[y].i][r] = 0;
			w[a1[y].i][r] = q->cell[a1[y].i][r]._int;
			
 			for (x=0; x < y; x++)
			{	
				/* u = what a1[y].i draws from a2[x].i */
				
				u = MIN(w[a1[y].i][r], g[a2[x].i][r]);
				w[a1[y].i][r] -= u;
				g[a1[y].i][r] += u;
				g[a2[x].i][r] -= u;
				if (u > 0) 
					srcs_plan_set_edge(es, a2[x].i, a1[y].i, 2);
				
				if (w[a1[y].i][r] == 0)
					break;						
  			} 
   		}
   		
   		/* a1[y].i has been appended; compute its (new) start/finish time */   		
		s1[a1[y].i] = f1[a1[y].i] = 0;
	
		for (sim=0; sim < simnum; sim++)
		{
			t = 0;
			for (x=0; x < y; x++) {
				if (es->edge[a1[x].i][a1[y].i]) {
					t = MAX(t, fmat1[sim][a1[x].i]);
 				}
			}
			smat1[sim][a1[y].i] = t;				
			fmat1[sim][a1[y].i] = t + d->cell[sim][a1[y].i]._real;
			
			/* accumulate for mean */
			s1[a1[y].i] += smat1[sim][a1[y].i];
			f1[a1[y].i] += fmat1[sim][a1[y].i];
		}
		s1[a1[y].i] /= (real_t)simnum;
		f1[a1[y].i] /= (real_t)simnum;
 			
   		if ((y > l+1) && (s1[a1[y].i] - s[a1[y].i] > dt)) {
			if (last_l) {
				assert(!"should not happen");
 			} else {
 				return 0;
			}
		}

    }
    
    return 1;
}

static int
cutwalk_main(es, p, d, simnum, q, b, i2i0)
	plan_t *es, *p;
	matr_t *d, *q, *b;
	int simnum, *i2i0;
{
	int n, k, i, j, l, jpreall, jpreseen, sim;
  	matr_t es_order;
  	int es_order_n;
  	real_t *s;
  	struct itpair *a;
  	int ret;
    
	n = p->n;
	k = b->cols;
    
  	assert(!srcs_matr_init(1, n, &es_order));
  	assert((a = calloc(sizeof(struct itpair), n)));
  	assert((s = calloc(sizeof(real_t), n)));
  	
  	/* set cutwalk_insert arguments */
  	assert((cutwalk_insert_arg.a1 = calloc(sizeof(struct itpair), n)));
  	assert((cutwalk_insert_arg.a2 = calloc(sizeof(struct itpair), n)));	
  	assert((cutwalk_insert_arg.w = calloc(sizeof(int*), n)));
   	assert((cutwalk_insert_arg.g = calloc(sizeof(int*), n)));
  	for (i=0; i < n; i++) {
  		assert((cutwalk_insert_arg.w[i] = calloc(sizeof(int), k)));
  		assert((cutwalk_insert_arg.g[i] = calloc(sizeof(int), k)));
  	}
  	cutwalk_insert_arg.b = b;
  	cutwalk_insert_arg.q = q;
  	cutwalk_insert_arg.simnum = simnum;
  	cutwalk_insert_arg.d = d;

	assert((cutwalk_insert_arg.s1 = calloc(sizeof(real_t), n)));
	assert((cutwalk_insert_arg.f1 = calloc(sizeof(real_t), n)));
	assert((cutwalk_insert_arg.smat1 = calloc(sizeof(real_t*), simnum)));
	assert((cutwalk_insert_arg.fmat1 = calloc(sizeof(real_t*), simnum)));	
	for (sim=0; sim < simnum; sim++) {
		assert((cutwalk_insert_arg.smat1[sim] = calloc(sizeof(real_t), n)));
		assert((cutwalk_insert_arg.fmat1[sim] = calloc(sizeof(real_t), n)));
	}
	 
	/* loop through each task */
	s[0] = 0;
  	for (j=n-1; j < n; j++) 
	{		
 		/* resize data structures */
		p->n = es->n = es_order.cols = d->cols = q->rows = j+1;
	 
		//printf("%d is now fixed [%.1f - %.1f]\n", j-1, s[j-1], f[j-1]);
		
		/* insert j-1 into a[] with priority s[j-1] */
		for (i=0; i < j; i++)
			itpair_sortadd(a, i, i, s[i]);
  		
		/* count j's number of predecessors */
		jpreall = 0;
 		for (i=0; i < es->n; i++)
			if (es->edge[i][j])
				jpreall++;
		
		/* assert acyclicity */
 		assert(!srcs_plan_order(&es_order, es, &es_order_n));
 		assert((es_order_n == es->n)); 
				
		jpreseen = 0;
 		for (l=0; l < j; l++)
		{ 
 			if (p->edge[a[l].i][j])
				jpreseen++;
						
			if (jpreall > jpreseen)
				continue;
			
			/* rho: l = number of elements before j;
			 * so to have j at the j-th position we need l=j-1 */
			 if (l < j-1)
			 	continue;
				
			if ((ret = cutwalk_insert(p, k, es, a, s, j, l, 0))) {
				/* save new start times if insertion was not rejected */
				for (i=0; i <= j; i++)
					s[i] = cutwalk_insert_arg.s1[i];
					
				//fprintf(stdout, "j %d l %d last %d\n", j, l, (l == j-1));
  				break;
			}
 		}
 		assert(ret == 1);
 		assert(jpreseen == jpreall);
	}
 	
	 
	/* cleanup */
	srcs_matr_free(&es_order);
	
	for (sim=0; sim < simnum; sim++) {
		free(cutwalk_insert_arg.smat1[sim]);
		free(cutwalk_insert_arg.fmat1[sim]);
	}
	free(cutwalk_insert_arg.smat1);
	free(cutwalk_insert_arg.fmat1);
	free(cutwalk_insert_arg.s1);
	free(cutwalk_insert_arg.f1);
	
	free(a);
 	free(s);
 	
 	free(cutwalk_insert_arg.a1); 
  	for (i=0; i < n; i++) { 
  		free(cutwalk_insert_arg.w[i]);
  		free(cutwalk_insert_arg.g[i]);
   	}
   	free(cutwalk_insert_arg.w);
   	free(cutwalk_insert_arg.g);
   	
	return 0;
}

int
srcs_sgs_cutwalk_rho(plan_t *es, plan_t *p, 
	matr_t *d, int simnum, matr_t *q, matr_t *b, matr_t *eseq)
{
	matr_t d0, q0;
	plan_t es0, p0;
	int *i2i0, *i02i;
	int i, j, r, i0, j0, x, sim;
	
	assert(!srcs_plan_init(p->n, &es0));
	assert(!srcs_plan_init(p->n, &p0));	
	assert(!srcs_matr_init(d->rows, d->cols, &d0));
	assert(!srcs_matr_init(q->rows, q->cols, &q0));	
	
	/* i2i0[] maps tasks to eseq order */
	assert((i2i0 = calloc(sizeof(int), p->n)));
	assert((i02i = calloc(sizeof(int), p->n)));
	for (i=0; i < p->n; i++)
	{
		x = 0;
		while(eseq->cell[0][x++]._int != i);
				
		i2i0[i] = x-1;
		i02i[x-1] = i;
 	}
 	
 	/* use i2i0[] to translate d[], q[], p[], es[] */
 	for (i=0; i < p->n; i++)
 	{
 		i0 = i2i0[i];
 		
		for (sim=0; sim < simnum; sim++)
			d0.cell[sim][i0] = d->cell[sim][i];
 		
 		for (r=0; r < q->cols; r++)
 			q0.cell[i0][r] = q->cell[i][r];
 		
 		for (j=0; j < p->n; j++)
 		{
 			j0 = i2i0[j];
 			
 			assert(!srcs_plan_set_edge(&p0, i0, j0, p->edge[i][j]));
  			assert(!srcs_plan_set_edge(&es0, i0, j0, es->edge[i][j]));
 		}
 	}
 	
	cutwalk_main(&es0, &p0, &d0, simnum, &q0, b, i02i);
	
	/* use i02i[] to translate es0[] */
	for (j=0; j < p->n; j++)
		for (i=0; i < p->n; i++)
			srcs_plan_set_edge(es, i, j, es0.edge[i2i0[i]][i2i0[j]]);
	
	free(i2i0);
	free(i02i);
	srcs_matr_free(&d0);
	srcs_matr_free(&q0);
 	srcs_plan_free(&p0);
	srcs_plan_free(&es0);
	
	return 0;
}
