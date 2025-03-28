#include "mip_arf.h"

#include <sstream>
#include <algorithm>

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// c_MIP_ARF
/////////////////////////////////////////////////////////////////////////////

c_MIP_ARF::c_MIP_ARF(int num_items, int num_elements, int num_binsMax, int capacity, const vector<int>& weight_elements, const vector<boost::dynamic_bitset<> >& v_bs_relationMatrix, double timeLimit)
	: ilo_env(),
	ilo_cplex(ilo_env),
	ilo_model(ilo_env),
	ilo_v(ilo_env, num_items* num_items, 0, 1, ILOINT),
	ilo_y(ilo_env, num_elements* num_items, 0, 1, ILOINT),
	ilo_objective(ilo_env),
	i_items(num_items),
	i_elements(num_elements),
	i_bins(num_binsMax),
	i_capacity(capacity),
	v_weights(weight_elements),
	v_bs_relations(v_bs_relationMatrix)
{
	ilo_cplex.setOut(ilo_env.getNullStream());
	ilo_cplex.setWarning(ilo_env.getNullStream());
	ilo_env.setOut(ilo_env.getNullStream());
	ilo_cplex.setParam(IloCplex::TiLim, timeLimit);
	ilo_cplex.setParam(IloCplex::Param::Threads, 1);

	for (int i = 0; i < i_items; i++)
	{
		for (int h = 0; h <= i; h++)
		{
			stringstream ss;
			ss << "v_" << i << "," << h;
			s_v(i,h).setName(ss.str().c_str());
		}
	}
	for (int l = 0; l < i_elements; l++)
	{
		for (int h = 0; h < i_bins; h++)
		{
			stringstream ss;
			ss << "y_" << l << "," << h;
			s_y(l, h).setName(ss.str().c_str());
		}
	}

	IloExpr expr(ilo_env);
	for (int h = 0; h < i_bins; h++)
		expr += s_v(h, h);
	ilo_objective = IloMinimize(ilo_env, expr);
	ilo_model.add(ilo_objective);

	for (int i = 0; i < i_items; i++)
	{
		IloExpr part(ilo_env);
		for (int h = 0; h <= i; h++)
			part += s_v(i, h);
		ilo_model.add(IloConstraint(part == 1));
	}

	for (int h = 0; h < i_bins; h++)
		for (int i = h; i < i_items; i++)
			ilo_model.add(IloConstraint(s_v(i, h) <= s_v(h, h)));

	for (int h = 0; h < i_bins; h++)
		for (int i = h; i < i_items; i++)
		{
			boost::dynamic_bitset<> bitset_tmp = v_bs_relations[i];
			for (int l = 0; l < i_elements; l++)
				if (v_bs_relations[i].test(l))
					ilo_model.add(IloConstraint(s_v(i, h) <= s_y(l, h)));
		}

	for (int h = 0; h < i_bins; h++)
	{
		IloExpr capConstr(ilo_env);
		for (int l = 0; l < i_elements; l++)
			capConstr += s_y(l, h) * v_weights[l];
		ilo_model.add(IloConstraint(capConstr <= s_v(h, h) * i_capacity));
	}
	
}

c_MIP_ARF::~c_MIP_ARF()
{
	ilo_env.end();
}

IloNumArray c_MIP_ARF::Solution_Items()
{
	IloNumArray curr_v(ilo_env, ilo_v.getSize());
	ilo_cplex.getValues(curr_v, ilo_v);
	return curr_v;
}

IloNumArray c_MIP_ARF::Solution_Elements()
{
	IloNumArray curr_y(ilo_env, ilo_y.getSize());
	ilo_cplex.getValues(curr_y, ilo_y);
	return curr_y;
}

double c_MIP_ARF::Solve(string& status, double& timeSolve)
{
	ilo_cplex.extract(ilo_model);
	ilo_cplex.solve();

	if (ilo_cplex.getStatus() == IloAlgorithm::Infeasible)
		status = "infeasible";
	else if (ilo_cplex.getStatus() == IloAlgorithm::Feasible)
		status = "feasible";
	else if (ilo_cplex.getStatus() == IloAlgorithm::Optimal)
		status = "optimal";
	else
		status = "other";

	return ilo_cplex.getObjValue();
}

