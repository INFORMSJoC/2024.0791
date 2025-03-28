#include "subp_spprc_labeling.h"

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////////////////////////////
// c_Label_SUBP
////////////////////////////////////////////////////////////////////////////////////////////////////////

void c_Label_SUBP::OutputInStream(std::ostream& s) const
{
	s << "cost:" << d_cost << endl;
	s << "load:" << i_load << endl;
	s << "sum dual:" << d_sumDual << endl;
	s << "items: ( ";
	bs_itemsBitset::BitIterator iter(bs_items);
	while (iter++)
	{
		s << (int)*iter << " ";
	}
	s << ")" << endl;
	s << "elements: ( ";
	bs_elementsBitset::BitIterator iterEle(bs_elements);
	while (iterEle++)
	{
		s << (int)*iterEle << " ";
	}
	s << ")" << endl;
	s << "compatible items: ( ";
	bs_itemsBitset::BitIterator iterCompItems(bs_compatibleItems);
	while (iterCompItems++)
	{
		s << (int)*iterCompItems << " ";
	}
	s << ")" << endl;
}

#ifdef UseLabelingWithDominance
bool c_Label_SUBP::Dominates(const c_Label_SUBP& second)
{
#ifdef LabelingOnElements
	if (i_load > second.i_load)
		return false;
	if ((~bs_compatibleItems & second.bs_compatibleItems).any())
		return false;
#else
	if ((~second.bs_elements & bs_elements).any())
		return false;
#endif //LabelingOnElements
	return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////
// c_SPPRC_Labeling_SUBP
////////////////////////////////////////////////////////////////////////////////////////////////////////

void c_SPPRC_Labeling_SUBP::RemoveAllREFs()
{
	for (int i = 0; i < (int)v_REFs.size(); ++i)
		v_REFs[i].clear();
}

void c_SPPRC_Labeling_SUBP::Clear()
{
	v_labels_one.clear();
	v_labels_two.clear();
#ifdef UseLabelingWithDominance
	v_labels_dominated.clear();
#endif
}

bool c_SPPRC_Labeling_SUBP::Solve(const int sink, int max_num_paths)
{
	Clear();
	v_oldLabels = &v_labels_one;
	v_newLabels = &v_labels_two;
	v_oldLabels->emplace_back();
#ifdef UseLabelingWithDominance
	v_labels_dominated.push_back(false);
#endif
	bool negRedCost = false;
	int numNegRedCostPath_fw = 0;
	double new_cost = 0.0;
	int new_load = 0;
	double new_sumDual = 0.0;
	bs_itemsBitset new_items;
	bs_elementsBitset new_elements;
	bs_itemsBitset new_compatibleItems;
	for (int predStage = 0; predStage < sink && (numNegRedCostPath_fw < max_num_paths); ++predStage)
	{
		if (predStage % 2 == 0)
		{
			v_oldLabels = &v_labels_one;
			v_newLabels = &v_labels_two;
		}
		else
		{
			v_oldLabels = &v_labels_two;
			v_newLabels = &v_labels_one;
		}
		numNegRedCostPath_fw = 0;
		v_newLabels->clear();

#ifdef UseLabelingWithDominance
		int idx = 0;
#endif
		for (auto &predLabel : *v_oldLabels)
		{
#ifdef UseLabelingWithDominance
			if (v_labels_dominated[idx])
			{
				++idx;
				continue;
			}
#endif
			for (auto &decision : v_REFs[predStage])
			{
				negRedCost = false;
				if (Propagate(predLabel, decision, predStage+1, new_cost, new_load, new_sumDual, new_items, new_elements, new_compatibleItems, negRedCost))
				{
					v_newLabels->emplace_back(new_cost, new_load, new_sumDual, new_items, new_elements, new_compatibleItems);
					if (negRedCost)
						++numNegRedCostPath_fw;
				}
			}
#ifdef UseLabelingWithDominance
			++idx;
#endif
		}
#ifdef UseLabelingWithDominance
		v_labels_dominated.resize(v_newLabels->size());
		fill(v_labels_dominated.begin(), v_labels_dominated.end(), false);
		sort(v_newLabels->begin(), v_newLabels->end());
		for (int idxOne = 0; idxOne < v_newLabels->size(); ++idxOne)
		{
			if (v_labels_dominated[idxOne])
				continue;
			c_Label_SUBP& curLabel = (*v_newLabels)[idxOne];
			for (int idxTwo = idxOne + 1; idxTwo < v_newLabels->size(); ++idxTwo)
				if (curLabel.Dominates((*v_newLabels)[idxTwo]))
					v_labels_dominated[idxTwo] = true;
		}
#endif
	}
	return (numNegRedCostPath_fw < max_num_paths);
}

bool c_SPPRC_Labeling_SUBP::GetPaths(vector<c_Label_SUBP*>& paths, function<bool(c_Label_SUBP*, c_Label_SUBP*)>* pointer_sorter, int max_num_paths)
{
	for (auto &label : *v_newLabels)
	{
		if (label.d_cost <= o_instance.CurrentTargetRDC())
			paths.push_back(&label);
	}
	if (pointer_sorter)
		sort(paths.begin(), paths.end(), *pointer_sorter);
	if ((int)paths.size() > max_num_paths && max_num_paths != numeric_limits<int>::max())
	{
		paths.resize(max_num_paths);
		if (!pointer_sorter)
			return false;
	}
	return true;
}

bool c_SPPRC_Labeling_SUBP::Propagate(const c_Label_SUBP& old, const c_Group_Decision& decision, int stage, double& new_cost, int& new_load, double& new_sumDual, bs_itemsBitset& new_items, bs_elementsBitset& new_elements, bs_itemsBitset& new_compatibleItems, bool& negRedCost)
{	
#ifdef LabelingOnElements
	if (decision.bs_elements.none())
	{
		new_load = old.i_load;
		new_items = old.bs_items;
		new_elements = old.bs_elements;
		new_compatibleItems = old.bs_compatibleItems & ~(o_instance.ItemsOfElements(o_instance.ElementsOfStage(stage)));
		new_cost = old.d_cost;
	}
	else
	{
		new_load = old.i_load + decision.i_weight;
		if (new_load > o_instance.Capacity())
			return false;

		new_elements = old.bs_elements | decision.bs_elements;
		bs_itemsBitset newFeasibleItems = o_instance.FeasibleItemsFromAddedElements(new_elements, decision.bs_elements, old.bs_compatibleItems);
		new_items = old.bs_items | newFeasibleItems;
		new_compatibleItems = o_instance.CompatibleItems(new_elements, old.bs_compatibleItems & ~newFeasibleItems, stage);
		
		new_cost = old.d_cost;
		bs_itemsBitset::BitIterator iter(newFeasibleItems);
		while(iter++)
			new_cost -= o_instance.Pi((int)*iter);
	}
#else
	if (decision.bs_items.none())
	{
		new_load = old.i_load; 
		new_items = old.bs_items;
		new_elements = old.bs_elements;
		new_compatibleItems = old.bs_compatibleItems & ~o_instance.ItemsOfStage(stage);
		new_sumDual = old.d_sumDual;
		new_cost = old.d_cost;
	}
	else		// new item(s) 
	{
		new_elements = old.bs_elements | decision.bs_elements;
		new_load = o_instance.WeightElements(new_elements);

		if (new_load > o_instance.Capacity())
			return false;

		new_items = old.bs_items | decision.bs_items;
		new_compatibleItems = o_instance.CompatibleItems(new_elements, old.bs_compatibleItems & ~o_instance.ItemsOfStage(stage), stage);
		new_sumDual = old.d_sumDual + decision.d_sumDual;
		new_cost = o_instance.CostBin() - new_sumDual;	
	}
#endif // LabelingOnElements

#ifdef UseBoundingInLabeling

	if (stage != o_instance.LastStage())
	{
#ifdef UseLabelingBoundGlobalSumDuals
		if (new_cost - o_instance.GetGlobalBoundSumDuals(stage) > o_instance.CurrentTargetRDC())
		{
			return false;
		}
#endif	// UseLabelingBoundGlobalSumDuals

#ifdef UseLabelingBoundSumCompDuals
		if (new_cost - o_instance.ComputeLabelingBoundSumDuals(new_compatibleItems) > o_instance.CurrentTargetRDC())
		{
			return false;
		}
#endif	// UseLabelingBoundSumCompDuals

#ifdef UseLabelingBoundDP
		if (new_cost - o_instance.ComputeLabelingBoundDP(new_elements, new_compatibleItems, stage, new_load) > o_instance.CurrentTargetRDC())
		{
			return false;
		}
#endif	// UseLabelingBoundDP

#ifdef UseLabelingBoundRelaxedSUKP
		if (new_cost - o_instance.ComputeLabelingBoundRelaxedSUKP(new_elements, new_compatibleItems, new_load) > o_instance.CurrentTargetRDC())
		{
			return false;
		}
#endif	// UseLabelingBoundRelaxedSUKP
	}

	else if (new_cost > o_instance.CurrentTargetRDC() || new_load == 0)	
		return false;
#else
	if (stage == o_instance.LastStage() && (new_cost > o_instance.CurrentTargetRDC() || new_load == 0))
		return false;

#endif // UseBoundingInLabeling


	if (new_cost <= o_instance.CurrentTargetRDC())	
		negRedCost = true;

	return true;
}
