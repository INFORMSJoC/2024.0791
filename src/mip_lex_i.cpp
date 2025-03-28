#include "mip_lex_i.h"

#include <sstream>
#include <algorithm>

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// c_MIP_LEX_I
/////////////////////////////////////////////////////////////////////////////

c_MIP_LEX_I::c_MIP_LEX_I(int num_items, int num_elements, int num_binsMax, int capacity, const vector<int>& weight_elements, const vector<boost::dynamic_bitset<> >& v_bs_relationMatrix, double timeLimit)
	: ilo_env(),
	ilo_cplex(ilo_env),
	ilo_model(ilo_env),
	ilo_x(ilo_env, num_items * num_binsMax, 0, 1, ILOINT),
	ilo_y(ilo_env, num_elements * num_binsMax, 0, 1, ILOINT),
	ilo_z(ilo_env, num_binsMax, 0, 1, ILOINT),
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

	for (int j = 0; j < i_bins; j++)
	{
		stringstream ss;
		ss << "z_" << j;
		s_z(j).setName(ss.str().c_str());
	}
	for (int i = 0; i < i_items; i++)
	{
		for (int j = 0; j < i_bins; j++)
		{
			stringstream ss;
			ss << "x_" << i << "," << j;
			s_x(i,j).setName(ss.str().c_str());
		}
	}
	for (int l = 0; l < i_elements; l++)
	{
		for (int j = 0; j < i_bins; j++)
		{
			stringstream ss;
			ss << "y_" << l << "," << j;
			s_y(l, j).setName(ss.str().c_str());
		}
	}

	IloExpr expr(ilo_env);
	for (int j = 0; j < i_bins; j++)
		expr += s_z(j);
	ilo_objective = IloMinimize(ilo_env, expr);
	ilo_model.add(ilo_objective);

	for (int j = 0; j < i_bins; j++)
	{
		IloExpr capConstr(ilo_env);
		for (int l = 0; l < i_elements; l++)
			capConstr += s_y(l, j) * v_weights[l];
		ilo_model.add(IloConstraint(capConstr <= s_z(j) * i_capacity));
	}
	for (int i = 0; i < i_items; i++)
	{
		IloExpr part(ilo_env);
		for (int j = 0; j < i_bins; j++)
			part += s_x(i, j);
		ilo_model.add(IloConstraint(part == 1));
	}
	for (int i = 0; i < i_items; i++) 
		for (int l = 0; l < i_elements; l++)
			if (v_bs_relations[i].test(l))
				for (int j = 0; j < i_bins; j++) 		
					ilo_model.add(IloConstraint(s_x(i, j) <= s_y(l, j)));

	for (int j = 0; j < i_bins - 1; j++)
	{
		IloExpr lhs(ilo_env);
		IloExpr rhs(ilo_env);
		for (int i = 0; i < i_items; i++)
		{
			lhs += pow(2, i_items - i) * s_x(i, j);
			rhs += pow(2, i_items - i) * s_x(i, j + 1);
		}
		ilo_model.add(IloConstraint(lhs >= rhs));
	}
}

c_MIP_LEX_I::~c_MIP_LEX_I()
{
	ilo_env.end();
}

IloNumArray c_MIP_LEX_I::Solution_Bins()
{
	IloNumArray curr_z(ilo_env, ilo_z.getSize());
	ilo_cplex.getValues(curr_z, ilo_z);
	return curr_z;
}

IloNumArray c_MIP_LEX_I::Solution_Items()
{
	IloNumArray curr_x(ilo_env, ilo_x.getSize());
	ilo_cplex.getValues(curr_x, ilo_x);
	return curr_x;
}

IloNumArray c_MIP_LEX_I::Solution_Elements()
{
	IloNumArray curr_y(ilo_env, ilo_y.getSize());
	ilo_cplex.getValues(curr_y, ilo_y);
	return curr_y;
}

double c_MIP_LEX_I::Solve(string& status, double& time)
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

	if (ilo_cplex.getSolnPoolNsolns() > 0)
		return ilo_cplex.getObjValue();

	return -1;
}