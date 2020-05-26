#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_statistics.h>
#include "srcs.h"
#include "fifo.h"

#define SOLVE_ALL 1

int srcs_seed;

int print;
int staprint;

static real_t
normstdsample(void)
{
	static gsl_rng *gsl = NULL; 
	
	if (!gsl)
		gsl = gsl_rng_alloc(gsl_rng_default);

	return (real_t)gsl_ran_gaussian_ziggurat(gsl, 1) + 0;
}

static real_t
normpdf(real_t x)
{
	return (1.0/sqrt(2.0*M_PI))*(double)expf(-0.5*(float)x*(float)x);
}

static real_t
normcdf(real_t x)
{
	return (double)0.5*erfcf(-(float)x/(float)M_SQRT2);
}

static int
bindice(void)
{
	return (rand() >= RAND_MAX/2)? 1: 0;
}

void
srcs_rvec_realize(rvar_t *V, int n, real_t *v)
{
	int k = V[0].k;
	int i, j;
	real_t gsv;
	
	assert(V && v);
	
	for (j=0; j < n; j++) {
		assert(V[j].k == k);
		v[j] = V[j].m1;
	}
	
	for (i=0; i < k; i++) {
		gsv = normstdsample();
		//gsv = (real_t)rand()/(real_t)RAND_MAX;
		for (j=0; j < n; j++)
			v[j] += V[j].cov[i] * gsv;
	}
}


static void
simulate(plan_t *plan, pair_t *bfs,
	cell_t *d, cell_t *s, cell_t *c)
{
	int i, j, a1, a2;

	for (i=0; i < plan->n; i++) {
		a2 = bfs[i].x._int;
		s[a2]._real = 0;
		
		for (j=0; j < i; j++) {
			a1 = bfs[j].x._int;
			if (!plan->edge[a1][a2])
				continue;

			if (c[a1]._real > s[a2]._real)
				s[a2]._real = c[a1]._real;
		}
		c[a2]._real = s[a2]._real + d[a2]._real;
	}
}

int
srcs_simulate(plan_t *plan, pair_t *dist,
	int numsim, matr_t *d, matr_t *s, matr_t *c)
{
	int i;
	
	assert(plan && d && s && c);

	for (i=0; i < numsim; i++)
		simulate(plan, dist, d->cell[i], s->cell[i], c->cell[i]);

	return 0;
}

int
srcs_simulate_tard(matr_t *c, int numsim, matr_t *due, matr_t *t)
{
	int i, a;
	real_t dd;
	
	assert(c && due && t);
	assert(c->cols <= due->cols);
	
	for (i=0; i < numsim; i++) {
		t->cell[i][0]._real = 0;
		for (a=0; a < c->cols; a++) {
			if ((dd = due->cell[0][a]._real) >= 0)
				t->cell[i][0]._real += MAX(0, c->cell[i][a]._real - dd);
		}
	}
	
	return 0;
}


real_t
srcs_rob(rvar_t *C, matr_t *due, int n, int type)
{	
	int a;
	real_t rob;
	
	rob = 0;
	for (a=0; a < n; a++) {
		if (due->cell[0][a]._real == -1)
			continue;
		
		rob += -(C[a].m1 + sqrt(C[a].m2));
	}
	
	return rob;
}


int
srcs_plan_init(int n, plan_t *plan)
{
	int i;
	assert(plan);

	plan->n = n;
	assert((plan->edge = calloc(sizeof(int*), plan->n)));
	assert((plan->dirt = calloc(sizeof(int*), plan->n)));
	assert((plan->numpre = calloc(sizeof(int), plan->n)));

	for (i=0; i < n; i++) {
		assert((plan->edge[i] = calloc(sizeof(int*), plan->n)));
		plan->dirt[i] = 1;
	}

	return 0;
}

void
srcs_plan_free(plan_t *plan)
{
	int i;
	
	assert(plan);
	
	for (i=0; i < plan->n; i++)
		free(plan->edge[i]);
	
	free(plan->edge);
	free(plan->dirt);
	free(plan->numpre);
}

int
srcs_plan_copy(plan_t *dst, plan_t *src)
{
	int a1, a2;
	
	assert(dst && src);
	
	if (dst->n != src->n)
		return 1;
		
	for (a1=0; a1 < src->n; a1++)
		for (a2=0; a2 < src->n; a2++)
			srcs_plan_set_edge(dst, a1, a2, src->edge[a1][a2]);
	
	return 0;
}

int
srcs_plan_set_edge(plan_t *plan, int a1, int a2, int e)
{
	assert(plan && (a1 >= 0) && (a2 >= 0));
	if ((e && (a1 == a2))) {
		return 1;
	}

	if ((a1 > plan->n) || (a2 > plan->n))
		return 1;
		
	if ((!plan->edge[a1][a2]) && (e)) {
		plan->dirt[a2] = e;
		plan->numpre[a2]++;
	}
	if ((plan->edge[a1][a2]) && (!e)) {
		plan->numpre[a2]--;
	}
		
	plan->edge[a1][a2] = e;
	
	return 0;
}

int
srcs_plan_print(plan_t *plan, FILE *f)
{
	int a1, a2;
	
	for (a1=0; a1 < plan->n; a1++)
		for (a2=0; a2 < plan->n; a2++)
			fprintf(f, "%d|", plan->edge[a1][a2]);
	fprintf(f, "\n");
	
	return 0;
}

static int 
dist_compar(const void *d1ptr, const void *d2ptr)
{
	pair_t *d1 = (pair_t*)d1ptr;
	pair_t *d2 = (pair_t*)d2ptr;
	
	return d1->y._int - d2->y._int;
}

#if 0
static void
plan_dfs(plan_t *plan, int a1, pair_t *dist, int level)
{
	int a2;
	
	if (dist[a1].y._int < level) {
		dist[a1].y._int = level;
		dist[a1].x._int = a1;
	}
		
	for (a2=0; a2 < plan->n; a2++) {
		if (!plan->edge[a1][a2])
			continue;
			
		plan_dfs(plan, a2, dist, level+1);
	}
}

int
srcs_plan_dfs(plan_t *plan, int a1, pair_t *dist)
{	
	assert(plan && dist && a1 >= 0);
	
	memset(dist, 0, sizeof(pair_t)*plan->n);
	plan_dfs(plan, 0, dist, 0);
	qsort(dist, plan->n, sizeof(pair_t), dist_compar);
	
	return 0;
}
#endif

void
srcs_bfsctx_init(bfsctx_t *ctx, int n)
{
	assert(ctx);
	assert((ctx->fifo = calloc(sizeof(int), n)));
	assert((ctx->discov = calloc(sizeof(int), n)));
	assert((ctx->numipre = calloc(sizeof(int), n)));
}

void
srcs_bfsctx_free(bfsctx_t *ctx)
{
	assert(ctx);
	free(ctx->fifo);
	free(ctx->discov);
	free(ctx->numipre);
}

static void
plan_bfs(bfsctx_t *ctx, plan_t *plan, int a0, pair_t *dist, int rand)
{
	int *fifo, fifoindx;
	int *discov, *numipre;
	int a1, a2;
	int start, end, step;
	int bfsindx;
	bfsctx_t localctx;
	
	if (ctx == NULL) {
		ctx = &localctx;
		srcs_bfsctx_init(ctx, plan->n);
	}
	
	fifo = ctx->fifo;
	discov = ctx->discov;
	numipre = ctx->numipre;
	
	for (a2=0; a2 < plan->n; a2++) {
		numipre[a2] = 0;
		for (a1=0; a1 < plan->n; a1++)
			if (plan->edge[a1][a2])
				numipre[a2]++;
		discov[a2] = 0;
		dist[a2].x._int = a2;
		dist[a2].y._int = 0;
	}
	
	fifoindx = 0;
	fifo[fifoindx++] = a0;
	bfsindx = 0;
	
	while (fifoindx) {
		a1 = fifo[--fifoindx];
		dist[a1].y._int = bfsindx++;
		
		if (rand && bindice()) {
			step = -1;
			start = plan->n-1;
			end = -1;
		} else {
			step = +1;
			start = 0;
			end = plan->n;
		}
		for (a2=start; a2 != end; a2 += step) {
			if (!plan->edge[a1][a2])
				continue;

			//if (dist[a1].y._int+1 > dist[a2].y._int)
			//	dist[a2].y._int = dist[a1].y._int+1;
				
			if (++discov[a2] == numipre[a2])
				fifo[fifoindx++] = a2;
		}
	}
	if (ctx == &localctx)
		srcs_bfsctx_free(ctx);
}

int
srcs_plan_bfs(bfsctx_t *ctx, plan_t *plan, int a1, pair_t *dist, int rand)
{	
	assert(plan && dist && a1 >= 0);
	
	memset(dist, 0, sizeof(pair_t)*plan->n);
	plan_bfs(ctx, plan, 0, dist, rand);
	qsort(dist, plan->n, sizeof(pair_t), dist_compar);

	return 0;
}

static int
plan_bfs2(plan_t *plan, int src, int *lifo, int lifo_i, int *mark, matr_t *bfs)
{
	int i, j, ri, rj;
	int bfs_i = 0;
	
	while (lifo_i-- > 0) {
		i = lifo[lifo_i];
		bfs->cell[0][bfs_i++]._int = i;
		ri = bfs->cell[1][i]._int;
		
		for (j=0; j < plan->n; j++) {
			if (plan->edge[i][j]) {
				/* get rank of j */
				rj = bfs->cell[1][j]._int;
				rj = MAX(rj, ri+1);
				bfs->cell[1][j]._int = rj;
				
				/* put j in lifo */
				if (!mark[j]) {
					mark[j] = 1;
					lifo[lifo_i++] = j;
				}
			}
		}
	}
	
	return bfs_i;
}

int
rankcmp(const void *e1ptr, const void *e2ptr)
{
	int r1 = ((pair_t*)e1ptr)->y._int;
	int r2 = ((pair_t*)e2ptr)->y._int;
	
	return r1 - r2;
}

int
srcs_plan_bfs2(plan_t *plan, int src, matr_t *bfs)
{
	int *lifo, *mark, i, bfs_n;
	assert(plan && src >= 0 && bfs);
	pair_t *rank;
	
	assert((lifo = calloc(sizeof(int), plan->n)));
	assert((mark = calloc(sizeof(int), plan->n)));
	assert((rank = calloc(sizeof(pair_t), plan->n)));
	
	memset(bfs->cell[0], 0, sizeof(cell_t)*plan->n);
	memset(bfs->cell[1], 0, sizeof(cell_t)*plan->n);
	memset(bfs->cell[2], 0, sizeof(cell_t)*plan->n);
	
	lifo[0] = src;
	mark[src] = 1;
	
	bfs_n = plan_bfs2(plan, src, lifo, 1, mark, bfs);
	
	for (i=0; i < bfs_n; i++) {
		rank[i].x._int = i;
		rank[i].y._int = bfs->cell[1][i]._int;
	}
	qsort(rank, bfs_n, sizeof(pair_t), rankcmp);
	for (i=0; i < bfs_n; i++) 
		bfs->cell[2][i]._int = rank[i].x._int;

	
	free(lifo);
	free(mark);
	free(rank);
	
	return 0;
}

int
srcs_plan_tc(plan_t *tc, plan_t *plan, matr_t *order)
{
	int i, j, k;
	int orderi, orderj, orderk;
	
	srcs_plan_copy(tc, plan);
	
	for (orderi=0; orderi < plan->n; orderi++) {
		i = order->cell[0][orderi]._int;
		
		for (orderj=0; orderj < plan->n; orderj++) {
			j = order->cell[0][orderj]._int;
			
			if (tc->edge[i][j]) {
			
				for (orderk=0; orderk < plan->n; orderk++) {
					k = order->cell[0][orderk]._int;

					if (tc->edge[k][i])
						tc->edge[k][j] = tc->edge[k][i];
				}
			}
		}
	}
	
	return 0;
}


int	
srcs_rvar_init(int k, rvar_t *V)
{

	assert((k >= 0) && V);
	
	V->k = k;
	assert((V->cov = calloc(sizeof(real_t*), V->k)));
		
	return 0;
}

void
srcs_rvar_free(int k, rvar_t *V)
{
	assert(V);
	if (V->k > 0) {
		assert(V->cov);
		free(V->cov);
	}
}

int
srcs_rvar_copy(rvar_t *dst, rvar_t *src)
{
	assert(dst && src);
	
	if (dst->k != src->k)
		return 1;
	
	dst->m1 = src->m1;
	dst->m2 = src->m2;
	memmove(dst->cov, src->cov, sizeof(real_t)*src->k);
	
	return 0;
}

int
srcs_rvar_zero(rvar_t *X)
{
	X->m1 = X->m2 = 0;
	memset(X->cov, 0, sizeof(real_t)*X->k);
	return 0;
}


static real_t
covariance(rvar_t *X1, rvar_t *X2)
{	
	int i;
	real_t cov12 = 0;
	
	assert(X1 && X2);
	
	for(i=0; i < X1->k; i++)
		cov12 += X1->cov[i] * X2->cov[i];
		
	return cov12;

}

int
srcs_rvar_set_cov(rvar_t *V, int j, real_t cov)
{
	V->cov[j] = cov;
	V->m2 = 0;
	for (j=0; j < V->k; j++)
		V->m2 += pow(V->cov[j], 2);
		
	return 0;
}


static int
lgsta_max(rvar_t *Y, rvar_t *X1, rvar_t *X2)
{
	int i;
	
	assert(Y && X1 && X2);
	
	if (!(Y->k == X1->k && X1->k == X2->k))
		return 1;

	real_t dev1 = sqrt(X1->m2);
	real_t dev2 = sqrt(X2->m2);
	real_t cov12 = covariance(X1, X2);
	real_t rho12;
	
	if (dev1 == 0 || dev2 == 0)
		rho12 = 0;
	else
		rho12 = cov12 / (dev1 * dev2);
	real_t xx1 = X1->m2 + X2->m2;
	real_t xx2 = cov12 * 2.0;
	
	real_t theta = (xx1 - xx2 < 0.001)? 0: sqrt(xx1 - xx2);
	real_t alpha;
	if (theta == 0)
		alpha = 0;
	else
		alpha = (X1->m1 - X2->m1)/theta;
	real_t tight1 = normcdf(alpha);
	real_t tight2 = 1.0 - tight1;

	if ((rho12 == 1) && (X1->m2 == X2->m2)) {
		srcs_rvar_copy(Y, (X1->m1 > X2->m1)? X1: X2);
		return 0;
	}
	
	/* compute Y->m1 */
	Y->m1 = X1->m1*tight1 + X2->m1*tight2 + theta*normpdf(alpha);
	
	/* compute Y->cov[] */
	double m2fake = 0;
	for (i=0; i < Y->k; i++) {
		Y->cov[i] = (tight1*X1->cov[i]) + (tight2*X2->cov[i]);
		m2fake += Y->cov[i] * Y->cov[i];
	}
	Y->m2 = m2fake;
	
	#if 0
	double m2real = 0;
	m2real = tight1*(X1->m1*X1->m1 + X1->m2) +
			 tight2*(X2->m1*X2->m1 + X2->m2) +
			 (X1->m1 + X2->m1) * alpha * normpdf(alpha) -
			 pow(Y->m1, 2);
	
	Y->m2real = m2real;
	#endif

	return 0;
}

static int
lgsta_sum(rvar_t *Y, rvar_t *X1, rvar_t *X2)
{
	int i;
	
	assert(Y && X1 && X2);
	if (!(Y->k == X1->k && X1->k == X2->k))
		return 1;
		
	Y->m1 = X1->m1 + X2->m1;
	Y->m2 = 0;
	for (i=0; i < Y->k; i++) {
		Y->cov[i] = X1->cov[i] + X2->cov[i]; 
		Y->m2 += pow(Y->cov[i], 2);
	}
	
	Y->m2real = Y->m2;

	return 0;
}

int
srcs_lgsta(plan_t *plan, pair_t *dist,
	rvar_t *D, rvar_t *S, rvar_t *C)
{
	int a1, a2, i, j, err;
	

	for (i=0; i < plan->n; i++) {
		a2 = dist[i].x._int;
		
		srcs_rvar_zero(&S[a2]);
		srcs_rvar_zero(&C[a2]);
		
		/* copy in X the completion time of
		 * the first imm. predecessor of a2 */
		 
		for (j=0; j < i; j++) {
			a1 = dist[j].x._int;
				
			if (plan->edge[a1][a2]) {
 				if ((err=srcs_rvar_copy(&S[a2], &C[a1])))
 					return err;

 				break;
 			}
		}
		
		/* compute in X the maximum of completion
		 * times of remaining imm. predecessors */
	
		for (j=j+1; j < i; j++) {
			a1 = dist[j].x._int;

			if (!plan->edge[a1][a2]) 
				continue;
			
			/* a1 = ipre(a2, j) */
			/* X = max(X, C[a1]) */
		
			if((err=lgsta_max(&S[a2], &S[a2], &C[a1])))
				return err;
		}
		
		/* S[a2] = X
		 * C[a2] = S[a2] + D[a2] */
		 
		if ((err=lgsta_sum(&C[a2], &S[a2], &D[a2])))
			return err;

	}
	
	return 0;
}

#ifdef INC
int
srcs_lgsta_inc(plan_t *plan, pair_t *dist, int *dirt, char **edirt,
	rvar_t *D, rvar_t *S, rvar_t *C)
{
	int a1, a2, a3, i, j, err;

	assert(dirt);
	
	for (i=0; i < plan->n; i++) {

		a2 = dist[i].x._int;
	
		/* skip if not dirty */
		if (!dirt[a2])		
			continue;
		
		if (print) printf("S[%-3d]: %.1f %.1f: ", a2, S[a2].m1, S[a2].m2);

		/* compute in X the maximum of completion
		 * times of remaining imm. predecessors */
		for (j=0; j < i; j++) {

			a1 = dist[j].x._int;

			if (!edirt[a1][a2])
				continue;
			edirt[a1][a2] = 0;

			/* a1 = ipre(a2, j) */
			/* X = max(X, C[a1]) */
			if((err=lgsta_max(&S[a2], &S[a2], &C[a1])))
				return err;
			
			if (print) printf("{%d}%-2d[%.1f,%.1f] ", 
						plan->edge[a1][a2], a1, S[a2].m1, S[a2].m2);
			
		}
		
		/* S[a2] = X
		 * C[a2] = S[a2] + D[a2] */
		 
		if (print) printf("\n");

		if ((err=lgsta_sum(&C[a2], &S[a2], &D[a2])))
			return err;
			

		/* not dirty anymore */
		if (dirt) {
			for (j=i+1; j < plan->n; j++) {
				a3 = dist[j].x._int;
				if (plan->edge[a2][a3]) {
					dirt[a3] = 1;
					edirt[a2][a3] = 1;
				}
			}
		}

	}

	if (dirt)
		memset(dirt, 0, sizeof(int)*plan->n);
	
	return 0;
}
#endif

int
srcs_lgsta_tard(rvar_t *C, matr_t *due, rvar_t *T)
{
	rvar_t Z, Ta;
	real_t dd;
	int a;
	
	assert(C && due && T);
	assert(C[0].k == T->k);
	
	srcs_rvar_init(C[0].k, &Z);
	srcs_rvar_init(C[0].k, &Ta);
	
	srcs_rvar_zero(&Z);
	srcs_rvar_zero(T);
	
	for (a=0; a < due->cols; a++) {
		if ((dd = due->cell[0][a]._real) < 0)
			continue;
			
		C[a].m1 -= dd;
		srcs_rvar_zero(&Ta);
		lgsta_max(&Ta, &Z, &C[a]);
		lgsta_sum(T, T, &Ta);
		C[a].m1 += dd;
	}
	
	srcs_rvar_free(Z.k, &Z);
	srcs_rvar_free(Ta.k, &Ta);
	
	return 0;
}


typedef struct solve_ctx {
	pair_t *soldist;
	int **cfg;
	rvar_t *S,  *C, *T;
	matr_t *s, *c, *t;
	bfsctx_t *bfsctx;
	real_t **dmatr;
	char **edirt;
}solve_ctx_t;

#if 0
static real_t
heu_ovp(int a1, int a2, rvar_t *S, rvar_t *C)
{
	
	real_t score;
	score = S[a1].m1;
	real_t lc1, es2;
	real_t x;
	
	x = 6;
	lc1 = C[a1].m1 + x*sqrt(C[a1].m2);
	es2 = S[a2].m1 - x*sqrt(S[a2].m2);
	
	score += lc1 - es2;

	return -score;
}

static int
solve(solve_ctx_t *ctx, pair_t *dist,  pair_t *resrank,
		rvar_t *D, rcons_t *res, 
		matr_t *due, int robtype, int ssmeth, int numsim, 
		plan_t *sol, real_t *rob, real_t *m1, real_t *m2)
{
	int i, j, r, m, a1, a2, added;
	int **cfg = ctx->cfg;
	rvar_t *S = ctx->S, *C = ctx->C, *T = ctx->T;
	matr_t *s = ctx->s, *c = ctx->c, *t = ctx->t;
	real_t **dmatr = ctx->dmatr;
	/* char **edirt = ctx->edirt; */
	int dirt[sol->n];
	real_t score, topscore;
	int top;
	
	/* initialize cfg and dirt */
	for (r=0; r < res->k; r++)
		for (m=0; m < res->cap[r]; m++)
			cfg[r][m] = -1;
	
	memset(dirt, 0 , sizeof(int)*sol->n);
	
	for (i=0; i < sol->n; i++) {
		a2 = dist[i].x._int;
		
		/* re-map to stochastic schedule */
		if (ssmeth == SSMETH_STA) {
			#ifdef INC
			assert(!srcs_lgsta_tard(C, due, T));
			#else
			assert(!srcs_lgsta(sol, dist, D, S, C));
			#endif
	
		} else if (ssmeth == SSMETH_SIM) {
			assert(!srcs_simulate(sol, dist, D, numsim, dmatr, s, c));
			assert(!srcs_rvec_sim(c, numsim, sol->n, C));
			assert(!srcs_rvec_sim(s, numsim, sol->n, S));
		}
		
		for (j=0; j < res->k; j++) {
			r = resrank[j].x._int;
			added = 0;
			m = 0;
			
			while(added < res->req[a2][r]) {
				#if 0
				do m = rand() % res->cap[r];
				while (cfg[r][m] == a2);
				#else
				topscore = -4000000;
				top = 0;
				for (m=0; m < res->cap[r]; m++) {
					a1 = cfg[r][m];
					if (a1 == a2) 
						continue;
					if (a1 == -1) {
						top = m;
						break;
					}
					score = heu_ovp(a1, a2, S, C);
					if (score > topscore) {
						topscore = score;
						top = m;
					}
				}
				m = top;
				#endif
				
				a1 = cfg[r][m];
				if ((a1 != -1) && (!sol->edge[a1][a2])) {
					srcs_plan_set_edge(sol, a1, a2, 2);
					dirt[a2] = 1;
					#ifdef INC
					edirt[a1][a2] = 1;
					#endif
 				}
				cfg[r][m] = a2;
				added++;
				m++;
			}
		}
	}
	
	/* re-map to stochastic schedule */
	if (ssmeth == SSMETH_STA) {
		assert(!srcs_lgsta_tard(C, due, T));
	} else if (ssmeth == SSMETH_SIM) {
		assert(!srcs_simulate_tard(c, numsim, due, t));
		assert(!srcs_rvec_sim(t, numsim, 1, T));
	}

	#if 1
	*m1	= T->m1;
	*m2	= T->m2;
	#else
	*m1	= C[sol->n-1].m1;
	*m2 = C[sol->n-1].m2;
	#endif
	
	if (ssmeth == SSMETH_STA)
		*rob	= -(*m1 + sqrt(*m2) * 1.96);
	else
		*rob	= -(*m1 + sqrt(*m2) * 1.96);

	return 0;
}


int	
srcs_solve(plan_t *plan, rvar_t *D, rcons_t *res,
			matr_t *due, int maxenum,
			int robtype, int ssmeth, 
			int numsim, plan_t *sol,
			solvecb_t *cb,
			int reseed)
{
	int err = 0, i, a1, a2;
	pair_t dist[plan->n];
	plan_t *bestsol = NULL;
	real_t rob, bestrob = -100000000, m1, m2, bestm1=-1, bestm2=-1;
	pair_t resrank[res->k];
	int r;
	solve_ctx_t solvectx = {0};
	bfsctx_t bfsctx = {0};
	int stop;
	int numbfskeep, maxbfskeep;
	
	/* allocate space for soldist */
	assert((solvectx.soldist = calloc(sizeof(pair_t), plan->n)));
	
	/* allocate space for cfgs */
 	assert((solvectx.cfg=calloc(sizeof(int*), res->k)));
	for (r=0; r < res->k; r++)
		assert((solvectx.cfg[r]=calloc(sizeof(int), res->cap[r])));
	
	/* allocate space for dmatr, s, c matrices */
	if (ssmeth == SSMETH_SIM) {
		assert((solvectx.dmatr = calloc(sizeof(real_t*), numsim)));
		for (i=0; i < numsim; i++) {
			assert((solvectx.dmatr[i] = calloc(sizeof(real_t), plan->n)));
			srcs_rvec_realize(D, plan->n, solvectx.dmatr[i]);
		}
		assert((solvectx.s = calloc(sizeof(matr_t), 1)));
		assert((solvectx.c = calloc(sizeof(matr_t), 1)));
		assert((solvectx.t = calloc(sizeof(matr_t), 1)));
		
		if ((err=srcs_matr_init(numsim, plan->n, solvectx.s)))
			goto cleanup;
		if ((err=srcs_matr_init(numsim, plan->n, solvectx.c)))
			goto cleanup;
		if ((err=srcs_matr_init(numsim, 1, solvectx.t)))
			goto cleanup;
	}
	
	#ifdef INC
	assert((solvectx.edirt = calloc(sizeof(char*), plan->n)));
	for (a1=0; a1 < plan->n; a1++)
		assert((solvectx.edirt[a1] = calloc(sizeof(char), plan->n)));
	#endif
	
	srcs_bfsctx_init(&bfsctx, plan->n);
	solvectx.bfsctx = &bfsctx;

	/* allocate & initialize `sol' and `bestsol' */
	if ((err=srcs_plan_init(plan->n, sol)))
		goto cleanup;
		
	if ((err=srcs_plan_copy(sol, plan)))
		goto cleanup;
			
	assert((bestsol=calloc(sizeof(plan_t), 1)));
	if ((err=srcs_plan_init(plan->n, bestsol)))
		goto cleanup;

	/* allocate `S, C' to be used by solve() */
	assert((solvectx.S = calloc(sizeof(rvar_t), sol->n)));
	assert((solvectx.C = calloc(sizeof(rvar_t), sol->n)));
	assert((solvectx.T = calloc(sizeof(rvar_t), 1)));
	
	for (i=0; i < plan->n; i++) {
		if ((err=srcs_rvar_init(D[0].k, &solvectx.S[i])))
			return err;
		if ((err=srcs_rvar_init(D[0].k, &solvectx.C[i])))
			return err;
	}
	if ((err=srcs_rvar_init(D[0].k, solvectx.T)))
		return err;	
	
	/* prepare initial resrank[] (to be shuffled) */
	for (i=0; i < res->k; i++) {
			resrank[i].x._int = i;
			resrank[i].y._int = rand();
	}
	
	if (reseed)
		srand(srcs_seed);
	
	stop = 0;
	numbfskeep = 0;
	maxbfskeep = MIN(maxenum, 10);
	for (i=0; (i < maxenum) && !stop ; i++) {
		/* prepare randomized `dist[]' */
		if (numbfskeep < maxbfskeep) {
			if ((err=srcs_plan_bfs(&bfsctx, plan, 0, dist, 1)))
				goto cleanup;
			numbfskeep++;
		}

		rob = 0;
		if((err=solve(&solvectx, dist, resrank, D, res, due, 
			robtype, ssmeth, numsim, sol, &rob, &m1, &m2)))
			goto cleanup;	
		
		if ( SOLVE_ALL || (rob > bestrob )) {
			srcs_plan_copy(bestsol, sol);
			bestrob = rob;
			bestm1 = m1;
			bestm2 = m2;
			if(cb && (err=cb->f(cb, i, maxenum, bestsol, bestrob, bestm1, bestm2)))
				goto cleanup;

		}
		
		/* cleanup and shuffle for next */
		for (a1=0; a1 < plan->n; a1++) 
			for (a2=0; a2 < plan->n; a2++)
				if (sol->edge[a1][a2] == 2)
					srcs_plan_set_edge(sol, a1, a2, 0);
					
		if (cb && *(cb->stop))
			stop = 1;
	}
	
	if (cb)
		cb->f(cb, i, -1, bestsol, bestrob, bestm1, bestm2);

cleanup:
	
	free(solvectx.soldist);
	
	for (r=0; r < res->k; r++)
		free(solvectx.cfg[r]);
	free(solvectx.cfg);

	for (i=0; i < plan->n; i++) {
		srcs_rvar_free(D[0].k, &solvectx.S[i]);
		srcs_rvar_free(D[0].k, &solvectx.C[i]);
 	}
	free(solvectx.S);
	free(solvectx.C);
	srcs_rvar_free(solvectx.T->k, solvectx.T);
	free(solvectx.T);

	if (ssmeth == SSMETH_SIM) {
		for (i=0; i < numsim; i++)
			free(solvectx.dmatr[i]);
		free(solvectx.dmatr); 

		if (solvectx.s)
			srcs_matr_free(solvectx.s);
		if (solvectx.c)
			srcs_matr_free(solvectx.c);
		if (solvectx.t)
			srcs_matr_free(solvectx.t);
		free(solvectx.s);
		free(solvectx.c);
		free(solvectx.t);
	}

	#ifdef INC
	for (a1=0; a1 < sol->n; a1++)
		free(solvectx.edirt[a1]);
	free(solvectx.edirt);
	#endif
	
	srcs_bfsctx_free(&bfsctx);

	if (bestsol) {
		srcs_plan_copy(sol, bestsol);
		srcs_plan_free(bestsol);
	}
	
	return err;
}
#endif

int	
srcs_rcons_init(int k, int n, rcons_t *res)
{
	int a;
	assert(res && k >= 0 && n >= 0);
	
	res->k = k;
	res->n = n;
	assert((res->cap=calloc(sizeof(int), res->k)));
	assert((res->req=calloc(sizeof(int*), res->n)));
	for (a=0; a < res->n; a++)
		assert((res->req[a]=calloc(sizeof(int), res->k)));
		
	return 0;
}

int
srcs_rcons_set_cap(rcons_t *res, int r, int cap)
{
	assert(res && r >= 0 && cap >= 0);
	
	if (r > res->k)
		return 1;
	
	res->cap[r] = cap;
		
	return 0;
}

int
srcs_rcons_set_req(rcons_t *res, int a, int r, int req)
{
	assert(res && r >= 0 && a >= 0 && req >= 0);
	
	
	if ((a > res->n) || (r > res->k) || (req > res->cap[r]))
		return 1;
	
	res->req[a][r] = req;
	
	return 0;
}

int
srcs_matr_init(int rows, int cols, matr_t *matr)
{
	int i;
	
	assert(matr);
	
	if (rows <= 0 || cols <= 0)
		return 1;
	
	matr->rows = rows;
	matr->cols = cols;
	assert((matr->cell=calloc(sizeof(cell_t*), matr->rows)));
	for (i=0; i < matr->rows; i++)
		assert((matr->cell[i]=calloc(sizeof(cell_t), matr->cols)));
	
	return 0;
}

int 
srcs_matr_expand(matr_t *dst, int rows)
{
	if (dst->rows >= rows)
		return 0;
		
	assert((dst->cell = realloc(dst->cell, sizeof(cell_t)*rows)));
	while(dst->rows < rows)
		assert((dst->cell[dst->rows++] = calloc(sizeof(cell_t), dst->cols)));
	
	return 0;
}

int 
srcs_matr_copy(matr_t *dst, matr_t *src)
{
	int r;
	assert(dst && src);
	assert(dst->cols >= src->cols && dst->rows >= src->rows);
	
	for (r=0;r < src->rows; r++)
		memmove(dst->cell[r], src->cell[r], sizeof(cell_t)*src->cols);
	
	return 0;
}

int
srcs_matr_addcol(matr_t *m1, matr_t *m2, int col, int type)
{
	int row;
 	
	for (row=0; row < MIN(m1->rows, m2->rows); row++) {
		if (type == 1)
			m1->cell[row][col]._int += m2->cell[row][col]._int;
		if (type == 2)
			m1->cell[row][col]._real += m2->cell[row][col]._real;
	}

	return 0;	
}

int
srcs_matr_add(matr_t *m1, matr_t *m2, int type)
{
	int col;
	
	for (col=0; col < MIN(m1->cols, m2->cols); col++)
		(void)srcs_matr_addcol(m1, m2, col, type);
		
	return 0;
}

int
srcs_matr_m1(real_t *m1, matr_t *matr, int simnum, int col, int type_int)
{
	int row;
	assert(m1 && matr);
 	
	if (!(simnum <= matr->rows && col <= matr->cols))
		return 1;
	
	*m1 = 0;
	for (row=0; row < simnum; row++)
		*m1 += (type_int == 0)? matr->cell[row][col]._int: matr->cell[row][col]._real;
	*m1 = *m1 / (real_t)simnum;
	
	return 0;
}

int 
srcs_matr_m2(real_t *m2, matr_t *m, int simnum, int col, real_t m1, int type_int)
{
	int sim;
	real_t sum = 0;
	
	for (sim=0; sim < simnum; sim++) {
		if (type_int == 0)
			sum += pow((m->cell[sim][col]._int - m1), 2);
		else
			sum += pow((m->cell[sim][col]._real - m1), 2);
	}
	
	*m2 = sum / simnum;
	
	return 0;
}

int
srcs_mcarlo_est_j(matr_t *s, matr_t *d, plan_t *es, int simnum, int j)
{
	int sim, i;
	real_t t;
	
	assert(s->rows >= simnum && d->rows >= simnum && s->cols >= d->cols && s->cols >= es->n);
	assert(j < es->n);
	
	for (sim=0; sim < simnum; sim++)
	{
		t = 0;
		for (i=0; i < es->n; i++)
			if (es->edge[i][j])
				t = MAX(t, s->cell[sim][i]._real + d->cell[sim][i]._real);
		
		s->cell[sim][j]._real = t;
	}
	
	return 0;
}

int
srcs_mcarlo_eft_j(matr_t *f, matr_t *s, matr_t *d, int simnum, int j)
{
	int sim;
	
	assert(f->rows == s->rows && f->cols == s->cols);
	assert(s->rows >= simnum && d->rows >= simnum && s->cols >= d->cols);
	assert(j < s->cols);
	
	for (sim=0; sim < simnum; sim++)
		f->cell[sim][j]._real = s->cell[sim][j]._real + d->cell[sim][j]._real;
	
	return 0;
}

/* mean absolute deviation */
int
srcs_matr_coldisp_mad(real_t *disp, matr_t *matr, int j, real_t v)
{
	int row;
	real_t matr_v;
	
	*disp = 0;
	for (row=0; row < matr->rows; row++) {
		matr_v = matr->cell[row][j]._real;
		*disp += ABS(matr_v - v);
	} 
	*disp /= (real_t)matr->rows;
	
	return 0;
}

/* mean-squared error */
int
srcs_matr_coldisp_mse(real_t *disp, matr_t *matr, int j, real_t v)
{
	int row;
	real_t matr_v;
	
	*disp = 0;
	for (row=0; row < matr->rows; row++) {
		matr_v = matr->cell[row][j]._real;
		*disp += pow(matr_v - v, 2);
	}
	*disp /= (real_t)matr->rows;
	
	return 0;
}

void
srcs_matr_free(matr_t *matr)
{
	int i;
	
	assert(matr);

	for (i=0; i < matr->rows; i++)
		free(matr->cell[i]);
	
	free(matr->cell);
}

int	
srcs_rvec_sim(matr_t *sim, int numsim, int n, rvar_t *V)
{
	int i, a1;
		
	assert(V && sim && sim->rows >= numsim);

	for (a1=0; a1 < n; a1++) {
		V[a1].m1 = 0;
		for (i=0; i < numsim; i++)
			V[a1].m1 += sim->cell[i][a1]._real;
	}
	
	for (a1=0; a1 < n; a1++) {
		/* compute m1 */
		V[a1].m1 /= (real_t)numsim;
		
		/* compute m2 */
		V[a1].m2 = 0;
		for (i=0; i < numsim; i++)
			V[a1].m2 += (real_t)pow((sim->cell[i][a1]._real - V[a1].m1), 2);
	
		V[a1].m2 /= (real_t)numsim;
	}
	
	return 0;
}

static int
event_compar(const void *event1ptr, const void *event2ptr)
{
	pair_t *event1 = (pair_t*)event1ptr;
	pair_t *event2 = (pair_t*)event2ptr;

	real_t dt = event1->x._real - event2->x._real;
	
	if (dt < 0)
		return -1;
	else if (dt > 0)
		return 1;
	return 0;
}

int	
srcs_resprof(int r, rcons_t *res, matr_t *prof,
	matr_t *sim_s, matr_t *sim_c, int numsim, int n)
{
	int i, a1, j, m, use;
	pair_t event[2*n];
	int eventindx;
	real_t t;
	
	/* validate input */
	assert(res && prof && sim_s && sim_c);
	
	/* order all start/completion events in use[] */
	for (i=0; i < numsim; i++) {
		eventindx = 0;
		for (a1=0; a1 < n; a1++) {
			if (res->req[a1][r] == 0)
				continue;
			event[eventindx].x._real = sim_s->cell[i][a1]._real;
			event[eventindx++].y._int = res->req[a1][r];
			
			event[eventindx].x._real = sim_c->cell[i][a1]._real;
			event[eventindx++].y._int = -res->req[a1][r];
		}
		qsort(event, eventindx, sizeof(pair_t), event_compar);
		
		/* construct r's profile for the i-th simulation */
		use = 0;
		for (j=0; j < eventindx; j++) {
			t=event[j].x._real;
			m=j;
			while (1) {
				use += event[j].y._int;
				if ((event[j+1].x._real != t) || (j+1 == eventindx))
					break;
				j++;
			}
			while (m <= j) {
				prof->cell[2*i][m]._real = t;
				prof->cell[2*i+1][m]._int = use;
				m++;
			}
		}
		if (eventindx < 2*n)
			prof->cell[2*i+1][eventindx]._int = -1;
	}
	
	return 0;
}

void
srcs_setseed(int seed)
{
	srcs_seed = seed;
}

void
srcs_srand(int seed)
{
	srand((srcs_seed = seed));
}

int
srcs_mcarlo_crt(st, pt, d, numsim, plan, sort, sink, crt, ecrt)
	matr_t *st, *pt, *d, *crt, *ecrt, *sort;
	plan_t *plan;
	int numsim, sink;
{
	int n = MIN(st->cols, d->cols);
	int i, j, k, a1, a2;
	real_t a1ct, a2st, a2pt;
	int a1crt, a2crt;
	
	assert(st && d && crt && sort);
	assert((MIN(st->rows, d->rows) >= numsim) && (sort->cols >= n));
 	if (ecrt)
		assert(ecrt->cols >= (st->cols*st->cols));

 	/* initialize crticalities */
	for (k=0; k < numsim; k++) {
		memset(crt->cell[k], 0, sizeof(cell_t) * n);
		crt->cell[k][sink]._real = 1;
	}
	
	/* find sink */
	for (i=n-1; i >= 0; i--) {
		a1 = sort->cell[0][i]._int;
		
		if (a1 == sink)
			break;
	}

	/* now i=sink's sort position */
	for (i=i-1; i >= 0; i--) {
		a1 = sort->cell[0][i]._int;
		
		for (j=i+1; j < n; j++) {
			a2 = sort->cell[0][j]._int;
			
			if (plan->edge[a1][a2]) {	

				for (k=0; k < numsim; k++) {
					a1ct = st->cell[k][a1]._real + d->cell[k][a1]._real;
					a2st = st->cell[k][a2]._real;
					a2pt = pt->cell[0][a2]._real;
					a2crt = (crt->cell[k][a2]._real != 0);
					
					a1crt = (a2crt && (a1ct == a2st) && (a2st > a2pt));
					
					if (a1crt) {
						crt->cell[k][a1]._real = 1.0;
						if (ecrt)
							ecrt->cell[k][a1*n+a2]._real = 1.0;
					}
				}
			}
		}
	}
	
	return 0;
}


static int
ft_cmp(int i, int *s, int m, matr_t *cmp)
{
	int l;
	
	for (l=0; l < m; l++)
 		if (!cmp->cell[i][s[l]]._int)
			return 0;

	return 1;
}


static int
ftstatus(matr_t *ft, int *numfs, int *s, int m, matr_t *q, matr_t *cp, 
		int ***mfslist, int *mfslistlen)
{
	int l, r, k = cp->cols;
	int qs[k];
	int forb;
	int minimal;
	
	forb = 0;
	for (r=0; r < k; r++) { 
		for (qs[r]=0, l=0; l < m; l++)
			qs[r] += q->cell[s[l]][r]._int;
		
		if (qs[r] > cp->cell[0][r]._int)
			forb = 1;
	}
	
	minimal = 0;
	if (forb) {
		minimal = 1;
		for (l=0; l < m; l++) {
			for (r=0; r < k; r++) {
				if (qs[r]-q->cell[s[l]][r]._int <= cp->cell[0][r]._int)
					continue;
				minimal = 0;
				break;
			}
		}
	}
	
	if (minimal) {
  		/* add to mfslist */
 		*mfslistlen = *mfslistlen + 1;
 		*mfslist = realloc(*mfslist, *mfslistlen * sizeof(int*));
 		assert(*mfslist != NULL);
 		
 		(*mfslist)[*mfslistlen-1] = calloc(sizeof(int), m+1);
 		assert((*mfslist)[*mfslistlen-1] != NULL);
 		
 		(*mfslist)[*mfslistlen-1][0] = m;
 		for (l=0; l < m; l++)
   			(*mfslist)[*mfslistlen-1][l+1] = s[l];
   			
  		return 0;
  	}
	
	return 1;
}

static void
ftsearch(matr_t *ft, int *numfs,
	int i, int *s, int m, matr_t *q, matr_t *cp, matr_t *cmp,
	int ***mfslist, int *mfslistlen)
{
	int j;
	int n = q->rows;
   	
	if (i == n)
		return;
	
	if (!ft_cmp(i, s, m, cmp))
		return;

	s[m++] = i;
		
	if (ftstatus(ft, numfs, s, m, q, cp, mfslist, mfslistlen))
		for (j=i+1; j < n; j++)
			ftsearch(ft, numfs, j, s, m, q, cp, cmp, mfslist, mfslistlen);
}

int
srcs_ft(matr_t *q, matr_t *cp, matr_t *cmp, matr_t *ft, 
		int ***mfslist, int *mfslistlen)
{
	int n = q->rows;
	int s[n];
	int i, numft;
	
	*mfslist = NULL;
	*mfslistlen = 0;
	
	for (i=1; i < n; i++)
		ftsearch(NULL, &numft, i, s, 0, q, cp, cmp, mfslist, mfslistlen);
	
	return 0;
}

static int
toporder_visit(matr_t *order, int *ordersiz, int j, int *mark, plan_t *plan)
{
	int i;
	
	if (mark[j] == 1)
		return -1;
	
	if (mark[j] == 0) {
		mark[j] = 1;
		
		for (i=0; i < plan->n; i++) {
			if (plan->edge[i][j]) {
				if (toporder_visit(order, ordersiz, i, mark, plan) < 0)
					return -1;
			}
 		}
 		
 		mark[j] = 2;
 		order->cell[0][(*ordersiz)++]._int = j;
	}

	return 0;
}

static int
toporder(matr_t *order, plan_t *plan)
{
	fifo_t f;
	int mark[plan->n];
 	int j;
 	int ordersiz;
	
	/* push unmarked nodes */
	fifo_init(&f);
	for (j=0; j < plan->n; j++)
		fifo_push(&f, j);
	memset(mark, 0, sizeof(mark));
	ordersiz = 0;
	
	while(!fifo_empty(&f)) {
		j = fifo_pop(&f);
		if (toporder_visit(order, &ordersiz, j, mark, plan) < 0) {
			fifo_free(&f);
			return -1;
		}
	}
	fifo_free(&f);
	return ordersiz;
}

int 
srcs_plan_order(matr_t *order, plan_t *plan, int *len)
{
	int l = toporder(order, plan);
	if (len)
		*len = l;
	
	if (order->rows > 1) {
		int i, j, ordi, ordj;
 		for (ordj=0; ordj < plan->n; ordj++) {
			j = order->cell[0][ordj]._int;
			
			order->cell[1][j]._int = 0;
			
			for (ordi=0; ordi < ordj; ordi++) {
				i = order->cell[0][ordi]._int;
				
				if (!plan->edge[i][j])
					continue;
				
				order->cell[1][j]._int = 
				MAX(order->cell[1][j]._int, order->cell[1][i]._int+1);
			}
 		}
 	}
	
	return 0;
}


int
srcs_mcarlo_est(matr_t *est, matr_t *d, matr_t *s, plan_t *plan, matr_t *order, int simnum)
{
	int i, j, m, ordi, ordj;
	real_t eft;
	
	if (simnum > d->rows)
		return 1;
	
 	for (m=0; m < simnum; m++) {
	
 		for (ordj=0; ordj < plan->n; ordj++) {
			j = order->cell[0][ordj]._int;
			est->cell[m][j]._real = 0;
			
			for (ordi=0; ordi < ordj; ordi++) {
				i = order->cell[0][ordi]._int;
				
				if (!plan->edge[i][j])
					continue;
				
				eft = est->cell[m][i]._real + d->cell[m][i]._real;
 				
				est->cell[m][j]._real = MAX(est->cell[m][j]._real, eft);
			}
			est->cell[m][j]._real = MAX(est->cell[m][j]._real, s->cell[0][j]._real);
 		}
 	}
	
	return 0;
}

#define MSPERDAY (24*60*60*1000)
#define DAYS2MS(x) ((x) * MSPERDAY)
#define MS2DAYS(x) ((x) / MSPERDAY)

static real_t weekend_expand(real_t s, real_t d)
{
	int k;

	for (k=1; (s + d) > 5.0 + (k-1)*7.0; k++) 
		d += 2;

	return d;
}

int
srcs_mcarlo_est_nwd(matr_t *est, matr_t *d, matr_t *t, matr_t *tday, plan_t *plan, matr_t *order, int simnum)
{
	int i, j, m, ordi, ordj;
	real_t sj, sdayj, tj, tdayj, dj;
	real_t f[est->cols], fday[est->cols];
	
	if (simnum > d->rows)
		return 1;
	
 	for (m=0; m < simnum; m++) {
 		for (ordj=0; ordj < plan->n; ordj++) {
			j = order->cell[0][ordj]._int;
			tj = t->cell[0][j]._real;
			tdayj = tday->cell[0][j]._real;
			est->cell[m][j]._real = 0;
			dj = d->cell[m][j]._real;
			
			/* compute sj */
			sj = tj;
			sdayj = tdayj;
			
			for (ordi=0; ordi < ordj; ordi++) {
				i = order->cell[0][ordi]._int;
				
				if (plan->edge[i][j] && f[i] > sj) {
					sj = f[i];
					sdayj = fday[i];
				}
			}
			est->cell[m][j]._real = sj;

			/* revisit dj */
			dj = DAYS2MS(weekend_expand(sdayj, MS2DAYS(dj)));
			d->cell[m][j]._real = dj;

			f[j] = sj + dj;
			fday[j] = fmodf(sdayj + MS2DAYS(dj), 7.0);

			/*
			fprintf(stderr, "sday[%d][%d] %.1f d[%d][%d] %.1f fday[%d][%d] %.1f\n",
					m, j, sdayj, 
					m, j, MS2DAYS(dj),
					m, j, fday[j]);
			*/
 		}
 	}
	
	return 0;
}

int
srcs_mcarlo_lst(matr_t *ls, matr_t *d, plan_t *plan, matr_t *order, real_t ub, int simnum)
{
	int i, j, ordi, ordj;
	real_t min_suc_lst;
	int sim;
	
	for (sim=0; sim < simnum; sim++) {
	 	for (ordi = plan->n-1; ordi >= 0; ordi--) {
			i = order->cell[0][ordi]._int;
			min_suc_lst = ub;
			
			for (ordj = ordi+1; ordj < plan->n; ordj++) {
				j = order->cell[0][ordj]._int;
				
				if (!plan->edge[i][j])
					continue;
					
	 			min_suc_lst = MIN(min_suc_lst, ls->cell[sim][j]._real);
			}
			
			ls->cell[sim][i]._real = min_suc_lst - d->cell[sim][i]._real;
		}
	}
	
	return 0;
}

int
srcs_mcarlo_lft(matr_t *lf, matr_t *d, plan_t *plan, matr_t *order, real_t ub, int simnum)
{
	int err;
	
	if ((err=srcs_mcarlo_lst(lf, d, plan, order, ub, simnum)))
		return err;
	
	assert(!srcs_matr_add(lf, d, 2));
	
	return 0;
}

int 
srcs_mcarlo_m1(real_t *m1, matr_t *v, int rownum, int col)
{
	int row;
	real_t sum = 0;
	
 	for (row=0; row < rownum; row++)
		sum += v->cell[row][col]._real;
 	
	*m1 = sum / rownum;
	
	return 0;
}

int 
srcs_mcarlo_m2(real_t *m2, matr_t *v, int rownum, int col, real_t m1)
{
	int row;
	real_t sum = 0;
	
	for (row=0; row < rownum; row++)
		sum += pow((v->cell[row][col]._real - m1), 2);
	
	*m2 = sum / rownum;
	
	return 0;
}

/* augment tc with (s,t) and store in fifo f the implied edges.
 * a node j inherits all predecessors from all of its 
 * immedediate predecessors:
 *  H <- E
 *  for each (i,j) in H:
 *   for each e in {(i',j) | (i',i) in H}:
 *    if e not in E':
 *     add e to f
 *     add e to E' */
 
int
srcs_plan_tc_update(plan_t *tc, int s, int t, int v, matr_t *order, fifo_t *f)
{
	int l, i, j, orderl, orderi, orderj;
	int findj;
	
	srcs_plan_set_edge(tc, s, t, v);
	if (f) {
		fifo_push(f, s);
		fifo_push(f, t);
	}
	findj = 1;
	
	for (orderj=0; orderj < tc->n; orderj++) {
		j = order->cell[0][orderj]._int;
		
		if (findj && j != t)
			continue;
		//printf("found %d\n", t);
		findj = 0;
			
		for (orderi=0; orderi < orderj; orderi++) {
			i = order->cell[0][orderi]._int;
			
			if (tc->edge[i][j]) {
				//printf(" (%d,%d) implies: ", i, j);
				for (orderl=0; orderl < orderi; orderl++) {
					l = order->cell[0][orderl]._int;
					
					if (tc->edge[l][i] && !tc->edge[l][j]) {
						if (f) {
							fifo_push(f, l);
							fifo_push(f, j);
						}
						tc->edge[l][j] = v;
					}
				}
				//puts("");
			}
		}
	}
	
	return 0;
}

/* makespan expectation & +entile */

static int
realcmp(const void *p1ptr, const void *p2ptr)
{
	real_t *r1 = (real_t*)p1ptr;
	real_t *r2 = (real_t*)p2ptr;
	
	if (*r1 == *r2)
		return 0;
	if (*r1 > *r2)
		return 1;
	
	return -1;
}


real_t 
srcs_makespan(matr_t *s, int simnum)
{
 	real_t mkm1;
 	//real_t mkm2;
 	
 	srcs_mcarlo_m1(&mkm1, s, simnum, s->cols-1);
	//srcs_mcarlo_m2(&mkm2, s, simnum, s->cols-1, mkm1);
	
 	return mkm1;	
}

real_t 
srcs_makespan_i(matr_t *s, int simnum, int i)
{
 	real_t mkm1;
 	//real_t mkm2;
 	
 	srcs_mcarlo_m1(&mkm1, s, simnum, i);
	//srcs_mcarlo_m2(&mkm2, s, simnum, i, mkm1);
	
 	return mkm1;	
}

static int
array_p(real_t *array, int len, real_t p, real_t *res)
{
 	qsort(array, len, sizeof(real_t), realcmp);
	*res = gsl_stats_quantile_from_sorted_data(array, 1, len, p);
	
	return 0;
}

/* srcs_matr_p_i():
 * extrapolate the p-th percentile of the i-th column of mat[] */

int
srcs_matr_p_j(matr_t *mat, int numrows, int j, real_t p, real_t *res)
{
	real_t values[numrows];
	int row;
	
	assert(mat->rows >= numrows && numrows >= 0);
	assert(mat->cols >= j && j >= 0);
	
	if (p < 0 || p > 1.0)
 		return 1;
	
	for (row=0; row < numrows; row++)
		values[row] = mat->cell[row][j]._real;
	
 	(void)array_p(values, numrows, p, res);
	
	return 0;
}

real_t
srcs_makespan_p(matr_t *s, int simnum, real_t P)
{
	real_t sort[simnum];
	int m;
	int rank;
			
	printf("makespan: ");
 	for (m=0; m < simnum; m++) {
		sort[m] = s->cell[m][s->cols-1]._real;
 		if (m <= 10)
 			printf("%.1f ", sort[m]);		
	}
 	qsort(sort, simnum, sizeof(real_t), realcmp);
	rank = (int)(P*simnum + 0.5);
	printf(".. sort[%d]: %.1f\n", rank, sort[rank]);
 	return sort[rank];
}

real_t
srcs_makespan_p_i(matr_t *s, int simnum, real_t P, int i)
{
	real_t sort[simnum];
	int m;
	int rank;
	
	//return srcs_makespan_i(s, simnum, i);
		
  	for (m=0; m < simnum; m++) {
		sort[m] = s->cell[m][i]._real;		
	}
 	qsort(sort, simnum, sizeof(real_t), realcmp);
	rank = (int)(P*simnum + 0.5);
  	return sort[rank];
}
