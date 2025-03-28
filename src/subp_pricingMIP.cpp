#include "subp_pricingMIP.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// c_Pricing_MIP
/////////////////////////////////////////////////////////////////////////////

c_Pricing_MIP::c_Pricing_MIP(c_SUBP_Instance_With_RDC& inst, vector<double>& duals)
	: ilo_env(),
	ilo_cplex(ilo_env),
	ilo_model(ilo_env),
	ilo_x(ilo_env, inst.NumItems(), 0, 1, ILOINT),	
	ilo_y(ilo_env, inst.NumElements(), 0, 1, ILOINT),
	ilo_objective(ilo_env),
	o_inst(inst),
	numItems(inst.NumItems()),
	numElements(inst.NumElements()),
	capacity(inst.Capacity()),
	v_profits(duals)
{
	ilo_cplex.setOut(ilo_env.getNullStream()); 
	ilo_cplex.setWarning(ilo_env.getNullStream());
	ilo_env.setOut(ilo_env.getNullStream());
	ilo_cplex.setParam(IloCplex::Param::Threads, 1);

	ilo_cplex.setParam(IloCplex::Param::MIP::Limits::Populate, 15);

	for (int i = 0; i < numItems; i++)
	{
		stringstream ss;
		ss << "x_" << i;
		s_x(i).setName(ss.str().c_str());
	}
	for (int e = 0; e < numElements; e++)
	{
		stringstream ss;
		ss << "y_" << e;
		s_y(e).setName(ss.str().c_str());
	}

	IloExpr expr(ilo_env);
	for (int i = 0; i < numItems; i++)
		expr += s_x(i) * v_profits[i];
	ilo_objective = IloMaximize(ilo_env, expr);
	ilo_model.add(ilo_objective);

	IloExpr capConstr(ilo_env);
	for (int e = 0; e < numElements; e++)
		capConstr += s_y(e) * o_inst.WeightElement(e);
	ilo_model.add(IloConstraint(capConstr <= capacity));

	for (int i = 0; i < numItems; i++) 
	{
		const bs_elementsBitset& elements = o_inst.ElementsOfItemReference(i);
		bs_elementsBitset::BitIterator it_element(elements);
		while (it_element++)
			ilo_model.add(IloConstraint(s_x(i) <= s_y((int)*it_element)));
	}
}

c_Pricing_MIP::~c_Pricing_MIP()
{
	ilo_env.end();
}

IloNumArray c_Pricing_MIP::Solution_Items(int num)
{
	IloNumArray curr_x(ilo_env, ilo_x.getSize());
	ilo_cplex.getValues(curr_x, ilo_x, num);
	return curr_x;
}

IloNumArray c_Pricing_MIP::Solution_Elements(int num)
{
	IloNumArray curr_y(ilo_env, ilo_y.getSize());
	ilo_cplex.getValues(curr_y, ilo_y, num);
	return curr_y;
}

double c_Pricing_MIP::Solve()
{
	ilo_cplex.extract(ilo_model);
	ilo_cplex.solve();

	if (ilo_cplex.getStatus() == IloAlgorithm::Infeasible)
		return -100.0;
	if (ilo_cplex.getSolnPoolNsolns() == 0)
		return -100.0;

	return ilo_cplex.getObjValue(0);
}