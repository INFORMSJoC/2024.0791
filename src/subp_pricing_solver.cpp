#include "subp_pricing_solver.h"

using namespace quick_union;

c_Pricing_Solver_SUBP::c_Pricing_Solver_SUBP(c_SUBP_Instance_With_RDC& instance, int approach)
	:
	i_approach(approach),
	o_instance(instance),
#ifdef LabelingOnElements
	o_SUBP_pricing_solver(instance.NumElements() + 1, instance)
#else
	o_SUBP_pricing_solver(instance.NumItems() + 1, instance)
#endif
{
}

bool greaterEqualGroupDecisions(const pair <pair<double, int>, vector<c_Group_Decision> >& x, const pair <pair<double, int>, vector<c_Group_Decision> >& y)
{
	return (x.first.first > y.first.first);
}
bool greaterEqualSort(const pair<double, pair<int, int> >& x, const pair<double, pair< int, int > >& y)
{
	return (x.first > y.first);
}
function<bool(c_Label_SUBP*, c_Label_SUBP*)> sorter_rdc = [](c_Label_SUBP* l1, c_Label_SUBP* l2)
	{ return l1->d_cost < l2->d_cost; };

void c_Pricing_Solver_SUBP::UpdateDuals(const vector<double>& item_covering_duals, double bin_convexity_dual)
{
	for (int i = 0; i < o_instance.NumItems(); ++i)
		o_instance.SetPi(i, item_covering_duals[i]);
	o_instance.SetMu(bin_convexity_dual);
	o_instance.SetCurrentTargetRDC(o_instance.Mu() - pricing_tolerance);
	double mu = o_instance.Mu();

#ifdef LabelingOnElements
	vector<double> normProfit(o_instance.NumItems(), 0);
	for (int i = 0; i < o_instance.NumItems(); i++)
		normProfit[i] = o_instance.Pi(i) / o_instance.WeightItem(i);
#endif // LabelingOnElements


	vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = o_instance.GetSortedGroupDecisions();
	for (int i = 0; i < (int)group_decisions.size(); ++i)
	{
		double bestCriterionValue = 0.0;
		int maxWeight = 0;
		vector<c_Group_Decision>& decisions = group_decisions[i].second;
		for (int j = 0; j < (int)decisions.size(); ++j)
		{

#ifdef LabelingOnElements
			double value = 0;
			bs_elementsBitset::BitIterator el(decisions[j].bs_elements);
			while (el++)
			{
				double frac = 0;
				bs_itemsBitset items = o_instance.ItemsOfElementReference((int)*el);
				bs_itemsBitset::BitIterator it(items);
				while (it++)
				{
					frac += normProfit[(int)*it];
				}
				frac /= o_instance.FrequencyElement((int)*el);
				value += frac;
			}
			if (bestCriterionValue < value)
				bestCriterionValue = value;
			if (maxWeight < decisions[j].i_weight)
				maxWeight = decisions[j].i_weight;
#else
			double sumDual = 0.0;
			bs_itemsBitset::BitIterator iter(decisions[j].bs_items);
			while (iter++)
				sumDual += o_instance.Pi((int)*iter);

			decisions[j].UpdateDuals(sumDual);

			if (bestCriterionValue < sumDual)
				bestCriterionValue = sumDual;
			if (maxWeight < decisions[j].i_weight)
				maxWeight = decisions[j].i_weight;
#endif // LabelingOnElements
		}
		group_decisions[i].first = make_pair(bestCriterionValue, maxWeight);
	}
	sort(group_decisions.begin(), group_decisions.end(), greaterEqualGroupDecisions);

	if (o_instance.Mu() >= 0)
		o_instance.SetLastStage((int)group_decisions.size());
	else
	{
		auto pos = lower_bound(group_decisions.begin(), group_decisions.end(), p_dummy, greaterEqualGroupDecisions);
		o_instance.SetLastStage((int)(pos - group_decisions.begin()));
	}

#ifdef UseBoundingInLabeling
	ComputeGlobalBounds();
#endif // UseBoundingInLabeling

	o_SUBP_pricing_solver.RemoveAllREFs();
	for (int i = 0; i < o_instance.LastStage(); ++i)
		o_SUBP_pricing_solver.AddREFs(i, group_decisions[i].second);
}

void c_Pricing_Solver_SUBP::UpdateBranchingConstraints(const vector<pair<int, int>>& separate_constraints, const vector<pair<int, int>>& together_constraints)
{
#ifdef LabelingOnElements
	int n = o_instance.NumElements();
#else
	int n = o_instance.NumItems();
#endif // LabelingOnElements

	c_QuickUnionFind groups(n);
	c_QuickUnionFind together(n);
	vector<vector<int> > separate(n);
	for (const auto& items : separate_constraints)
	{
		separate[items.first].push_back(items.second);
		separate[items.second].push_back(items.first);
		groups.merge(items.first, items.second);
	}
	for (const auto& items : together_constraints)
	{
		together.merge(items.first, items.second);
		groups.merge(items.first, items.second);
	}

	vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = o_instance.GetSortedGroupDecisions();
	group_decisions.clear();

	vector<vector<int> > vec_set(n);
	for (int i = 0; i < n; i++)
		vec_set[groups.root(i)].push_back(i);
	for (int i = 0; i < n; i++)
	{
#ifdef LabelingOnElements
		vector<bs_elementsBitset> ps;
#else
		vector<bs_itemsBitset> ps;
#endif // LabelingOnElements

		if (vec_set[i].empty())
			continue;
		else if (vec_set[i].size() == 1)
		{
#ifdef LabelingOnElements
			bs_elementsBitset set_i;
#else
			bs_itemsBitset set_i;
#endif // LabelingOnElements
			set_i.set(vec_set[i].front());
			ps.push_back(set_i);
		}
		else
		{
			vector<int> representatives;
#ifdef LabelingOnElements
			vector<bs_elementsBitset> repr_conflicts_bs(n);
#else
			vector<bs_itemsBitset> repr_conflicts_bs(n);
#endif // LabelingOnElements		
			for (auto& ii : vec_set[i])
			{
				if (together.root(ii) == ii)
					representatives.push_back(ii);
				for (auto& jj : separate[ii])
				{
					repr_conflicts_bs[together.root(ii)].set(together.root(jj));
					repr_conflicts_bs[together.root(jj)].set(together.root(ii));
				}
			}
#ifdef LabelingOnElements
			vector<bs_elementsBitset> repr_sub;
#else
			vector<bs_itemsBitset> repr_sub;
#endif // LabelingOnElements			
			int res_size = (int)max((double)(representatives.size() * 2), pow(2, representatives.size() / 2));
			repr_sub.reserve(res_size);
			all_subsets(representatives, repr_conflicts_bs, repr_sub);
			for (int si = 0; si < (int)repr_sub.size(); ++si)
			{
#ifdef LabelingOnElements
				bs_elementsBitset union_set;
				bs_elementsBitset::BitIterator iter(repr_sub[si]);
#else
				bs_itemsBitset union_set;
				bs_itemsBitset::BitIterator iter(repr_sub[si]);
#endif // LabelingOnElements	
				while (iter++)
				{
					bs_component_items(together, (int)*iter, union_set);
				}
				ps.push_back(union_set);
			}
		}

		vector<c_Group_Decision> decisions;
		decisions.reserve(ps.size() + 1);
		bs_elementsBitset elements;
		int weight = 0;
#ifdef LabelingOnElements
		decisions.push_back(move(c_Group_Decision(elements, weight)));
#else
		bs_itemsBitset items;
		decisions.push_back(move(c_Group_Decision(items, 0.0, weight, elements)));
#endif // LabelingOnElements

		for (int ii = 0; ii < (int)ps.size(); ++ii)
		{
			elements.reset();
			weight = 0;
#ifdef LabelingOnElements
			elements = ps[ii];
			weight = o_instance.WeightElements(elements);
#else
			items.reset();
			items = ps[ii];
			elements = o_instance.ElementsOfItems(items);
			weight = o_instance.WeightElements(elements);
#endif // LabelingOnElements

			if (weight <= o_instance.Capacity())
			{
#ifdef LabelingOnElements
				decisions.push_back(move(c_Group_Decision(elements, weight)));
#else
				decisions.push_back(move(c_Group_Decision(items, 0.0, weight, elements)));
#endif // LabelingOnElements
			}
		}
		group_decisions.push_back(make_pair(make_pair(0.0, 0), decisions));
	}
}

bool c_Pricing_Solver_SUBP::Pricing(vector<bs_itemsBitset>& new_columns, vector<double>& new_rdc)
{
	new_columns.clear();
	new_rdc.clear();
	bool optimalPricing = false;

	if (UsePricingHeuristic)
	{		
		HeuristicPricing(new_columns, new_rdc);
		if (!new_columns.empty())
			return false;
	}

	// labeling
	if (i_approach == 0)
	{
		int source = 0;
		int sink = o_instance.LastStage();
		int maxNumRedCostPath = MaxNumNegRedCostPathFactor > 0 ? (int)(MaxNumNegRedCostPathFactor * o_instance.NumItems()) : numeric_limits<int>::max();

		vector<c_Label_SUBP*> bins;
		
		optimalPricing = o_SUBP_pricing_solver.Solve(sink, maxNumRedCostPath);
		optimalPricing &= o_SUBP_pricing_solver.GetPaths(bins, &sorter_rdc, maxNumRedCostPath);

		for (int i = 0; i < (int)bins.size(); ++i)
		{
			if (bins[i]->i_load == 0)	
			{
				continue;
			}
			double rdc = bins[i]->d_cost - o_instance.Mu();
			if (rdc > -pricing_tolerance)	
			{
				break;
			}
			new_columns.push_back(bins[i]->bs_items);
			new_rdc.push_back(bins[i]->d_cost);
		}
	}

	// MIP
	else if(i_approach == 1)
	{
		vector<double> curr_duals(o_instance.NumItems(), 0);
		for (int i = 0; i < o_instance.NumItems(); ++i)
		{
			curr_duals[i] = o_instance.Pi(i);
		}
		c_Pricing_MIP pricing_model(o_instance, curr_duals);
		double sumDuals = pricing_model.Solve();

		optimalPricing = true;
		for (int num = 0; num < pricing_model.NumSolutions(); ++num)
		{
			double rdc = o_instance.CostBin() - pricing_model.Solution_ObjVal(num) - o_instance.Mu();
			if (rdc > -pricing_tolerance)
			{
				continue;
			}
			IloNumArray itemSet = pricing_model.Solution_Items(num);
			bs_itemsBitset bs_items;
			for (int i = 0; i < pricing_model.Solution_Items(num).getSize(); ++i)
			{
				if (itemSet[i] > 0.99)
				{
					bs_items.set(i);
				}
			}
			new_columns.push_back(bs_items);
			new_rdc.push_back(rdc);
			optimalPricing = false;
		}
	}

	return optimalPricing;
}

int c_Pricing_Solver_SUBP::HeuristicPricing(vector<bs_itemsBitset>& new_columns, vector<double>& new_rdc)
{
	new_columns.clear();
	new_rdc.clear();
	bs_itemsBitset itemSet;
	bs_elementsBitset elementSet;
	vector<pair<double, pair<int, int> > > v_ratios;
	unordered_map<bs_itemsBitset, double> map_allBins;
	unordered_map<bs_itemsBitset, double> map_negRDCBins;
	bs_itemsBitset bs_stageList;

#ifdef LabelingOnElements
	for (int g = 0; g < o_instance.NumItems(); ++g)
	{
		itemSet.reset();
		double sumProfit = 0.0;
		int weightInit = o_instance.WeightItem(g);
		bs_itemsBitset itemSetWithItem = itemSet;
		itemSetWithItem.set(g);
		if (weightInit <= o_instance.Capacity() && map_allBins.find(itemSetWithItem) == map_allBins.end())
		{
			itemSet.set(g);
			sumProfit = o_instance.Pi(g);
			map_allBins[itemSet] = 1;
			if (o_instance.CostBin() - sumProfit - o_instance.Mu() <= -pricing_tolerance)
			{
				map_negRDCBins[itemSet] = o_instance.CostBin() - sumProfit - o_instance.Mu();
			}
			bool recalculateRatios = true;
			while (recalculateRatios)
			{
				v_ratios.clear();
				for (int d = 0; d < o_instance.NumItems(); ++d)
				{
					if (!itemSet.test(d))
					{
						double ratio = -1;
						int decisionIndex = -1;
						itemSetWithItem = itemSet;
						itemSetWithItem.set(d);
						if (o_instance.WeightItems(itemSetWithItem) <= o_instance.Capacity() && map_allBins.find(itemSetWithItem) == map_allBins.end())
						{
							double additionalWeight = o_instance.WeightItems(itemSetWithItem) - o_instance.WeightItems(itemSet);
							if (additionalWeight == 0)
							{
								additionalWeight += 0.0001;
							}
							if (ratio < o_instance.Pi(d) / additionalWeight)
							{
								v_ratios.push_back(make_pair(o_instance.Pi(d) / additionalWeight, make_pair(d, decisionIndex)));
							}
						}
						itemSetWithItem.reset(d);
					}
				}
				if (v_ratios.size() > 0)
				{
					sort(v_ratios.begin(), v_ratios.end(), greaterEqualSort);
					int item = v_ratios[0].second.first;
					itemSet.set(item);
					sumProfit = 0;
					bs_itemsBitset::BitIterator iter(itemSet);
					while (iter++)
					{
						sumProfit += o_instance.Pi((int)*iter);
					}
					map_allBins[itemSet] = 1;
					if (o_instance.CostBin() - sumProfit - o_instance.Mu() <= -pricing_tolerance)
					{
						map_negRDCBins[itemSet] = o_instance.CostBin() - sumProfit - o_instance.Mu();
					}
				}
				else
				{
					recalculateRatios = false;	
				}
			}
		}
	}
#else 
	vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = o_instance.GetSortedGroupDecisions();
	
	for (int g = 0; g < o_instance.LastStage(); ++g)
	{
		vector<c_Group_Decision>& decisions = group_decisions[g].second;
		for (int i = 1; i < (int)decisions.size(); ++i)
		{
			itemSet.reset();
			elementSet.reset();
			bs_stageList.reset();
			double sumProfit = 0;
			int weightInit = o_instance.WeightElements(decisions[i].bs_elements);

			if (weightInit <= o_instance.Capacity() && map_allBins.find(decisions[i].bs_items) == map_allBins.end())
			{
				itemSet = decisions[i].bs_items;
				elementSet = decisions[i].bs_elements;
				sumProfit = decisions[i].d_sumDual;			
				bs_stageList.set(g);
				map_allBins[itemSet] = 1;
				if (o_instance.CostBin() - sumProfit - o_instance.Mu() <= -pricing_tolerance)
				{
					map_negRDCBins[itemSet] = o_instance.CostBin() - o_instance.Mu() - sumProfit;
				}
				bool recalculateRatios = true;
				while (recalculateRatios)
				{
					v_ratios.clear();
					for (int d = 0; d < o_instance.LastStage(); ++d)
					{
						if (!bs_stageList.test(d))
						{
							vector<c_Group_Decision>& decisions = group_decisions[d].second;
							double ratio = -1;
							int decisionIndexL = -1;
							for (int l = 1; l < (int)decisions.size(); ++l)
							{
								if (o_instance.WeightItems(itemSet | decisions[l].bs_items) <= o_instance.Capacity() &&
									(map_allBins.find((itemSet | decisions[l].bs_items)) == map_allBins.end()))
								{
									double additionalWeight = o_instance.WeightElements(decisions[l].bs_elements & ~elementSet) + 0.0001; //add 0.0001 to avoid denominator to be 0											
									if (ratio < decisions[l].d_sumDual / additionalWeight)
									{
										ratio = decisions[l].d_sumDual / additionalWeight;
										decisionIndexL = l;
									}
								}
							}
							if (ratio > -1)
							{
								v_ratios.push_back(make_pair(ratio, make_pair(d, decisionIndexL)));	
							}
						}
					}
					if (v_ratios.size() > 0)
					{
						sort(v_ratios.begin(), v_ratios.end(), greaterEqualSort);
						int stage = v_ratios[0].second.first;
						int dec = v_ratios[0].second.second;
						vector<c_Group_Decision>& decisions = group_decisions[stage].second;
						bs_itemsBitset addItems = decisions[dec].bs_items;
						bs_elementsBitset addElements = decisions[dec].bs_elements;
						itemSet |= addItems;
						elementSet |= addElements;
						bs_stageList.set(stage);
						sumProfit = 0;
						bs_itemsBitset::BitIterator iter(itemSet);
						while (iter++)
						{
							sumProfit += o_instance.Pi((int)*iter);
						}
						map_allBins[itemSet] = 1;
						if (o_instance.CostBin() - sumProfit - o_instance.Mu() <= -pricing_tolerance)
						{
							map_negRDCBins[itemSet] = o_instance.CostBin() - sumProfit - o_instance.Mu();
						}
					}
					else
						recalculateRatios = false;
				}
			}
		}
	}
#endif //LabelingOnElements
	
	for (auto& i : map_negRDCBins)
	{
		double rdc = i.second;
		itemSet = i.first;
		if (rdc > -pricing_tolerance)
		{
			continue;
		}
		else
		{
			new_columns.push_back(itemSet);
			new_rdc.push_back(rdc);
		}
	}

	return (int)new_columns.size();
}

#ifdef UseBoundingInLabeling
void c_Pricing_Solver_SUBP::ComputeGlobalBounds()
{
	int source = 0;
	int sink = o_instance.LastStage();
	const vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = o_instance.GetSortedGroupDecisions();

	bool optimalPricing = true;
#ifdef UseLabelingBoundGlobalSumDuals
	vector<double>& boundsGlobalSumDual = o_instance.GetGlobalBoundSumDuals();
	for (int i = 0; i < boundsGlobalSumDual.size(); i++)
		boundsGlobalSumDual[i] = -INFINITY;
#ifdef LabelingOnElements
	bs_itemsBitset futureItems;
	for (int stage = sink - 1; stage > 0; stage--)
	{
		const vector<c_Group_Decision>& decisions = group_decisions[stage].second;
		for (int k = 0; k < (int)decisions.size(); k++)
			futureItems |= o_instance.ItemsOfElements(decisions[k].bs_elements);
		double sum = 0;
		bs_itemsBitset::BitIterator it(futureItems);
		while (it++)
			sum += o_instance.Pi((int)*it);
		boundsGlobalSumDual[stage] = sum;
	}
#else
	double sum = 0;
	for (int stage = sink - 1; stage > 0; stage--)
	{
		const vector<c_Group_Decision>& decisions = group_decisions[stage].second;
		double maxDualStage = 0;
		for (int k = 0; k < (int)decisions.size(); k++)
			if (decisions[k].d_sumDual > maxDualStage)
				maxDualStage = decisions[k].d_sumDual;
		sum += maxDualStage;
		boundsGlobalSumDual[stage] = sum;
	}
#endif // LabelingOnElements
#endif // UseLabelingBoundGlobalSumDuals
}
#endif

void c_Pricing_Solver_SUBP::tree(const vector<int>& base_set, int base_set_pos, const vector<bs_itemsBitset>& conflicts_bs, vector<bs_itemsBitset>& result)
{
	if (base_set_pos == (int)base_set.size())
		return;
	int i = base_set[base_set_pos];
	int oldPos = (int)result.size();
	for (int rr = 0; rr < oldPos; ++rr)
	{
		if ((conflicts_bs[i] & result[rr]).none())
		{
			bs_itemsBitset new_set(result[rr]);
			new_set.set(i);
			result.push_back(new_set);
		}
	}
	bs_itemsBitset set_i;
	set_i.set(i);
	result.push_back(set_i);
	base_set_pos++;
	tree(base_set, base_set_pos, conflicts_bs, result);
}

void c_Pricing_Solver_SUBP::all_subsets(const vector<int>& base_set, const vector<bs_itemsBitset>& conflicts_bs, vector<bs_itemsBitset>& result)
{
	tree(base_set, 0, conflicts_bs, result);
}

void c_Pricing_Solver_SUBP::bs_component_items(c_QuickUnionFind& qu, int p, bs_itemsBitset& comp)
{
	int j = qu.root(p);
	for (int q = 0; q < (int)qu.id.size(); q++)
		if (qu.root(q) == j)
			comp.set(q);
}
