#include "subp_instance.h"
#include "quick_union.h"
#include "random.h"
#include <fstream> 
#include <sstream>

#include "subp_relaxSUKP.h"

using namespace std;
using namespace quick_union;

vector<int> bin;

void OutputBin(const bs_itemsBitset& bin)
{
	for (int i = 0; i < maxNumItems; ++i)
		if (bin.test(i))
			cout << i << " ";
}

void OutputElements(const bs_elementsBitset& elements)
{
	for (int i = 0; i < maxNumElements; ++i)
		if (elements.test(i))
			cout << i << " ";
}

void c_Group_Decision::OutputInOStream(ostream& s) const
{
	s << "*********************" << endl;
#ifdef LabelingOnElements
	s << "Elements: ";
	OutputBin(bs_elements);
	s << endl;
	s << "Weight: " << i_weight << endl;

#else
	s << "Items: ";
	OutputBin(bs_items);
	s << endl;
	s << "Weight: " << i_weight << endl;
	s << "SumDual: " << d_sumDual << endl;
#endif // LabelingOnElements
}


/////////////////////////////////////////////////////////////////////////////
// c_SUBP_Instance
/////////////////////////////////////////////////////////////////////////////

c_SUBP_Instance::c_SUBP_Instance(string filename, double json_number)
	: d_instNumber(json_number)
{
	string fnend = filename.substr(filename.find_last_of(".") + 1);

	string delimiter = "/";
	size_t pos_start = 0, pos_end, delim_len = delimiter.length();
	string token;
	while ((pos_end = filename.find(delimiter, pos_start)) != string::npos)
	{
		token = filename.substr(pos_start, pos_end - pos_start);
		pos_start = pos_end + delim_len;
	}
	s_instance = filename.substr(pos_start);

	//----------------------------------------------------------------------------------
	// read inst from txt file---------------------------------------------------------
	//----------------------------------------------------------------------------------

	if (fnend == "txt")
	{
		ifstream infile(filename);
		if (!infile.good()) {
			cout << "File " << filename << " cannot be opened.  Serious error." << endl;
			throw;
		}
		string reading_string;
		int reading_int;

		infile >> reading_string;	// "m="
		i_numItemsRaw = atoi(reading_string.substr(2, reading_string.size()).c_str()); // num items (m)
		infile >> reading_string;	// "n="
		i_numElements = atoi(reading_string.substr(2, reading_string.size()).c_str()); // num elements (n)
		infile >> reading_string;	// "knapsack"
		infile >> reading_string;	// "capacity="
		i_capacity = atoi(reading_string.substr(5, reading_string.size()).c_str()); // capacity
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		for (int i = 0; i < i_numItemsRaw; i++) {
			infile >> reading_int;
			v_profit.push_back((double)reading_int);  //Profit per item
		}
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		infile >> reading_string;
		for (int i = 0; i < i_numElements; i++) {
			infile >> reading_int;
			v_weightElements.push_back(reading_int);  //Weight per element
		}
		infile >> reading_string;
		infile >> reading_string;

		// Relation matrix (m x n)
		v_bs_relationMatrix.resize(i_numItemsRaw);
		v_bs_ItemsOfElement.resize(i_numElements);
		v_frequencyElements.resize(i_numElements, 0);
		for (int i = 0; i < i_numItemsRaw; i++)
		{
			for (int e = 0; e < i_numElements; e++)
			{
				infile >> reading_int;
				if (reading_int == 1) {
					v_bs_relationMatrix[i].set(e);
					v_bs_ItemsOfElement[e].set(i);
					v_frequencyElements[e] += 1;
				}
			}
		}

		// redefine capacity according to input factor 
		i_capacity = -1;
	}

	//----------------------------------------------------------------------------------
	// read inst from json file--------------------------------------------------------
	//----------------------------------------------------------------------------------

	else if (fnend == "json")
	{
		ifstream infile(filename);
		if (!infile.good()) {
			cout << "File " << filename << " cannot be opened.  Serious error." << endl;
			throw;
		}
		infile >> instance;
		json one_instance = instance[(int)json_number]; // Json from infile

		i_capacity = one_instance.at("capacity");
		i_paginationUB = one_instance.at("pageCount");
		i_numItemsRaw = one_instance.at("tileCount");
		i_numElements = one_instance.at("symbolCount");
		if (one_instance.find("cplexOpt") != one_instance.end())
			if(one_instance.at("cplexOpt").is_number())
				i_cplexOpt = one_instance.at("cplexOpt");
		d_avgFrequency = one_instance.at("avgMultiplicity");
		i_itemMinSize = one_instance.at("tileMinSize");
		i_itemMaxSize = one_instance.at("tileMaxSize");

		for (int i = 0; i < i_numElements; i++)
			v_weightElements.push_back(1);

		v_bs_relationMatrix.resize(i_numItemsRaw);
		v_bs_ItemsOfElement.resize(i_numElements);
		v_frequencyElements.resize(i_numElements, 0);
		for (int i = 0; i < i_numItemsRaw; i++) {
			int element = -1;
			for (int k = 0; k < one_instance.at("tiles")[i].size(); k++) {
				element = one_instance.at("tiles")[i][k];
				v_bs_relationMatrix[i].set(element);
				v_bs_ItemsOfElement[element].set(i);
				v_frequencyElements[element] += 1;
			}
		}
	}

	else {
		cout << "keine Datei eingelesen!" << endl;
		return;
	}
	i_numItems = i_numItemsRaw;

	i_maxWeightItem = 0;
	for (int i = 0; i < i_numItems; i++)
	{
		int i_weight = 0;
		int i_frequency = 0;
		int i_cardinality = 0;		

		bs_elementsBitset::BitIterator it_element(v_bs_relationMatrix[i]);
		while (it_element++)
		{
			i_weight += v_weightElements[(int)*it_element];
			i_frequency += v_frequencyElements[(int)*it_element];
			i_cardinality++;
		}		
		if (i_weight > i_maxWeightItem)
			i_maxWeightItem = i_weight;
		v_weightItems.push_back(i_weight);
		v_frequencyItems.push_back(i_frequency);
		d_avgItemSize += i_cardinality;
	}
	d_avgItemSize /= i_numItems;

	if (d_avgFrequency < 0)	
	{
		d_avgFrequency = 0;
		for (int e = 0; e < i_numElements; e++)
			d_avgFrequency += v_frequencyElements[e];
		d_avgFrequency /= i_numElements;
	}	

	// redefine capacity according to input factor (SUKP instances)
	if (i_capacity < 0)
	{
		int maxElement = *max_element(v_weightElements.begin(), v_weightElements.end());
		i_capacity = (int)(json_number * maxElement) + i_maxWeightItem;
	}

	i_maxNumBinsInstance = i_numItems;
}

bs_elementsBitset c_SUBP_Instance::ElementsOfItems(const bs_itemsBitset& items)
{
#ifdef UseElementsOfItemsHashmap
	auto entry(o_elementsOfItems.find(items));
	if (entry != o_elementsOfItems.end())
		return entry->second;
	else
	{
		bs_elementsBitset elements;
		bs_itemsBitset::BitIterator it_item(items);
		while (it_item++)
			elements |= ElementsOfItemReference((int)*it_item);
		o_elementsOfItems[items] = elements;
		return elements;
	}
#else
	bs_elementsBitset elements;
	bs_itemsBitset::BitIterator it_item(items);
	while (it_item++)
		elements |= ElementsOfItemReference((int)*it_item);
	return elements;
#endif
}

int c_SUBP_Instance::WeightElements(const bs_elementsBitset& elements)
{
	int weight = 0;
#ifdef UnionWeightInstance
	weight = (int)elements.count();
#else
	bs_elementsBitset::BitIterator it_element(elements);
	while (it_element++)
		weight += WeightElement((int)*it_element);
#endif // UnionWeightInstance
	return weight;
}

int c_SUBP_Instance::WeightItems(const bs_itemsBitset& items)
{
#ifdef UnionWeightInstance
	return (int)ElementsOfItems(items).count();
#else
	return WeightElements(ElementsOfItems(items));
#endif // UnionWeightInstance
}

int c_SUBP_Instance::FrequencyElements(const bs_elementsBitset& elements)
{
	int freq = 0;
	bs_elementsBitset::BitIterator iter(elements);
	while (iter++)
		freq += FrequencyElement((int)*iter);
	return freq;
}

double c_SUBP_Instance::RelativeWeightElements(const bs_elementsBitset& elements)
{
	double relWeight = 0;
	bs_elementsBitset::BitIterator iter(elements);
	while (iter++)
#ifdef UnionWeightInstance
		relWeight += 1 / (double)FrequencyElement((int)*iter);
#else
		relWeight += WeightElement((int)*iter) / (double)FrequencyElement((int)*iter);
#endif // UnionWeightInstance
		
	return relWeight;
}

bs_itemsBitset c_SUBP_Instance::ItemsOfElements(const bs_elementsBitset& elements)
{
	bs_itemsBitset itemsSet;
	bs_elementsBitset::BitIterator el(elements);
	while (el++)
		itemsSet |= ItemsOfElement((int)*el);

	return itemsSet;
}

void c_SUBP_Instance::StartHeuristic(vector<bs_itemsBitset>& bins, int iteration)
{	
	c_Random random(-iteration);
	bs_itemsBitset bs_bin;
	bs_itemsBitset bs_remainingItems;
	for (int i = 0; i < NumItems(); i++)
		bs_remainingItems.set(i);

	int i_maxWeight = -1;
	int i_seed = -1;
	bs_itemsBitset::BitIterator iter(bs_remainingItems);
	while (iter++)
	{
		if (WeightItem((int)*iter) > i_maxWeight)
		{
			i_maxWeight = WeightItem((int)*iter);
			i_seed = (int)*iter;
		}
	}

	while (bs_remainingItems.any())
	{
		bs_bin.set(i_seed);
		bs_remainingItems.reset(i_seed);

		vector<pair<double, int> > v_criterion;	
		bs_itemsBitset::BitIterator it(bs_remainingItems);
		while (it++)
		{
			if (WeightItems(bs_bin.set((int)*it)) <= Capacity())
			{
				double criterion = -1;				
				double ran = 1 + random.dran(-NoiseFactorStartHeuristic, NoiseFactorStartHeuristic);
				criterion = WeightElements(ElementsOfItems(bs_bin) & ElementsOfItemReference((int)*it));
				v_criterion.push_back(make_pair(ran * criterion, (int)*it));
			}
			bs_bin.reset((int)*it);
		}
		if (!v_criterion.empty())
		{
			sort(v_criterion.begin(), v_criterion.end(), std::greater<>());
			bs_bin.set(v_criterion[0].second);
			bs_remainingItems.reset(v_criterion[0].second);
			if (bs_remainingItems.none())
				bins.push_back(bs_bin);
		}
		else
		{
			bins.push_back(bs_bin);
			bs_bin.reset();		

			i_seed = -1;
			i_maxWeight = -1;
			bs_itemsBitset::BitIterator iter(bs_remainingItems);
			while (iter++)
			{
				if (WeightItem((int)*iter) > i_maxWeight)
				{
					i_maxWeight = WeightItem((int)*iter);
					i_seed = (int)*iter;
				}
			}
		}
	}

	if (i_maxNumBinsInstance > (int)bins.size())
		i_maxNumBinsInstance = (int)bins.size();
}

int c_SUBP_Instance::MinNumBinsInstance()
{
	bs_itemsBitset all;
	for (int i = 0; i < NumItems(); ++i)
		all.set(i);
	return MinNumBinsHeuristics(all);
}

int c_SUBP_Instance::MinNumBinsHeuristics(const bs_itemsBitset& items)
{
	if (items.count() == 0)
		return 0;
	if (items.count() == 1)
		return 1;

	int greedy = 0;
	int sw = 0;
	int msw = 0;

	int weight = WeightItems(items);
	greedy = (int) ceil( weight / (double)Capacity() );
	
	// (modified) sweeping procedure (Tang Denardo / Crama Oerlemans)
	vector<bs_itemsBitset> v_bs_compatibleItems(NumItems());
	vector<double> v_modifiedSweeping;
	bs_itemsBitset bs_remainingItems = items;
	int numRemaining = (int)items.count();
	while(bs_remainingItems.count() > 0)
	{	
		bs_itemsBitset dummy;
		bs_itemsBitset::BitIterator iter_1(bs_remainingItems);
		while (iter_1++)
		{
			int i = (int)*iter_1;
			dummy.set(i);
			bs_itemsBitset::BitIterator iter_2(bs_remainingItems);
			iter_2.set(i);
			while (iter_2++)
			{
				int j = (int)*iter_2;
				dummy.set(j);
				if (WeightItems(dummy) <= Capacity())
				{
					v_bs_compatibleItems[i].set(j);
					v_bs_compatibleItems[j].set(i);
				}
				dummy.reset(j);
			}
			iter_2.reset();
			dummy.reset(i);
		}
		int minCompatible = NumItems();
		int minWeightCompatible = WeightItems(items) + 1;
		int seedItem = -1;
		bs_itemsBitset::BitIterator iter(bs_remainingItems);
		while (iter++)
		{
			int i = (int)*iter;
			if (v_bs_compatibleItems[i].count() < minCompatible)
			{
				minCompatible = (int)v_bs_compatibleItems[i].count();
				seedItem = i;
				minWeightCompatible = WeightItems(v_bs_compatibleItems[i]);
			}
			else if (minCompatible == v_bs_compatibleItems[i].count())
			{
				int weightComp = WeightItems(v_bs_compatibleItems[i]);
				if (weightComp < minWeightCompatible)
				{
					minWeightCompatible = weightComp;
					seedItem = i;
				}
			}
		}
		bs_remainingItems &= ~v_bs_compatibleItems[seedItem]; 
		bs_remainingItems.reset(seedItem);
		sw++;
		v_modifiedSweeping.push_back(sw + ceil(WeightItems(bs_remainingItems) / Capacity()));
	}
	if (v_modifiedSweeping.size() > 0)
		msw = (int)*max_element(std::begin(v_modifiedSweeping), std::end(v_modifiedSweeping));

	return max(greedy, max(sw, msw));
}

int c_SUBP_Instance::BKS_UB()
{
	string inst = s_instance;
	inst += "_" + to_string(JSONNumber());
	auto found = m_BKS.find(inst);
	if (found != m_BKS.end())
		return found->second;
	else
	{
		cout << "BKS is not known for instance " << inst << endl;
		throw;
	}

}

void c_SUBP_Instance::Go(void)
{
	bs_itemsBitset items;

	i_maxNumItemsBin = 0;
	int binWeight = 0;
	vector<bs_elementsBitset> v_newItems = RelationMatrix();
	vector<pair<int, int> > v_weightNewItems(NumItems());
	v_weightNewItems[0] = make_pair(WeightItem(0), 0);
	for (int i = 1; i < NumItems(); i++)
	{
		for (int j = 0; j < i; j++)
		{
			v_newItems[i] = v_newItems[i] & ~v_newItems[j];
		}
		v_weightNewItems[i] = make_pair(WeightElements(v_newItems[i]), i);
	}
	sort(v_weightNewItems.begin(), v_weightNewItems.end());
	
	for (int i = 0; i < NumItems(); i++)
	{
		if (binWeight + v_weightNewItems[i].first <= Capacity())
		{
			binWeight += v_weightNewItems[i].first;
			i_maxNumItemsBin++;
		}
		else
			break;
	}
	bin.reserve(i_maxNumItemsBin);
}


/////////////////////////////////////////////////////////////////////////////
// c_SUBP_Instance_With_RDC
/////////////////////////////////////////////////////////////////////////////

c_SUBP_Instance_With_RDC::c_SUBP_Instance_With_RDC(string filename, double json_nr)
	: c_SUBP_Instance(filename, json_nr),
	i_lastStage(-1)
{
	v_rdc.resize(NumItems());

#ifdef LabelingOnElements
	v_group_decisions.reserve(NumElements());
#else
	v_group_decisions.reserve(NumItems());

#endif // LabelingOnElements
}

#ifdef LabelingOnElements
bs_itemsBitset c_SUBP_Instance_With_RDC::CompatibleItems(const bs_elementsBitset& elements, const bs_itemsBitset& oldCompatibleItems, int stage) 
{
	bs_itemsBitset compatibleItems = oldCompatibleItems;
	bs_itemsBitset::BitIterator iter(oldCompatibleItems);
	while (iter++ && (int)*iter < NumItems())
	{
		if (WeightElements(elements | ElementsOfItem((int)*iter)) > Capacity() )
			compatibleItems.reset((int)*iter);
	}
	return compatibleItems;
}

bs_itemsBitset c_SUBP_Instance_With_RDC::FeasibleItemsFromAddedElements(const bs_elementsBitset& totalElements, const bs_elementsBitset& addedElements, const bs_itemsBitset& compatibleItems)
{
	bs_itemsBitset feasibleItems;
	bs_elementsBitset::BitIterator element(addedElements);
	while (element++)
	{
		bs_itemsBitset items = (ItemsOfElementReference((int)*element) & compatibleItems);
		bs_itemsBitset::BitIterator iter(items);
		while (iter++)
			if ((ElementsOfItem((int)*iter) & totalElements) == ElementsOfItem((int)*iter))
				feasibleItems.set((int)*iter);
	}
	return feasibleItems;
}

bs_elementsBitset c_SUBP_Instance_With_RDC::ElementsOfStage(int stage) 
{
	const vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = GetSortedGroupDecisions();
	const vector<c_Group_Decision>& decisions = group_decisions[stage - 1].second;
	bs_elementsBitset elements;
	for (int j = 1; j < (int)decisions.size(); j++)
	{
		elements |= decisions[j].bs_elements;
	}
	return elements;
}

#else

bs_itemsBitset c_SUBP_Instance_With_RDC::CompatibleItems(const bs_elementsBitset& elements, const bs_itemsBitset& oldCompatibleItems, int stage)
{
	bs_itemsBitset compatibleItems;
	const vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = GetSortedGroupDecisions();	
	for (int i = stage; i < (int)group_decisions.size(); ++i)
	{
		const vector<c_Group_Decision>& decisions = group_decisions[i].second;
		for (int j = 1; j < (int)decisions.size(); j++)	
			if ((decisions[j].bs_items & ~oldCompatibleItems).none())
			{
				int weight = 0;
#ifdef UnionWeightInstance
				weight = (int) (elements | decisions[j].bs_elements).count();
#else
				weight = WeightElements((elements | decisions[j].bs_elements));
#endif // UnionWeightInstance
				if (weight <= Capacity())
					compatibleItems |= decisions[j].bs_items;
			}
	}
	return compatibleItems;
}

bs_itemsBitset c_SUBP_Instance_With_RDC::ItemsOfStage(int stage)
{
	const vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = GetSortedGroupDecisions();
	int i_stage = stage - 1;
	const vector<c_Group_Decision>& decisions = group_decisions[i_stage].second;
	bs_itemsBitset items;
	for (int j = 1; j < (int)decisions.size(); j++)
		items |= decisions[j].bs_items;

	return items;
}
#endif // LabelingOnElements

#ifdef UseBoundingInLabeling
void c_SUBP_Instance_With_RDC::InitBoundingMatrices()
{
#ifdef LabelingOnElements
	v_GlobalBoundsSumDual.resize(NumElements(), -1.0);
#else
	v_GlobalBoundsSumDual.resize(NumItems(), -1.0);
#endif // LabelingOnElements
}

#ifdef UseLabelingBoundSumCompDuals
double c_SUBP_Instance_With_RDC::ComputeLabelingBoundSumDuals(const bs_itemsBitset& compatibleItems)
{
	double bound_sumDuals = 0;
	bs_itemsBitset::BitIterator iter(compatibleItems);
	while (iter++ && (int)*iter < NumItems())
		bound_sumDuals += Pi((int)*iter);
	return bound_sumDuals;
}
#endif // UseLabelingBoundSumCompDuals

#ifdef UseLabelingBoundRelaxedSUKP
double c_SUBP_Instance_With_RDC::ComputeLabelingBoundRelaxedSUKP(const bs_elementsBitset& elements, const bs_itemsBitset& compatibleItems, int load)
{
	int resCap = Capacity() - load;
	vector<double> v_profit;
	vector<bs_elementsBitset> v_relation;

	bs_itemsBitset::BitIterator iter(compatibleItems);
	while (iter++ && (int)*iter < NumItems())
	{
		int i = (int)*iter;
		v_profit.push_back(Pi(i));
		v_relation.emplace_back(ElementsOfItemReference(i) & ~elements);
	}

	c_relaxSUKP relaxModel((int)v_relation.size(), NumElements(), resCap, v_profit, WeightElements(), v_relation);

	return relaxModel.Solve();
}
#endif // UseLabelingBoundRelaxedSUKP

#ifdef UseLabelingBoundDP
double c_SUBP_Instance_With_RDC::ComputeLabelingBoundDP(const bs_elementsBitset& elements, const bs_itemsBitset& compatibleItems, int stage, int load)
{
	int factorDP = 10;

	vector<double> v_relWeightElements(NumElements(), 0);
	bs_elementsBitset elementsNotIncluded = ~elements;
	bs_elementsBitset::BitIterator el(elementsNotIncluded);
	while (el++ && (int)*el < NumElements())
	{
		int e = (int)*el;
		int freq = (int)(ItemsOfElement(e) & compatibleItems).count();
		if (freq > 0)
			v_relWeightElements[e] = WeightElement(e) / (double)freq;
	}

#ifdef LabelingOnElements
	vector<pair<int,int> > relWeightItem;
	bs_itemsBitset::BitIterator iter(compatibleItems);
	while (iter++ && (int)*iter < NumItems())
	{
		double weight = 0;
		bs_elementsBitset elementsOfItem = ElementsOfItemReference((int)*iter);
		bs_elementsBitset::BitIterator el(elementsOfItem);
		while (el++)
			weight += v_relWeightElements[(int)*el];
		relWeightItem.push_back(make_pair((int) floor(factorDP * weight), (int)*iter));
	}

	int i_resCap = Capacity() - load;
	vector<double> v_profit(i_resCap * factorDP + 1, 0);
	for (int d = 0; d < (int)relWeightItem.size(); d++)
	{
		int relWeight = relWeightItem[d].first;
		double dual = Pi(relWeightItem[d].second);
		for (int cap = i_resCap * factorDP; cap >= relWeight; cap--)
			if (v_profit[cap - relWeight] + dual > v_profit[cap])
				v_profit[cap] = v_profit[cap - relWeight] + dual;
	}
#else
	vector< pair <pair<double, int>, vector<c_Group_Decision> > >& group_decisions = GetSortedGroupDecisions();
	vector<vector<pair<int, c_Group_Decision*> > > compatibleGroups(group_decisions.size());

	for (int i = stage; i < (int)group_decisions.size(); i++)
	{
		vector<c_Group_Decision>& decisions = group_decisions[i].second;
		for (int j = 1; j < (int)decisions.size(); j++)
		{
			if (WeightElements(elements | decisions[j].bs_elements) <= Capacity())
			{
				double relWeightGroup = 0;
				bs_elementsBitset::BitIterator el(decisions[j].bs_elements);
				while (el++)
					relWeightGroup += v_relWeightElements[(int)*el];
				relWeightGroup = floor(factorDP * relWeightGroup);
				compatibleGroups[i].push_back(make_pair((int)relWeightGroup, &decisions[j]));
			}
		}
	}

	int i_resCap = Capacity() - load;
	int i_capEnlarged = i_resCap * factorDP;
	vector<double> v_profit(i_capEnlarged + 1, 0);
	for (int s = stage; s < (int)compatibleGroups.size(); s++)
	{
		for (int d = 0; d < (int)compatibleGroups[s].size(); d++)
		{
			int relWeight = compatibleGroups[s][d].first;
			double dual = compatibleGroups[s][d].second->d_sumDual;
			for (int cap = i_capEnlarged; cap >= relWeight; cap--)
			{
				int i_cap = cap - relWeight;
				if (v_profit[i_cap] + dual > v_profit[cap])
					v_profit[cap] = v_profit[i_cap] + dual;
			}
		}
	}
#endif // LabelingOnElements
	
	return v_profit.back();
}
#endif	//UseLabelingBoundDP
#endif	// Bounding in Labeling 