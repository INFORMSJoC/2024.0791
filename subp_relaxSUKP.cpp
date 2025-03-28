#include "subp_relaxSUKP.h"

#include <strstream>
#include <numeric>


c_relaxSUKP::c_relaxSUKP(int num_items, int num_elements, int capacity, const vector<double>& profit, const vector<int>& weight, const vector<bs_elementsBitset>& relation)
	: ilo_env(),
	ilo_cplex(ilo_env),
	ilo_model(ilo_env),
	ilo_x(ilo_env, num_items, 0.0, 1.0, ILOFLOAT), 
	ilo_y(ilo_env, num_elements, 0.0, 1.0, ILOFLOAT), 
	i_num_items(num_items),
	i_num_elements(num_elements),
	i_capacity(capacity)
{
	ilo_cplex.setOut(ilo_env.getNullStream()); 
	ilo_cplex.setWarning(ilo_env.getNullStream());
	ilo_env.setOut(ilo_env.getNullStream());
	ilo_cplex.setParam(IloCplex::Param::Threads, 1);

	for (int i = 0; i < num_items; i++)
	{
		stringstream ss;
		ss << "x_" << i;
		ilo_x[i].setName(ss.str().c_str());
	}
	for (int l = 0; l < num_elements; l++)
	{
		stringstream ss;
		ss << "y_" << l;
		ilo_y[l].setName(ss.str().c_str());
	}

	IloExpr obj(ilo_env);
	for (int i = 0; i < i_num_items; i++)
		obj += profit[i] * ilo_x[i];
	
	IloObjective objective(ilo_env, obj, IloObjective::Maximize);
	ilo_model.add(objective);

	IloExpr a(ilo_env);
	for (int j = 0; j < i_num_elements; j++)	
		a += weight[j] * ilo_y[j];
	ilo_model.add(IloConstraint(a <= i_capacity));

	for (int i = 0; i < i_num_items; i++) 
	{
		const bs_elementsBitset& tmp = relation[i];	
		for (int j = 0; j < i_num_elements; j++) 
		{
			if (tmp.test(j))
			{		
				ilo_model.add(IloConstraint(ilo_x[i] <= ilo_y[j]));
			}
		}
	}
}


c_relaxSUKP::~c_relaxSUKP()
{
	ilo_env.end();
}

IloNumArray c_relaxSUKP::Solution_Items()
{
	IloNumArray curr_x(ilo_env, ilo_x.getSize());
	ilo_cplex.getValues(curr_x, ilo_x);
	return curr_x;
}

IloNumArray c_relaxSUKP::Solution_Elements()
{
	IloNumArray curr_y(ilo_env, ilo_y.getSize());
	ilo_cplex.getValues(curr_y, ilo_y);
	return curr_y;
}

void c_relaxSUKP::WriteModel() 
{
	ilo_cplex.extract(ilo_model);	
	ilo_cplex.exportModel("relax-SUKP-model.lp");
}


double c_relaxSUKP::Solve() 
{
	ilo_cplex.extract(ilo_model);
	ilo_cplex.solve();

	if (ilo_cplex.getStatus() != IloAlgorithm::Optimal)
		return INFINITY;
		
	return ilo_cplex.getObjValue();
}

void c_relaxSUKP::SetBoundsItem(int item, double lb, double ub)
{
	ilo_x[item].setBounds(lb, ub);
}