#include <stdio.h>
#include <stdlib.h>
#include <tcl/tcl.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include "fifo.h"
#include "srcs.h"

Tcl_HashTable 			ptrtab;
static int				ptrtab_get(Tcl_Obj *objkey, void **val);
static Tcl_HashEntry*	ptrtab_set(Tcl_Obj *objkey, void *val);
static int				ptrtab_del(Tcl_Obj *objkey);


static int
C_plan_new_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	int n;
	
	 	
 	if (objc != 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
	/* objv[1]: plan */
	if (ptrtab_get( objv[1], (void**)&plan)) {
		result = Tcl_NewStringObj("already exists: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: n */
	if (Tcl_GetIntFromObj(ip, objv[2], &n) != TCL_OK) {
		return TCL_ERROR;
	}
	
	assert((plan = calloc(sizeof(plan_t), 1)));
	if (srcs_plan_init(n, plan)) {
		result = Tcl_NewStringObj("srcs_plan_init failed ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		free(plan);
		return TCL_ERROR;
	}
 	
	(void)ptrtab_set(objv[1], plan);
	return TCL_OK;
}


static int
C_plan_set_edge_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	int a1, a2, e;
	
	if (objc != 5) {
		result = Tcl_NewStringObj("wrong num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[1]: plan */
	if (!ptrtab_get(objv[1], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2], objv[3]: a1, a2 */
	if (Tcl_GetIntFromObj(ip, objv[2], &a1) != TCL_OK)
		return TCL_ERROR;
	if (Tcl_GetIntFromObj(ip, objv[3], &a2) != TCL_OK)
		return TCL_ERROR;
		
	/* objv[4]: value */
	if (Tcl_GetIntFromObj(ip, objv[4], &e) != TCL_OK)
		return TCL_ERROR;
		
	if (srcs_plan_set_edge(plan, a1, a2, e)) {
		result = Tcl_NewStringObj("plan_set_edge failed: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_AppendStringsToObj(result, ", ", NULL);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

static int
C_plan_get_edge_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	int a1, a2;
	
	if (objc != 4) {
		result = Tcl_NewStringObj("wrong num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[1]: plan */
	if (!ptrtab_get(objv[1], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2], objv[3]: a1, a2 */
	if (Tcl_GetIntFromObj(ip, objv[2], &a1) != TCL_OK)
		return TCL_ERROR;
	if (Tcl_GetIntFromObj(ip, objv[3], &a2) != TCL_OK)
		return TCL_ERROR;
	
	if (a1 < 0 || a2 < 0 || a1 >= plan->n || a2 >= plan->n)	{
		result = Tcl_NewStringObj("invalid params", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewIntObj(plan->edge[a1][a2]);
	Tcl_SetObjResult(ip, result);
	
	return TCL_OK;
}

static int
C_plan_get_n_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	
	if (objc != 2) {
		result = Tcl_NewStringObj("wrong num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[1]: plan */
	if (!ptrtab_get(objv[1], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewIntObj(plan->n);
	Tcl_SetObjResult(ip, result);
	
	return TCL_OK;
}

static int
C_plan_copy_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *dstplan, *srcplan;
	
	if (objc != 3) {
		result = Tcl_NewStringObj("wrong num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[1]: plan1 */
	if (!ptrtab_get(objv[1], (void**)&dstplan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: plan2 */
	if (!ptrtab_get(objv[2], (void**)&srcplan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_plan_copy(dstplan, srcplan)) {
		result = Tcl_NewStringObj("srcs_plan_copy failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}


static int
C_vec_new_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	real_t *vec;
	int n;
	
	/* objv[1]: vector */
	if (ptrtab_get(objv[1], (void**)&vec)) {
		result = Tcl_NewStringObj("already exists: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: n */
	if ((Tcl_GetIntFromObj(ip, objv[2], &n) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid n: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	assert((vec = calloc(sizeof(real_t), 1)));
	/* todo: compose resource constraints */
	
	(void)ptrtab_set(objv[1], vec);

	return TCL_OK;
}



int
C_rcons_new_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	rcons_t *res;
	int k, n;
 	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: resource constraints */
	if (ptrtab_get( objv[1], (void**)&res)) {
		result = Tcl_NewStringObj("already exists: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: k */
	if ((Tcl_GetIntFromObj(ip, objv[2], &k) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid k: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: n */
	if ((Tcl_GetIntFromObj(ip, objv[3], &n) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid n: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	assert((res=calloc(sizeof(rcons_t), 1)));
	if (srcs_rcons_init(k, n, res)) {
		result = Tcl_NewStringObj("srcs_rcons_new failed ", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	(void)ptrtab_set(objv[1], res);
	
	return TCL_OK;
}

int
C_rcons_set_cap_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	rcons_t *res;
	int r, cap;
 	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: resource constraints */
	if (!ptrtab_get( objv[1], (void**)&res)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: r */
	if ((Tcl_GetIntFromObj(ip, objv[2], &r) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid r: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: cap */
	if ((Tcl_GetIntFromObj(ip, objv[3], &cap) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid cap: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (srcs_rcons_set_cap(res, r, cap)) {
		result = Tcl_NewStringObj("srcs_rcons_set_cap failed ", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	
	return TCL_OK;
}

int
C_rcons_set_req_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	rcons_t *res;
	int a, r, req;
 	
 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: resource constraints */
	if (!ptrtab_get( objv[1], (void**)&res)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: a */
	if ((Tcl_GetIntFromObj(ip, objv[2], &a) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid a: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: r */
	if ((Tcl_GetIntFromObj(ip, objv[3], &r) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid r: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[4]: req */
	if ((Tcl_GetIntFromObj(ip, objv[4], &req) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid req: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (srcs_rcons_set_req(res, a, r, req)) {
		result = Tcl_NewStringObj("srcs_rcons_set_req failed ", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_matr_new_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	int rows, cols;
 	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("already exists: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: rows */
	if ((Tcl_GetIntFromObj(ip, objv[2], &rows) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid rows: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: cols */
	if ((Tcl_GetIntFromObj(ip, objv[3], &cols) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid cols: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	assert((matr=calloc(sizeof(matr_t), 1)));
	if (srcs_matr_init(rows, cols, matr)) {
		result = Tcl_NewStringObj("srcs_matr_init failed ", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	(void)ptrtab_set(objv[1], matr);

	return TCL_OK;
}

int
C_matr_free_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
 	
 	if (objc != 2) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (matr == NULL)
		fprintf(stderr, "srcs::c_matr_free: matr = NULL\n");
	else
		srcs_matr_free(matr);
		
	Tcl_SetObjResult(ip, Tcl_NewIntObj(matr->rows));

	return TCL_OK;
}
int
C_matr_get_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	int row, col;
	char *type;
 	
 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: row */
	if ((Tcl_GetIntFromObj(ip, objv[2], &row) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid row: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: col */
	if ((Tcl_GetIntFromObj(ip, objv[3], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	assert((type = Tcl_GetString(objv[4])));
	if (!strncmp(type, "real", strlen("real")))
		Tcl_SetObjResult(ip, Tcl_NewDoubleObj(matr->cell[row][col]._real));
	else if (!strncmp(type, "int", strlen("int")))
		Tcl_SetObjResult(ip, Tcl_NewIntObj(matr->cell[row][col]._int));
	else if (!strncmp(type, "ptr", strlen("ptr")))
		Tcl_SetObjResult(ip, (Tcl_Obj*)matr->cell[row][col]._ptr);
	else {
		result = Tcl_NewStringObj("invalid type: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	return TCL_OK;
}



int
C_matr_set_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	int row, col;
	char *type;
 	
 	if (objc != 6) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: row */
	if ((Tcl_GetIntFromObj(ip, objv[2], &row) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid row: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: col */
	if ((Tcl_GetIntFromObj(ip, objv[3], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[4]: type */
	assert((type = Tcl_GetString(objv[4])));
	if (!strncmp(type, "real", strlen("real"))) {
		/* objv[5]: val */
		if ((Tcl_GetDoubleFromObj(ip, objv[5], &matr->cell[row][col]._real) != TCL_OK)) {
			result = Tcl_NewStringObj("invalid val: ", -1);
			Tcl_AppendObjToObj(result, objv[5]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}
		
	} else if (!strncmp(type, "int", strlen("int"))) {
		/* objv[5]: val */
		if ((Tcl_GetIntFromObj(ip, objv[5], &matr->cell[row][col]._int) != TCL_OK)) {
			result = Tcl_NewStringObj("invalid val: ", -1);
			Tcl_AppendObjToObj(result, objv[5]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}
 	} else if (!strncmp(type, "ptr", strlen("ptr"))) {
		/* objv[5]: val */
		matr->cell[row][col]._ptr = (void*)Tcl_DuplicateObj(objv[5]);
		
	} else {
		result = Tcl_NewStringObj("invalid type: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	return TCL_OK;
}

int
C_matr_add_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *m1, *m2;
 	char *typestr;
	int type;
 	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: m1 */
	if (!ptrtab_get( objv[1], (void**)&m1)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: m2 */
	if (!ptrtab_get( objv[2], (void**)&m2)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
 
	assert((typestr = Tcl_GetString(objv[3])));
	if (!strncmp(typestr, "int", strlen("int")))
		type = 1;
	else if (!strncmp(typestr, "real", strlen("real")))
		type = 2;
	else {
		result = Tcl_NewStringObj("invalid type: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_matr_add(m1, m2, type)) {
 		result = Tcl_NewStringObj("srcs_matr_add failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	return TCL_OK;
}

int
C_matr_copy_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *m1, *m2;
  	
 	if (objc != 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: m1 */
	if (!ptrtab_get( objv[1], (void**)&m1)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: m2 */
	if (!ptrtab_get( objv[2], (void**)&m2)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_matr_copy(m1, m2)) {
 		result = Tcl_NewStringObj("srcs_matr_cpy failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	return TCL_OK;
}

int
C_matr_get_rows_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
 	
 	if (objc != 2) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	Tcl_SetObjResult(ip, Tcl_NewIntObj(matr->rows));

	return TCL_OK;
}

int
C_matr_get_cols_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
 	
 	if (objc != 2) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	Tcl_SetObjResult(ip, Tcl_NewIntObj(matr->cols));

	return TCL_OK;
}

int
C_matr_m1_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	int simnum, col, type_int;
	char *type;
	real_t m1;
 	
 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: simnum */
	if ((Tcl_GetIntFromObj(ip, objv[2], &simnum) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: col */
	if ((Tcl_GetIntFromObj(ip, objv[3], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[4]: type */
	type = Tcl_GetString(objv[4]);
	type_int = -1;
	if (!strcmp(type, "int"))
		type_int = 0;
	if (!strcmp(type, "real"))
		type_int = 1;
	
	if (srcs_matr_m1(&m1, matr, simnum, col, type_int)) {
 		result = Tcl_NewStringObj("srcs_matr_m1 failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewDoubleObj(m1);
	Tcl_SetObjResult(ip, result);	
	
	return TCL_OK;
}

int
C_matr_m2_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	int simnum, col, type_int;
	char *type;
	real_t m1, m2;
 	
 	if (objc != 6) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: simnum */
	if ((Tcl_GetIntFromObj(ip, objv[2], &simnum) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: col */
	if ((Tcl_GetIntFromObj(ip, objv[3], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[4]: col */
	if ((Tcl_GetDoubleFromObj(ip, objv[4], &m1) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid m1: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[5]: type */
	type = Tcl_GetString(objv[5]);
	type_int = -1;
	if (!strcmp(type, "int"))
		type_int = 0;
	if (!strcmp(type, "real"))
		type_int = 1;
	
	if (srcs_matr_m2(&m2, matr, simnum, col, m1, type_int)) {
 		result = Tcl_NewStringObj("srcs_matr_m2 failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewDoubleObj(m2);
	Tcl_SetObjResult(ip, result);	
	
	return TCL_OK;
}


int
C_matr_p_j_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *mat;
	int simnum, col;
	real_t per;
	real_t res;
 	
 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&mat)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: simnum */
	if ((Tcl_GetIntFromObj(ip, objv[2], &simnum) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: col */
	if ((Tcl_GetIntFromObj(ip, objv[3], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[4]: percentile */
	if ((Tcl_GetDoubleFromObj(ip, objv[4], &per) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid per: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
 
 	if (srcs_matr_p_j(mat, simnum, col, per, &res)) {
 		result = Tcl_NewStringObj("srcs_matr_p_j failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	result = Tcl_NewDoubleObj(res);
	Tcl_SetObjResult(ip, result);	
	
	return TCL_OK;
}

int
C_matr_coldisp_mad_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	real_t value;
	int col;
	real_t disp;
	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: col */
	if ((Tcl_GetIntFromObj(ip, objv[2], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: value */
	if ((Tcl_GetDoubleFromObj(ip, objv[3], &value) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid value: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	if (srcs_matr_coldisp_mad(&disp, matr, col, value)) {
 		result = Tcl_NewStringObj("srcs_matr_coldisp_mad failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewDoubleObj(disp);
	Tcl_SetObjResult(ip, result);	
	
	return TCL_OK;
}

int
C_matr_coldisp_mse_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *matr;
	real_t value;
	int col;
	real_t disp;
	
 	if (objc != 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: matrix */
	if (!ptrtab_get( objv[1], (void**)&matr)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: col */
	if ((Tcl_GetIntFromObj(ip, objv[2], &col) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid col: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: value */
	if ((Tcl_GetDoubleFromObj(ip, objv[3], &value) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid value: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	if (srcs_matr_coldisp_mse(&disp, matr, col, value)) {
 		result = Tcl_NewStringObj("srcs_matr_coldisp_mad failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	result = Tcl_NewDoubleObj(disp);
	Tcl_SetObjResult(ip, result);	
	
	return TCL_OK;
}

int
C_resprof_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	int r;
	rcons_t *res;
	matr_t *prof;
	matr_t *sim_s, *sim_c;
	int numsim, n;

 	if (objc != 8) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
 	/* objv[1]: r */
	if ((Tcl_GetIntFromObj(ip, objv[1], &r) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid r: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: res */
	if (!ptrtab_get( objv[2], (void**)&res)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: prof_s */
	if (!ptrtab_get( objv[3], (void**)&prof)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[4]: sim_s */
	if (!ptrtab_get( objv[4], (void**)&sim_s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[5]: sim_c */
	if (!ptrtab_get( objv[5], (void**)&sim_c)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[6]: numsim */
	if ((Tcl_GetIntFromObj(ip, objv[6], &numsim) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid numsim: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[7]: numsim */
	if ((Tcl_GetIntFromObj(ip, objv[7], &n) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid n: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_resprof(r, res, prof, sim_s, sim_c, numsim, n)) {
		result = Tcl_NewStringObj("srcs_resprof failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_srand_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	int seed;

 	if (objc != 2) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
 	/* objv[1]: r */
	if ((Tcl_GetIntFromObj(ip, objv[1], &seed) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid seed: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	srcs_srand(seed);
	
	return TCL_OK;
}

int
C_lgsta_tard_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	rvar_t *C, *T;
	matr_t *due;
	int a;

 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
 	/* objv[1]: T */
	if (!ptrtab_get( objv[1], (void**)&T)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: C */
	if (!ptrtab_get( objv[2], (void**)&C)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: due */
	if (!ptrtab_get( objv[3], (void**)&due)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
 	/* objv[4]: a */
	if ((Tcl_GetIntFromObj(ip, objv[4], &a) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid a: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	if (srcs_lgsta_tard(C, due, T)) {
		result = Tcl_NewStringObj("srcs_lgsta_tard failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_simul_tard_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
 	matr_t *t, *c, *due;
	int numsim;

 	if (objc != 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
 	/* objv[1]: t */
	if (!ptrtab_get( objv[1], (void**)&t)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: c */
	if (!ptrtab_get( objv[2], (void**)&c)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
 	/* objv[3]: numsim */
	if ((Tcl_GetIntFromObj(ip, objv[3], &numsim) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid a: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
 	/* objv[4]: due */
	if (!ptrtab_get( objv[4], (void**)&due)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	if (srcs_simulate_tard(c, numsim, due, t)) {
		result = Tcl_NewStringObj("srcs_simulate_tard failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_mcarlo_crt_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *crt, *ecrt;
	matr_t *st, *pt, *d;
	matr_t *sort;
	plan_t *plan;
	int numsim, sink;
 	
 	if (objc != 10) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: crt */
	if (!ptrtab_get( objv[1], (void**)&crt)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: ecrt */
	if (!strcmp(Tcl_GetString(objv[2]), ".NULL"))
		ecrt = NULL;
	else if (!ptrtab_get( objv[2], (void**)&ecrt)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	/* objv[3]: st */
	if (!ptrtab_get( objv[3], (void**)&st)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[4]: pt */
	if (!ptrtab_get( objv[4], (void**)&pt)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[5]: d */
	if (!ptrtab_get( objv[5], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
 	/* objv[6]: numsim */
	if ((Tcl_GetIntFromObj(ip, objv[6], &numsim) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid numsim: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	/* objv[7]: plan */
	if (!ptrtab_get( objv[7], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
			
	/* objv[8]: sort */
	if (!ptrtab_get( objv[8], (void**)&sort)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[8]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
 	/* objv[9]: sink */
	if ((Tcl_GetIntFromObj(ip, objv[9], &sink) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid sink: ", -1);
		Tcl_AppendObjToObj(result, objv[9]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	if (srcs_mcarlo_crt(st, pt, d, numsim, plan, sort, sink, crt, ecrt)) {
		result = Tcl_NewStringObj("srcs_crt_sim failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_plan_bfs_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	matr_t *bfs;
	int rand;
	pair_t *dist;
	int i;
 	
 	if (objc < 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: bfs */
	if (!ptrtab_get( objv[1], (void**)&bfs)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	rand = 0;
	if (objc >= 4) {
		/* objv[3]: rand */
		if ((Tcl_GetIntFromObj(ip, objv[3], &rand) != TCL_OK)) {
			result = Tcl_NewStringObj("invalid rand: ", -1);
			Tcl_AppendObjToObj(result, objv[3]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}
	}
	
	assert((dist = calloc(sizeof(pair_t), plan->n)));
	if (srcs_plan_bfs(NULL, plan, 0, dist, rand)) {
		result = Tcl_NewStringObj("srcs_crit_sim failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	for (i=0; i < plan->n; i++)
		bfs->cell[0][i]._int = dist[i].x._int;
	
	free(dist);
	
	return TCL_OK;
}

int
C_plan_bfs2_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *plan;
	matr_t *bfs;
	int rand;
  	
 	if (objc < 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: bfs */
	if (!ptrtab_get( objv[1], (void**)&bfs)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	rand = 0;
	if (objc >= 4) {
		/* objv[3]: rand */
		if ((Tcl_GetIntFromObj(ip, objv[3], &rand) != TCL_OK)) {
			result = Tcl_NewStringObj("invalid rand: ", -1);
			Tcl_AppendObjToObj(result, objv[3]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}
	}
	
	if (srcs_plan_bfs2(plan, 0, bfs)) {
		result = Tcl_NewStringObj("srcs_crit_sim failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}

int
C_plan_tc_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	plan_t *tc, *plan;
	matr_t *order;
  	
 	if (objc < 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: tc */
	if (!ptrtab_get( objv[1], (void**)&tc)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: order */
	if (!ptrtab_get( objv[3], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_plan_tc(tc, plan, order)) {
		result = Tcl_NewStringObj("srcs_crit_sim failed", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	return TCL_OK;
}


int
C_plan_order_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *order;
	plan_t *plan;
	int orderlen;
	
    	
 	if (objc < 3) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: order */
	if (!ptrtab_get( objv[1], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
			
	if (srcs_plan_order(order, plan, &orderlen)) {
		result = Tcl_NewStringObj("srcs_plan_order failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	} else {
		result = Tcl_NewIntObj(orderlen);
		Tcl_SetObjResult(ip, result);
	}
 	
	return TCL_OK;
}

int
C_plan_tc_update_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t  *order;
	plan_t *tc;
	int s, t, v;
	fifo_t f;
	int i, j;
	Tcl_Obj *flist;
    	
 	if (objc < 6) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: tc */
	if (!ptrtab_get( objv[1], (void**)&tc)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (Tcl_GetIntFromObj(ip, objv[2], &s) != TCL_OK) {
		result = Tcl_NewStringObj("invalid s: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (Tcl_GetIntFromObj(ip, objv[3], &t) != TCL_OK) {
		result = Tcl_NewStringObj("invalid t: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	if (Tcl_GetIntFromObj(ip, objv[4], &v) != TCL_OK) {
		result = Tcl_NewStringObj("invalid v: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[5]: order */
	if (!ptrtab_get( objv[5], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	

	
	fifo_init(&f);
			
	if (srcs_plan_tc_update(tc, s, t, v, order, &f)) {
		result = Tcl_NewStringObj("srcs_mfsbreak failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}
	
	flist = Tcl_NewListObj(0, NULL);
	
 	while(!fifo_empty(&f)) {
		i = fifo_pop(&f);
		j = fifo_pop(&f);
		Tcl_ListObjAppendElement(ip, flist, Tcl_NewIntObj(i));
		Tcl_ListObjAppendElement(ip, flist, Tcl_NewIntObj(j));
	}
 	Tcl_SetObjResult(ip, flist); 	
	return TCL_OK;
}

int
C_mcarlo_est_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *est, *d, *s, *order;
	plan_t *plan;
	int simnum;
    	
 	if (objc < 7) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: est */
	if (!ptrtab_get( objv[1], (void**)&est)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
  
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[3]: order */
	if (!ptrtab_get( objv[3], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: d */
	if (!ptrtab_get( objv[4], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[5]: s */
	if (!ptrtab_get( objv[5], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (Tcl_GetIntFromObj(ip, objv[6], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
			
	if (srcs_mcarlo_est(est, d, s, plan, order, simnum)) {
		result = Tcl_NewStringObj("srcs_mcarlo_est failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}
 	
	return TCL_OK;
}

int
C_mcarlo_est_nwd_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *est, *sday, *d, *s, *order;
	plan_t *plan;
	int simnum;
    	
 	if (objc < 8) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: est */
	if (!ptrtab_get( objv[1], (void**)&est)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[3]: order */
	if (!ptrtab_get( objv[3], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: d */
	if (!ptrtab_get( objv[4], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[5]: s */
	if (!ptrtab_get( objv[5], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[2]: sday */
	if (!ptrtab_get( objv[6], (void**)&sday)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
  
	
	if (Tcl_GetIntFromObj(ip, objv[7], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
			
	if (srcs_mcarlo_est_nwd(est, d, s, sday, plan, order, simnum)) {
		result = Tcl_NewStringObj("srcs_mcarlo_est_nwd failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}
 	
	return TCL_OK;
}

int
C_mcarlo_lst_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *s, *d, *order;
	plan_t *plan;
 	real_t upper;
 	int simnum;
    	
 	if (objc < 6) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: s */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
  
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[3]: order */
	if (!ptrtab_get( objv[3], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: d */
	if (!ptrtab_get( objv[4], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (Tcl_GetDoubleFromObj(ip, objv[5], &upper) != TCL_OK) {
		result = Tcl_NewStringObj("invalid upper: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	simnum = 1;
	if (objc == 7) {
		if (Tcl_GetIntFromObj(ip, objv[6], &simnum) != TCL_OK) {
			result = Tcl_NewStringObj("invalid simnum: ", -1);
			Tcl_AppendObjToObj(result, objv[5]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}			
	}
					
	if (srcs_mcarlo_lst(s, d, plan, order, upper, simnum)) {
		result = Tcl_NewStringObj("srcs_mcarlo_lst failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}
 	
	return TCL_OK;
}

int
C_mcarlo_lft_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *f, *d, *order;
	plan_t *plan;
 	real_t upper;
 	int simnum;
    	
 	if (objc < 6) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	/* objv[1]: f */
	if (!ptrtab_get( objv[1], (void**)&f)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
  
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&plan)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[3]: order */
	if (!ptrtab_get( objv[3], (void**)&order)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: d */
	if (!ptrtab_get( objv[4], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (Tcl_GetDoubleFromObj(ip, objv[5], &upper) != TCL_OK) {
		result = Tcl_NewStringObj("invalid upper: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	simnum = 1;
	if (objc == 7) {
		if (Tcl_GetIntFromObj(ip, objv[6], &simnum) != TCL_OK) {
			result = Tcl_NewStringObj("invalid simnum: ", -1);
			Tcl_AppendObjToObj(result, objv[5]);
			Tcl_SetObjResult(ip, result);
			return TCL_ERROR;
		}			
	}	
					
	if (srcs_mcarlo_lft(f, d, plan, order, upper, simnum)) {
		result = Tcl_NewStringObj("srcs_mcarlo_lft failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}
 	
	return TCL_OK;
}

int
C_mcarlo_beta_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result, *sample;
 	real_t alpha, beta;
	gsl_rng *rng;
 	int sim, simnum, seed;
    	
 	if (objc < 5) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}

	if (Tcl_GetDoubleFromObj(ip, objv[1], &alpha) != TCL_OK) {
		result = Tcl_NewStringObj("invalid alpha: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (Tcl_GetDoubleFromObj(ip, objv[2], &beta) != TCL_OK) {
		result = Tcl_NewStringObj("invalid beta: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(ip, objv[3], &simnum) != TCL_OK || (simnum <= 0)) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	if (Tcl_GetIntFromObj(ip, objv[4], &seed) != TCL_OK) {
		result = Tcl_NewStringObj("invalid seed: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	
	gsl_rng_set((rng = gsl_rng_alloc(gsl_rng_default)),  seed);
	sample = Tcl_NewListObj(0, NULL);
	for (sim=0; sim < simnum; sim++) 
		Tcl_ListObjAppendElement(ip, sample, Tcl_NewDoubleObj(gsl_ran_beta(rng, alpha, beta)));
 	Tcl_SetObjResult(ip, sample); 	
	gsl_rng_free(rng);
	
	return TCL_OK;
}


int
C_makespan_p_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
	matr_t *s;
	int simnum;
	real_t P;
    	
 	if (objc < 4) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
 	
	/* objv[1]: s */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	 
	if (Tcl_GetIntFromObj(ip, objv[2], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	if (Tcl_GetDoubleFromObj(ip, objv[3], &P) != TCL_OK) {
		result = Tcl_NewStringObj("invalid P: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}			

	result = Tcl_NewDoubleObj(srcs_makespan_p(s, simnum, P));
 	Tcl_SetObjResult(ip, result); 	
	return TCL_OK;
}

static int
C_sgs_serial_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
 	plan_t *p;
	matr_t *s;
	matr_t *eseq;
	matr_t *d;
	matr_t *q;
	matr_t *b;
	
	if (objc < 7) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;	
	}

	/* objv[1]: s[] */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&p)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: d */
	if (!ptrtab_get( objv[3], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
	
	/* objv[4]: q */
	if (!ptrtab_get( objv[4], (void**)&q)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[5]: b */
 	if (!ptrtab_get( objv[5], (void**)&b)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[6]: elig sequence */
	if (!ptrtab_get( objv[6], (void**)&eseq)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_sgs_serial(s, p, d, q, b, eseq)) {
		result = Tcl_NewStringObj("srcs_sgs_serial failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}		
	return TCL_OK;
}

static int
C_sgs_cutwalk0_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
 	plan_t *p;
	plan_t *s;
	matr_t *eseq;
	matr_t *d;
	matr_t *q;
	matr_t *b;
	int simnum;
	real_t dt;
	
	if (objc < 8) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;	
	}

	/* objv[1]: s[] */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&p)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: d */
	if (!ptrtab_get( objv[3], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: simnum */
	if (Tcl_GetIntFromObj(ip, objv[4], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
		
	/* objv[5]: q */
	if (!ptrtab_get( objv[5], (void**)&q)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[6]: b */
 	if (!ptrtab_get( objv[6], (void**)&b)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[7]: elig sequence */
	if (!ptrtab_get( objv[7], (void**)&eseq)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[8]: dt */
	if ((Tcl_GetDoubleFromObj(ip, objv[8], &dt) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid dt: ", -1);
		Tcl_AppendObjToObj(result, objv[8]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_sgs_cutwalk0(s, p, d, simnum, q, b, eseq, dt)) {
		result = Tcl_NewStringObj("srcs_sgs_cutwalk failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}		
	return TCL_OK;
}

static int
C_sgs_cutwalk1_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
 	plan_t *p;
	plan_t *s;
	matr_t *eseq;
	matr_t *d;
	matr_t *q;
	matr_t *b;
	int simnum;
	real_t dt;
	char *style_str;
	cw_style_t style;
	
	if (objc < 9) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;	
	}

	/* objv[1]: s[] */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&p)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: d */
	if (!ptrtab_get( objv[3], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: simnum */
	if (Tcl_GetIntFromObj(ip, objv[4], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
		
	/* objv[5]: q */
	if (!ptrtab_get( objv[5], (void**)&q)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[6]: b */
 	if (!ptrtab_get( objv[6], (void**)&b)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[7]: elig sequence */
	if (!ptrtab_get( objv[7], (void**)&eseq)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}

	/* objv[8]: dt */
	if ((Tcl_GetDoubleFromObj(ip, objv[8], &dt) != TCL_OK)) {
		result = Tcl_NewStringObj("invalid dt: ", -1);
		Tcl_AppendObjToObj(result, objv[8]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[9]: style */
	style_str = "mean";
	if (objc > 9)
		style_str = Tcl_GetString(objv[9]);
	
	if (Tcl_StringMatch(style_str, "TRQ_CMX")) {
		style = CW_TRQ_CMX;
 	} else if (Tcl_StringMatch(style_str, "TRQ_EXP")) {
		style = CW_TRQ_EXP;
 	} else if (Tcl_StringMatch(style_str, "TRQ_NUT")) {
		style = CW_TRQ_NUT;
 	} else if (Tcl_StringMatch(style_str, "TRQ")) {
		style = CW_TRQ;
  	} else if (Tcl_StringMatch(style_str, "CMX")) {
		style = CW_CMX;
 	} else if (Tcl_StringMatch(style_str, "SGS")) {
 		style = CW_SGS;
 	} else {
 		result = Tcl_NewStringObj("invald style: ", -1);
 		Tcl_AppendStringsToObj(result, style_str, NULL);
 		Tcl_SetObjResult(ip, result);		
		return TCL_ERROR;
	}
 	 		
	if (srcs_sgs_cutwalk1(s, p, d, simnum, q, b, eseq, dt, style)) {
		result = Tcl_NewStringObj("srcs_sgs_cutwalk failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}		
	return TCL_OK;
}

static int
C_sgs_cutwalk_rho_Cmd(ClientData data, Tcl_Interp *ip, 
				int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;
 	plan_t *p;
	plan_t *s;
	matr_t *eseq;
	matr_t *d;
	matr_t *q;
	matr_t *b;
	int simnum;
 	
	if (objc < 7) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;	
	}

	/* objv[1]: s[] */
	if (!ptrtab_get( objv[1], (void**)&s)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	
	/* objv[2]: plan */
	if (!ptrtab_get( objv[2], (void**)&p)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[2]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[3]: d */
	if (!ptrtab_get( objv[3], (void**)&d)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[3]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
		
	/* objv[4]: simnum */
	if (Tcl_GetIntFromObj(ip, objv[4], &simnum) != TCL_OK) {
		result = Tcl_NewStringObj("invalid simnum: ", -1);
		Tcl_AppendObjToObj(result, objv[4]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}	
		
	/* objv[5]: q */
	if (!ptrtab_get( objv[5], (void**)&q)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[5]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[6]: b */
 	if (!ptrtab_get( objv[6], (void**)&b)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[6]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	/* objv[7]: elig sequence */
	if (!ptrtab_get( objv[7], (void**)&eseq)) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[7]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	
	if (srcs_sgs_cutwalk_rho(s, p, d, simnum, q, b, eseq)) {
		result = Tcl_NewStringObj("srcs_sgs_rho failed", -1);
 		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;		
	}		
	return TCL_OK;
}


int
C_ptr_del_Cmd(ClientData data, Tcl_Interp *ip, 
	int objc, Tcl_Obj *const objv[])
{
	Tcl_Obj *result;

 	if (objc != 2) {
 		result = Tcl_NewStringObj("invalid num. arguments", -1);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
 	}
	
	/* objv[1]: object name */
	if (!ptrtab_del(objv[1])) {
		result = Tcl_NewStringObj("does not exist: ", -1);
		Tcl_AppendObjToObj(result, objv[1]);
		Tcl_SetObjResult(ip, result);
		return TCL_ERROR;
	}
	 
	return TCL_OK;
}


/* register commands */
int DLLEXPORT
Srcs_Init(Tcl_Interp *ip)
{
	if (!Tcl_InitStubs(ip, TCL_VERSION, 0))
		return TCL_ERROR;

	/* initialize rng seed */
	srcs_srand(time(0));
		
	/* initialize hash tables */
	Tcl_InitHashTable(&ptrtab, TCL_STRING_KEYS);
 	
	Tcl_CreateObjCommand(ip, "srcs::c_plan_new",		C_plan_new_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_set_edge",	C_plan_set_edge_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_get_edge",	C_plan_get_edge_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_get_n",		C_plan_get_n_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_copy",		C_plan_copy_Cmd, NULL, NULL);
	

	Tcl_CreateObjCommand(ip, "srcs::c_rcons_new",		C_rcons_new_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_rcons_set_cap", 	C_rcons_set_cap_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_rcons_set_req", 	C_rcons_set_req_Cmd, NULL, NULL);
	
	Tcl_CreateObjCommand(ip, "srcs::c_vec_new",			C_vec_new_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_new", 		C_matr_new_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_free", 		C_matr_free_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_get", 		C_matr_get_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_set", 		C_matr_set_Cmd, NULL, NULL); 
	Tcl_CreateObjCommand(ip, "srcs::c_matr_copy", 		C_matr_copy_Cmd, NULL, NULL); 	
	Tcl_CreateObjCommand(ip, "srcs::c_matr_add", 		C_matr_add_Cmd, NULL, NULL); 
	
	Tcl_CreateObjCommand(ip, "srcs::c_matr_get_rows", 	C_matr_get_rows_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_get_cols", 	C_matr_get_cols_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_m1", 		C_matr_m1_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_m2", 		C_matr_m2_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_p_j", 		C_matr_p_j_Cmd, NULL, NULL);

	Tcl_CreateObjCommand(ip, "srcs::c_matr_coldisp_mad",C_matr_coldisp_mad_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_matr_coldisp_mse",C_matr_coldisp_mse_Cmd, NULL, NULL);

	Tcl_CreateObjCommand(ip, "srcs::c_resprof", 		C_resprof_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_srand",	 		C_srand_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_simul_tard",		C_simul_tard_Cmd, NULL, NULL);

	Tcl_CreateObjCommand(ip, "srcs::c_plan_bfs",		C_plan_bfs_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_bfs2",		C_plan_bfs2_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_tc",			C_plan_tc_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_order",		C_plan_order_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_plan_tc_update",	C_plan_tc_update_Cmd, NULL, NULL);
	
	
	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_est",		C_mcarlo_est_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_est_nwd",	C_mcarlo_est_nwd_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_lst",		C_mcarlo_lst_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_lft",		C_mcarlo_lft_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_beta",		C_mcarlo_beta_Cmd, NULL, NULL);
   
 	Tcl_CreateObjCommand(ip, "srcs::c_mcarlo_crt",		C_mcarlo_crt_Cmd, NULL, NULL);
 	
  	Tcl_CreateObjCommand(ip, "srcs::c_makespan_p",		C_makespan_p_Cmd, NULL, NULL);
  	  	
    Tcl_CreateObjCommand(ip, "srcs::c_sgs_serial",		C_sgs_serial_Cmd, NULL, NULL);
    Tcl_CreateObjCommand(ip, "srcs::c_sgs_cutwalk0",	C_sgs_cutwalk0_Cmd, NULL, NULL);
    Tcl_CreateObjCommand(ip, "srcs::c_sgs_cutwalk1",	C_sgs_cutwalk1_Cmd, NULL, NULL);
    Tcl_CreateObjCommand(ip, "srcs::c_sgs_cutwalk_rho",	C_sgs_cutwalk_rho_Cmd, NULL, NULL);
	
 	Tcl_CreateObjCommand(ip, "srcs::c_ptr_del", 		C_ptr_del_Cmd, NULL, NULL);

	return TCL_OK;
}



static int
ptrtab_get(Tcl_Obj *objkey, void **valptr)
{
	Tcl_HashEntry *entr;
 
	if (!(entr = Tcl_FindHashEntry(&ptrtab, Tcl_GetString(objkey))))
		return 0;
	
	*valptr = (void**)Tcl_GetHashValue(entr);
	return 1; 
}

static Tcl_HashEntry*
ptrtab_set(Tcl_Obj *objkey, void *val)
{
	Tcl_HashEntry *newentr;
	int isnew;
	assert((newentr=Tcl_CreateHashEntry(&ptrtab, Tcl_GetString(objkey), &isnew)));
	assert(isnew);
	Tcl_SetHashValue(newentr, val);
	
	return newentr;
}

static int
ptrtab_del(Tcl_Obj *objkey)
{
	Tcl_HashEntry *entr;
 
	if (!(entr = Tcl_FindHashEntry(&ptrtab, Tcl_GetString(objkey))))
		return 0;
	Tcl_DeleteHashEntry(entr);

 	return 1; 
}
