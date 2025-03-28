#ifndef MIP_ARF_H
#define MIP_ARF_H

#include "subp_instance.h"

#include <ilcplex/ilocplex.h>

class c_MIP_ARF : public IloModel
{
	int i_items;
	int i_elements;
	int i_bins;
	int i_capacity;
	vector<int> v_weights;
	vector<boost::dynamic_bitset<> > v_bs_relations;

	IloEnv ilo_env;
	IloCplex ilo_cplex;
	IloModel ilo_model;

	IloNumVarArray ilo_v;
	IloNumVarArray ilo_y;
	IloObjective ilo_objective; 

public:
	c_MIP_ARF(int num_items, int num_elements, int num_binsMax, int capacity, const vector<int>& weight_elements, const vector<boost::dynamic_bitset<> >& v_bs_relationMatrix, double timeLimit);
	~c_MIP_ARF();

	IloNumVar& s_v(int i, int h) { return ilo_v[i + i_items * static_cast<IloInt>(h)]; }
	IloNumVarArray& v() { return ilo_v; }
	IloNumVar& s_y(int l, int h) { return ilo_y[l + i_elements * static_cast<IloInt>(h)]; }
	IloNumVarArray& y() { return ilo_y; }

	int NumSolutions() { return (int)ilo_cplex.getSolnPoolNsolns(); }
	IloNumArray Solution_Items();
	IloNumArray Solution_Elements();

	double Solve(string& status, double& timeSolve);
};


#endif
