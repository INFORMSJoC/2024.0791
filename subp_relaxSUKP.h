#ifndef relax_MODEL_H
#define relax_MODEL_H

#include "subp_instance.h"

#include <ilcplex/ilocplex.h>

using namespace std;

class c_relaxSUKP : public IloModel
{
	IloEnv ilo_env;
	IloCplex ilo_cplex;
	IloModel ilo_model;
	IloNumVarArray ilo_x;
	IloNumVarArray ilo_y;
	int i_num_items;
	int i_num_elements;
	int i_capacity;

public:
	c_relaxSUKP(int num_items, int num_elements, int capacity, const vector<double>& profit, const vector<int>& weight, const vector<bs_elementsBitset>& relation);
	~c_relaxSUKP();

	IloNumArray Solution_Elements();
	IloNumArray Solution_Items();

	void SetBoundsItem(int item, double lb, double ub);
	double Solve();
	void WriteModel();

};

#endif // of c_relaxMODEL
