#ifndef SUBP_SPPRC_LABELING_H
#define SUBP_SPPRC_LABELING_H

#include "subp_instance.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////
// c_Label_SUBP
////////////////////////////////////////////////////////////////////////////////////////////////////////

class c_Label_SUBP {
public:
	double d_cost;
	int i_load;
	double d_sumDual;
	bs_itemsBitset bs_items;
	bs_elementsBitset bs_elements;
	bs_itemsBitset bs_compatibleItems;
public:
	c_Label_SUBP() : d_cost(1.0), i_load(0), d_sumDual(0.0), bs_compatibleItems(bs_compatibleItems.set()) // init label components
	{}	
	c_Label_SUBP(const c_Label_SUBP& orig)
		: d_cost(orig.d_cost), i_load(orig.i_load), d_sumDual(orig.d_sumDual), bs_items(orig.bs_items), bs_elements(orig.bs_elements), bs_compatibleItems(orig.bs_compatibleItems)
	{}
	c_Label_SUBP(double cost, int load, double sumDual, const bs_itemsBitset& items, const bs_elementsBitset& elements, const bs_itemsBitset& compatibleItems)
		: d_cost(cost), i_load(load), d_sumDual(sumDual), bs_items(items), bs_elements(elements), bs_compatibleItems(compatibleItems)
	{}
	virtual ~c_Label_SUBP() {}
	void OutputInStream(std::ostream& s) const;

#ifdef UseLabelingWithDominance
	bool Dominates(const c_Label_SUBP& second);
	bool operator<(const c_Label_SUBP& other) //(1)
	{
		if (d_cost < other.d_cost)
			return true;
		else if (d_cost > other.d_cost)
			return false;
		else return i_load < other.i_load;
	}
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// c_SPPRC_Labeling_SUBP
////////////////////////////////////////////////////////////////////////////////////////////////////////

class c_SPPRC_Labeling_SUBP {
	c_SUBP_Instance_With_RDC& o_instance;
	vector<vector<c_Group_Decision> > v_REFs;
	vector<c_Label_SUBP> v_labels_one;
	vector<c_Label_SUBP> v_labels_two;
	vector<c_Label_SUBP>* v_oldLabels;
	vector<c_Label_SUBP>* v_newLabels;
#ifdef UseLabelingWithDominance
	vector<bool> v_labels_dominated;
#endif
public:
	c_SPPRC_Labeling_SUBP(int maxStages, c_SUBP_Instance_With_RDC& instance) : v_REFs(maxStages), v_oldLabels(nullptr), v_newLabels(nullptr), o_instance(instance) {}
	virtual ~c_SPPRC_Labeling_SUBP() {}
	void AddREFs(int stage, const vector<c_Group_Decision>& REFs) { v_REFs[stage] = REFs; }
	void RemoveAllREFs();
	bool Solve(const int sink, int max_num_paths = numeric_limits<int>::max());
	bool GetPaths(vector<c_Label_SUBP*>& paths, function<bool(c_Label_SUBP*, c_Label_SUBP*)>* pointer_sorter = nullptr, int max_num_paths = numeric_limits<int>::max());
	void Clear();
	bool Propagate(const c_Label_SUBP& old, const c_Group_Decision& decision, int stage, double& new_cost, int& new_load, double& new_sumDual, bs_itemsBitset& new_items, bs_elementsBitset& new_elements, bs_itemsBitset& new_compatibleItems, bool& negRedCost);
};

#endif // of SUBP_DEDICATED_LABELING_H