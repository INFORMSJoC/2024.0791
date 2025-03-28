#ifndef MIP_LEX_I
#define MIP_LEX_I

#include "subp_instance.h"

#include <ilcplex/ilocplex.h>

class c_MIP_LEX_I : public IloModel
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

	IloNumVarArray ilo_x;
	IloNumVarArray ilo_y;
	IloNumVarArray ilo_z;
	IloObjective ilo_objective; 

public:
	c_MIP_LEX_I(int num_items, int num_elements, int num_binsMax, int capacity, const vector<int>& weight_elements, const vector<boost::dynamic_bitset<> >& v_bs_relationMatrix, double timeLimit);
	~c_MIP_LEX_I();

	IloNumVar& s_x(int i, int j) { return ilo_x[i + i_items * static_cast<IloInt>(j)]; }
	IloNumVarArray& x() { return ilo_x; }
	IloNumVar& s_y(int l, int j) { return ilo_y[l + i_elements * static_cast<IloInt>(j)]; }
	IloNumVarArray& y() { return ilo_y; }
	IloNumVar& s_z(int j) { return ilo_z[j]; }
	IloNumVarArray& z() { return ilo_z; }

	int NumSolutions() { return (int)ilo_cplex.getSolnPoolNsolns(); }
	IloNumArray Solution_Bins();
	IloNumArray Solution_Items();
	IloNumArray Solution_Elements();

	double Solve(string& status, double& timeSolve);

};


#endif
