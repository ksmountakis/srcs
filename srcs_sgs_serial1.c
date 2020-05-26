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

static void
make_a1(struct itpair *a1, struct itpair *a, int l, int j, int last_l)
{
	/* copy first l+1 out of j, add j, copy last j-(l+1)
	 * l is a position (starts at 0)
	 * the number of elements before j is (l+1) */
	 
	memmove(a1, a, sizeof(struct itpair)*(l+1));
 	a1[l+1].i = j;
	a1[l+1].t = -1;
 	if (!last_l)
		memmove(&a1[l+2], &a[l+1], sizeof(struct itpair)*(j-(l+1)));
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
	int n0;
	int accept;
	int *truename;
} cutwalk_insert_arg;

static int
cutwalk_insert(p, k, es, a, s, j, l, dt, style)
	plan_t *p, *es; 
	int k, j, l;
	struct itpair *a;
	real_t *s, dt;
	cw_style_t style;
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
 	int n0 = cutwalk_insert_arg.n0;
	int *accept = &cutwalk_insert_arg.accept;
	//int *truename = cutwalk_insert_arg.truename;
  	
  	int u, iu, ju;
	int last_l = (l == j-1);
	
	make_a1(a1, a, l, j, last_l);
	
	/* Epsilon = E */ 
	srcs_plan_copy(es, p);
	
	/* initialize w[a1[0]][r] and g[a1[0]][r] */
	for (r=0; r < k; r++) {
		w[a1[0].i][r] = 0;
		g[a1[0].i][r] = b->cell[0][r]._int;
	}
 	 	
 	/* make a j-solution from scratch */
 	f1[0] = d->cell[0][0]._real;
 	*accept = 1;
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
   		
   		/* a1[y].i has been appended; 
   		 * compute its (new) start/finish time */  
   		  		
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
 		
 		*accept = *accept && 1;
   		if ((y > l+1) && (s1[a1[y].i] - s[a1[y].i] > dt)) {
			if (last_l) {
				assert(simnum > 1);
  			} else {
  				*accept = 0;
			}
		}
		/* a1[y].i was just added to es */
    }
    
    /* compute est of unscheduled tasks {j+1, .., n} */
    for (ju=j+1; ju < n0; ju++) {
		s1[ju] = f1[ju] = 0;
	
		for (sim=0; sim < simnum; sim++) {
			t = 0;
			for (iu=0; iu < ju; iu++) {
				if (es->edge[iu][ju]) {
					t = MAX(t, fmat1[sim][iu]);
 				}
			}
			smat1[sim][ju] = t;				
			fmat1[sim][ju] = t + d->cell[sim][ju]._real;
			
			/* accumulate for mean */
			s1[ju] += smat1[sim][ju];
			f1[ju] += fmat1[sim][ju];
		}
		s1[ju] /= (real_t)simnum;
		f1[ju] /= (real_t)simnum;
    }
    
    /* s1[] holds the whole ESS of the partial solution
     * by inserting j in position l */ 
    
    return 1;
}

static void
init_weights(real_t *W, int n, int k, matr_t *d, matr_t *q)
{
	int i, r; 

	for (i=0; i < n; i++) {
		W[i] = 0;
		
		for (r=0; r < k; r++) 
			W[i] += q->cell[i][r]._int;
		
		W[i] *= d->cell[0][i]._real;
 	}
}

static real_t
torque(real_t *s, real_t **smat1, int simnum, int n, int j, matr_t *d, real_t *W, real_t H, cw_style_t style)
{
	int i, sim;
	real_t T, di;
	
	T = 0;
	for (sim=0; sim < simnum; sim++) {
		for (i=0; i < n; i++) {
			di = d->cell[sim][i]._real;

			if (style == CW_TRQ_NUT)
				T += W[i] * (H - (s[i] + 0.5*di));		
				
			if (style == CW_TRQ)
				T += W[i] * (H - (s[i] + 0.5*di));
				
			if (style == CW_TRQ_CMX)
				T += (H/smat1[sim][n-1]) * W[i] * (H - (s[i] + 0.5*di));
			
			if (style == CW_TRQ_EXP) {
				T += W[i] *(1+(real_t)(i+1)/(real_t)n) * (H - (s[i] + 0.5*di));
			}
		}

	}
	
	return T/simnum;
}

static int
cutwalk_main(es, p, d, simnum, q, b, truename, dt, style)
	plan_t *es, *p;
	matr_t *d, *q, *b;
	int simnum, *truename;
	real_t dt;
	cw_style_t style;
{
	int n, k, i, j, l, jpreall, jpreseen, sim;
  	matr_t es_order;
  	plan_t es_best;
  	int es_order_n;
  	real_t *s;
  	struct itpair *a;
  	real_t es_score, es_best_score;
  	real_t *W = NULL, H;
     
	n = p->n;
	k = b->cols;
    
    assert(!srcs_plan_init(n, &es_best));
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
  	cutwalk_insert_arg.n0 = n;

	assert((cutwalk_insert_arg.s1 = calloc(sizeof(real_t), n)));
	assert((cutwalk_insert_arg.f1 = calloc(sizeof(real_t), n)));
	assert((cutwalk_insert_arg.smat1 = calloc(sizeof(real_t*), simnum)));
	assert((cutwalk_insert_arg.fmat1 = calloc(sizeof(real_t*), simnum)));	
	for (sim=0; sim < simnum; sim++) {
		assert((cutwalk_insert_arg.smat1[sim] = calloc(sizeof(real_t), n)));
		assert((cutwalk_insert_arg.fmat1[sim] = calloc(sizeof(real_t), n)));
	}
	cutwalk_insert_arg.truename = truename;
	
	/* initialize weights vector and horizon */
	assert((W = calloc(sizeof(real_t), n)));
	init_weights(W, n, k, d, q);
	H = 0;
	for (j=0; j < n; j++)
		H += d->cell[0][j]._real;
	
	/* initialize es with p */
	srcs_plan_copy(es, p);
		 
	/* loop through each task */
 	s[0] = 0;
  	for (j=1; j < n; j++) 
	{
 		/* resize data structures */
		p->n = es->n = es_best.n = es_order.cols = d->cols = q->rows = j+1;
	 		
		/* insert j-1 into a[] with priority s[j-1] */
		for (i=0; i < j; i++)
			itpair_sortadd(a, i, i, s[i]);
 		
		/* count j's number of predecessors */
		jpreall = 0;
 		for (i=0; i < es->n; i++)
			if (p->edge[i][j])
				jpreall++;
		
		/* assert acyclicity */
 		assert(!srcs_plan_order(&es_order, es, &es_order_n));
 		assert((es_order_n == es->n)); 
		
  		jpreseen = 0;
		es_best_score = -DBL_MAX;
 		for (l=0; l < j; l++)
		{ 
 			if (p->edge[a[l].i][j])
				jpreseen++;
						
			if (jpreall > jpreseen)
				continue;
			
			cutwalk_insert(p, k, es, a, s, j, l, dt, style);
			
			if (cutwalk_insert_arg.accept) {
				
				switch (style) {
					case CW_TRQ_NUT:
 						for (i=j+1; i < n; i++)
							cutwalk_insert_arg.s1[i] = 0;
					case CW_TRQ_EXP:				
					case CW_TRQ_CMX:			
					case CW_TRQ: 					
						es_score = torque(cutwalk_insert_arg.s1, cutwalk_insert_arg.smat1, simnum, 
								n, j, d, W, H, style);
						break;
						
					case CW_CMX: 
						es_score = -cutwalk_insert_arg.s1[n-1];
						break;
						
					case CW_SGS:
					default:
						es_score = -l;
						break;
				} 

				/* debug */
				#if 1
				printf("%d ", truename[j]);
  				for (i=0; i < n; i++)
					printf("%d %.0f ", truename[i], cutwalk_insert_arg.s1[i]);
				
				printf(" %f", es_score);
				if (es_score > es_best_score)
					printf(" accept\n");
				else
					printf(" reject\n");
				#else
				
				printf("# %-2d %-2d\n", truename[j], l);
				#endif
				
				if (es_score > es_best_score) {
					/* save new best solution */
					es_best_score = es_score;
					srcs_plan_copy(&es_best, es);	
					
 					/* save new anchor start times  */
					for (i=0; i <= j; i++)
						s[i] = cutwalk_insert_arg.s1[i];								
 				}
				
			} 
  		}
 		
 		assert(cutwalk_insert_arg.accept == 1);
 		assert(jpreseen == jpreall);
 		
	}
 	
 	srcs_plan_copy(es, &es_best);
	 
	/* cleanup */
	srcs_plan_free(&es_best);
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
   	
   	free(W);
   	
	return 0;
}

int
srcs_sgs_cutwalk1(plan_t *es, plan_t *p, 
	matr_t *d, int simnum, matr_t *q, matr_t *b, matr_t *eseq, real_t dt, cw_style_t style)
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
 	
	cutwalk_main(&es0, &p0, &d0, simnum, &q0, b, i02i, dt, style);
	
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
