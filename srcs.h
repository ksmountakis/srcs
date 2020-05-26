#ifndef _SRCS_H
#define _SRCS_H
#include <float.h>
#include "fifo.h"

#define SSMETH_SIM 0
#define SSMETH_STA 1
#define SSMETH_SCU 2

#define ROB_BETA 0
#define ROB_ALPHA 1
#define MAX(a, b) ((a > b)? (a): (b))
#define MIN(a, b) ((a < b)? (a): (b))
#define NORM_975_QUANTILE(m1, m2) (m1 + 1.96 * sqrt(m2))
#define ABS(x) MAX(x, -(x));

typedef enum {MK_MCARLO, MK_PWL, MK_NONE} mkmeth_t;
typedef enum {CW_TRQ, CW_TRQ_NUT, CW_CMX, CW_SGS, CW_TRQ_CMX, CW_TRQ_EXP} cw_style_t;

typedef double real_t;
#define REAL_MAX DBL_MAX

typedef union cell {
	void *_ptr;
	int _int;
	real_t _real;
}cell_t;

typedef struct matr {
	int rows, cols;
	cell_t **cell;
}matr_t;

typedef struct rvar {
	real_t m1, m2;
	double m2real;
	real_t *cov;
	int k;
}rvar_t;

typedef struct plan {
	int n;
	int **edge;
	int *dirt;
	int *numpre;
}plan_t;

typedef struct rcons {
	int k, n;
	int **req;
	int *cap;
}rcons_t;

typedef struct pair {
	union {
		int _int;
		void *_ptr;
		real_t _real;
	}x, y;
}pair_t;

typedef struct solvecb {
	int (*f)(struct solvecb *, int , int, plan_t *, real_t, real_t, real_t);
	int argc;
	void **argv;
	int *stop;
}solvecb_t;


typedef struct bfsctx {
	int *fifo;
	int *discov;
	int *numipre;
} bfsctx_t;

real_t srcs_rob(rvar_t *C, matr_t *due, int n, int type);

int	srcs_simulate(plan_t *plan, pair_t *dist,
				int numsim, matr_t *d, matr_t *s, matr_t *c);

#if 0
int	srcs_solve(plan_t *plan, rvar_t *dur, rcons_t *res,
	matr_t *due, int maxenum,
	int robtype, int ssmeth, 
	int numsim, plan_t *sol,
	solvecb_t *cb, int reseed);
#endif

int	srcs_plan_init(int n, plan_t *plan);

int	srcs_plan_copy(plan_t *dst, plan_t *src);

void srcs_plan_free(plan_t *plan);

int	srcs_plan_set_edge(plan_t *plan, int a1, int a2, int e);

int	srcs_plan_dfs(plan_t *plan, int a1, pair_t *dist);

void srcs_bfsctx_init(bfsctx_t *ctx, int n);

void srcs_bfsctx_free(bfsctx_t *ctx);

int	srcs_plan_bfs(bfsctx_t *ctx, plan_t *plan, 
	int a1, pair_t *dist, int rand);
	
int	srcs_plan_bfs2(plan_t *plan, int a1, matr_t *bfs);
	

int	srcs_rvar_init(int k, rvar_t *X);

void srcs_rvar_free(int k, rvar_t *X);

int srcs_rvec_sim(matr_t *sim, int numsim, int n, rvar_t *V);

void srcs_rvec_realize(rvar_t *V, int n, real_t *v);

	
int	srcs_rvar_set_cov(rvar_t *V, int j, real_t cov);

int	srcs_lgsta(plan_t *plan, pair_t *dist, 
	rvar_t *D, rvar_t *S, rvar_t *C);

int	srcs_rcons_init(int k, int n, rcons_t *res);

int	srcs_rcons_set_cap(rcons_t *res, int r, int cap);

int	srcs_rcons_set_req(rcons_t *res, int a, 
	int r, int req);

int	srcs_matr_init(int rows, int cols, matr_t *matr);
int srcs_matr_expand(matr_t *dst, int rows);
int srcs_matr_copy(matr_t *dst, matr_t *src);
int srcs_matr_addcol(matr_t *m1, matr_t *m2, int i, int type);
int srcs_matr_remcol(matr_t *m1, matr_t *m2, int i, int type);
int	srcs_matr_add(matr_t *m1, matr_t *m2, int type);
int	srcs_matr_m1(real_t *m1, matr_t *matr, int simnum, int col, int type_int);
int	srcs_matr_m2(real_t *m2, matr_t *m, int simnum, int col, real_t m1, int type_int);
int srcs_matr_p_j(matr_t *mat, int numrows, int j, real_t p, real_t *result);
int srcs_matr_coldisp_mad(real_t *disp, matr_t *matr, int j, real_t v);
int srcs_matr_coldisp_mse(real_t *disp, matr_t *matr, int j, real_t v);
int	srcs_matr_avg_row(matr_t *rowmat, matr_t *mat, int simnum);
void srcs_matr_free(matr_t *matr);

int	srcs_resprof(int r, rcons_t *res, matr_t *prof,
	matr_t *sim_s, matr_t *sim_c, int numsim, int n);
			
void srcs_srand(int seed);

int	srcs_lgsta_tard(rvar_t *D, matr_t *due,  rvar_t *T);

int	srcs_simulate_tard(matr_t *c, int numsim, 
	matr_t *due,  matr_t *t);

int srcs_mcarlo_crt(matr_t *st, matr_t *pt, matr_t *d, int numsim, 
	plan_t *plan, matr_t *bfs, int sink, matr_t *crit, matr_t *ecrit);
	

int	srcs_iflat(plan_t *plan, rvar_t *dur, rcons_t *res, matr_t *due, 
	int maxfail, int maxrel, real_t prem,
	int robtype, int ssmeth,
	rvar_t *S, rvar_t *C,
	int numsim, matr_t *dmatr, matr_t *smatr, matr_t *cmatr,
	plan_t *sol, solvecb_t *cb, int reseed);

int srcs_plan_tc(plan_t *tc, plan_t *plan, matr_t *bfs);

int srcs_ft(matr_t *q, matr_t *cp, matr_t *comp, matr_t *ft, int ***mfslistptr, int *mfslistlenptr);
	
int srcs_plan_order(matr_t *order, plan_t *plan, int *len);

int	srcs_plan_tc_update(plan_t *tc, int s, int t, int v, matr_t *order, fifo_t *f);

int	srcs_mcarlo_est(matr_t *est, matr_t *d, matr_t *s, plan_t *plan, matr_t *order, int simnum);
int	srcs_mcarlo_est_nwd(matr_t *est, matr_t *d, matr_t *s, matr_t *sday, plan_t *plan, matr_t *order, int simnum);

int	srcs_mcarlo_lst(matr_t *ls, matr_t *d, plan_t *plan, matr_t *order, real_t ub, int simnum);

int	srcs_mcarlo_lft(matr_t *lf, matr_t *d, plan_t *plan, matr_t *order, real_t ub, int simnum);

int srcs_mcarlo_m1(real_t *m1, matr_t *v, int rownum, int col);

int srcs_mcarlo_m2(real_t *m2, matr_t *v, int rownum, int col, real_t m1);

int	srcs_mfssearch(plan_t *best, matr_t *sbs, matr_t *cvr, matr_t *map,
	matr_t *mfs, plan_t *plan, matr_t *d, matr_t *D, 
	int simnum, int simmax, mkmeth_t mkmeth, real_t *mk, int *sec);
	
int srcs_pwl_est(matr_t *S, real_t *p95, plan_t *plan, matr_t *order, matr_t *D);

int	srcs_chaining(plan_t *sol, matr_t *sch, plan_t *plan, matr_t *cp, 
			  	  matr_t *rq, matr_t *d, matr_t *s, int simnum, int simmax, 
			  	  int method, real_t *mk, int *mcarlonum);
			  	  
int	srcs_tsc(plan_t *sol, matr_t *sch, plan_t *plan, matr_t *cp, 
			  	  matr_t *rq, matr_t *d, matr_t *s, int simnum, int simmax, 
			  	  int method, real_t *mk);	
			  	  		  	  
real_t	srcs_makespan(matr_t *s, int simnum);
real_t	srcs_makespan_i(matr_t *s, int simnum, int i);
 
real_t	srcs_makespan_p(matr_t *s, int simnum, real_t P);
real_t	srcs_makespan_p_i(matr_t *s, int simnum, real_t P, int i);

int srcs_sgs_serial(matr_t *s, plan_t *p, matr_t *d, matr_t *q, matr_t *b, matr_t *eseq);
int srcs_sgs_cutwalk0(plan_t *s, plan_t *p, matr_t *d, int simnum, matr_t *q, matr_t *b, matr_t *eseq, real_t dt);
int srcs_sgs_cutwalk1(plan_t *s, plan_t *p, matr_t *d, int simnum, matr_t *q, matr_t *b, matr_t *eseq, real_t dt, cw_style_t);
int srcs_sgs_cutwalk_rho(plan_t *s, plan_t *p, matr_t *d, int simnum, matr_t *q, matr_t *b, matr_t *eseq);

#endif
