#ifndef SUBP_PRICINGMIP_H
#define SUBP_PRICINGMIP_H

#include "subp_instance.h"

#include <ilcplex/ilocplex.h>

class c_Pricing_MIP : public IloModel
{
	const c_SUBP_Instance_With_RDC& o_inst;
	int numItems;
	int numElements;
	int capacity;
	vector<double> v_profits;

	IloEnv ilo_env;
	IloCplex ilo_cplex;
	IloModel ilo_model;
	IloObjective ilo_objective;

	IloNumVarArray ilo_x;
	IloNumVarArray ilo_y;

public:
	c_Pricing_MIP(c_SUBP_Instance_With_RDC& inst, vector<double>& duals);
	~c_Pricing_MIP();

	IloNumVar& s_x(int i) { return ilo_x[i]; }
	IloNumVarArray& x() { return ilo_x; }
	IloNumVar& s_y(int l) { return ilo_y[l]; }
	IloNumVarArray& y() { return ilo_y; }

	int NumSolutions() { return (int)ilo_cplex.getSolnPoolNsolns(); }
	IloNumArray Solution_Items(int num);
	IloNumArray Solution_Elements(int num);
	double Solution_ObjVal(int num) { return ilo_cplex.getObjValue(num); }

	void WriteModel();
	double Solve();
};


#endif
