#ifndef SUBP_PRICING_SOLVER_H
#define SUBP_PRICING_SOLVER_H

#include "quick_union.h"

#include "subp_instance.h"
#include "subp_spprc_labeling.h"
#include "subp_pricingMIP.h"

class c_Pricing_Solver_SUBP {
	int i_approach;
	c_SUBP_Instance_With_RDC& o_instance;

	c_SPPRC_Labeling_SUBP o_SUBP_pricing_solver;
	pair <pair<double, int>, vector<c_Group_Decision> > p_dummy;

	void tree(const vector<int>& base_set, int base_set_pos, const vector<bs_itemsBitset>& conflicts_bs, vector<bs_itemsBitset>& result);
	void all_subsets(const vector<int>& base_set, const vector<bs_itemsBitset>& conflicts_bs, vector<bs_itemsBitset>& result);
	void bs_component_items(quick_union::c_QuickUnionFind& qu, int p, bs_itemsBitset& comp);

public:
	c_Pricing_Solver_SUBP(c_SUBP_Instance_With_RDC& instance, int approach);
	~c_Pricing_Solver_SUBP() {}

	void UpdateDuals(const vector<double>& item_covering_duals, double bin_convexity_dual);
	void UpdateBranchingConstraints(const vector<pair<int,int>>& separate_constraints, const vector<pair<int, int>>& together_constraints);
	bool Pricing(vector<bs_itemsBitset>& new_columns, vector<double>& new_rdc);
	int HeuristicPricing(vector<bs_itemsBitset>& new_columns, vector<double>& new_rdc);

#ifdef UseBoundingInLabeling
	void ComputeGlobalBounds();
#endif
};

#endif // of SUBP_PRICING_SOLVER_H