#ifndef SUBP_INSTANCE_H
#define SUBP_INSTANCE_H

#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "bitfield.h"
#include "boost/dynamic_bitset.hpp"
#include "json.hpp"
#include "unordered_dense.h"

#include "subp_settings.h"

namespace std {
	template<size_t size>
	struct hash<BitField<size> >
	{
		size_t operator()(const BitField<size>& bf) const
		{
			uint64_t h = size;
			for (size_t i = 0; i < BitField<size>::_words; ++i)
				h ^= ankerl::unordered_dense::detail::wyhash::hash(bf._bits[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return static_cast<size_t>(h);
		}
	};
}

using json = nlohmann::json;
using namespace std;

void OutputBin(const bs_itemsBitset& bin);
void OutputElements(const bs_elementsBitset& elements);

class c_Group_Decision
{
public:
#ifndef LabelingOnElements
	bs_itemsBitset bs_items;
	double d_sumDual;
#endif // LabelingOnElements	
	bs_elementsBitset bs_elements;
	int i_weight;

public:
#ifndef LabelingOnElements
	c_Group_Decision(const bs_itemsBitset& items, double duals, int weight, const bs_elementsBitset& elements)
		: bs_items(items), d_sumDual(duals), i_weight(weight), bs_elements(elements)
	{
	}
	c_Group_Decision(const c_Group_Decision& orig)
		: bs_items(orig.bs_items), d_sumDual(orig.d_sumDual), i_weight(orig.i_weight), bs_elements(orig.bs_elements)
	{
	}
	~c_Group_Decision(void) { }
	void UpdateDuals(double dual){ d_sumDual = dual; }
	void OutputInOStream(ostream& s) const;
#else
	c_Group_Decision(const bs_elementsBitset& elements, int weight)
		: bs_elements(elements), i_weight(weight)
	{
	}
	c_Group_Decision(const c_Group_Decision& orig)
		: bs_elements(orig.bs_elements), i_weight(orig.i_weight)
	{
	}
	~c_Group_Decision(void) { }
	void OutputInOStream(ostream& s) const;
#endif // LabelingOnElements
};

class c_SUBP_Instance
{
	string s_instance;
	// internal representation is zero-based	
	int i_numItemsRaw;		// before preprocessing (original)
	int i_numItems;			// after preprocessing (reduced)
	int i_numElements;
	int i_capacity;
	int i_paginationUB = -1;		// UB from pagination instances
	double d_avgFrequency = -1;		// average frequency from pagination instances (each element is required by an average of x items)
	int i_cplexOpt = -1;			// cplex opt from pagination instances
	int i_itemMinSize = -1;			// min item size (cardinality) from pagination instances
	int i_itemMaxSize = -1;			// max item size (cardinality) from pagination instances
	double d_avgItemSize = -1;
	double i_cost = 1.0;
	json instance;
	double d_instNumber;	// denotes json nr (pagination) AND capacity ratio (SUKP)
	vector<bs_elementsBitset> v_bs_relationMatrix;
	vector<bs_itemsBitset> v_bs_ItemsOfElement;
	vector<double> v_profit;
	vector<int> v_weightElements;
	vector<int> v_weightItems;
	vector<int> v_frequencyElements;
	vector<int> v_frequencyItems;
	int i_maxNumItemsBin;
	int i_maxNumBinsInstance;
	int i_maxWeightItem;

	map<string, int> m_BKS;

#ifdef UseElementsOfItemsHashmap
	ankerl::unordered_dense::map< bs_itemsBitset, bs_elementsBitset, hash<bs_itemsBitset>> o_elementsOfItems;
#endif

public:
	c_SUBP_Instance(string filename, double json_number);

	string InstanceName() const { return s_instance; }
	int JSONNumber() const { return (int)d_instNumber; }
	double CapacityRatio() const { return d_instNumber; }	
	int NumItems() const { return i_numItems; }
	int NumElements() const { return i_numElements; }
	int Capacity() const { return i_capacity; }

	int PaginationInstUB() const { return i_paginationUB; }
	double AverageElementFrequency() const { return d_avgFrequency; }
	int MinCardItem() const { return i_itemMinSize; }
	int MaxCardItem() const { return i_itemMaxSize; }
	double AverageCardItem() const { return d_avgItemSize; }
	int OptCPLEX() const { return i_cplexOpt; }
	int WeightElement(int element) const { return v_weightElements[element]; }	
	int WeightElements(const bs_elementsBitset& elements);
	const vector<int>& WeightElements() const { return v_weightElements; }
	int WeightItem(int item) const { return v_weightItems[item]; }
	int WeightItems(const bs_itemsBitset& items);
	int MaxWeightItem() const { return i_maxWeightItem; }
	int FrequencyElement(int element) const { return v_frequencyElements[element]; }
	int FrequencyElements(const bs_elementsBitset& elements);
	int FrequencyItem(int item) const { return v_frequencyItems[item]; }
	double RelativeWeightElements(const bs_elementsBitset& elements);
	double CostBin() const { return i_cost; }

	const vector<bs_elementsBitset>& RelationMatrix() { return v_bs_relationMatrix; }
	bs_elementsBitset ElementsOfItem(int item) const { return v_bs_relationMatrix[item]; }
	const bs_elementsBitset& ElementsOfItemReference(int item) const { return v_bs_relationMatrix[item]; }
	bs_elementsBitset ElementsOfItems(const bs_itemsBitset& items);
	bs_itemsBitset ItemsOfElement(int element) const { return v_bs_ItemsOfElement[element];	}
	const bs_itemsBitset& ItemsOfElementReference(int element) const { return v_bs_ItemsOfElement[element];	}
	bs_itemsBitset ItemsOfElements(const bs_elementsBitset& elements);

	void StartHeuristic(vector<bs_itemsBitset>& bins, int iteration);
	int MaxNumBinsInstance() const { return i_maxNumBinsInstance; }
	int MinNumBinsInstance();
	int MinNumBinsHeuristics(const bs_itemsBitset& items);

	int BKS_UB();

	virtual void Go(void);
};

class c_SUBP_Instance_With_RDC : public c_SUBP_Instance {
	double d_currentTargetRDC;
	double d_mu;
	vector<double> v_rdc;
	int i_lastStage;

	vector< pair <pair<double, int>, vector<c_Group_Decision> > > v_group_decisions;

#ifdef UseBoundingInLabeling
	vector<double> v_GlobalBoundsSumDual;
#endif // UseBoundingInLabeling

public:

	c_SUBP_Instance_With_RDC(string filename, double json_nr);

	void SetCurrentTargetRDC(double rdc) { d_currentTargetRDC = rdc; }
	double CurrentTargetRDC() const { return d_currentTargetRDC; }	

	void SetMu(double mu) { d_mu = mu; }
	double Mu() const { return d_mu; }

	void SetPi(int i, double rdc) { v_rdc[i] = rdc; }
	double Pi(int i) const { return v_rdc[i]; }

	int LastStage() const { return i_lastStage; }
	void SetLastStage(int stage) { i_lastStage = stage; }

#ifdef LabelingOnElements
	bs_itemsBitset FeasibleItemsFromAddedElements(const bs_elementsBitset& totalElements, const bs_elementsBitset& addedElements, const bs_itemsBitset& compatibleItems);
	bs_itemsBitset CompatibleItems(const bs_elementsBitset& newLabelElements, const bs_itemsBitset& oldCompatibleItems, int stage);
	bs_elementsBitset ElementsOfStage(int stage);
#else
	bs_itemsBitset CompatibleItems(const bs_elementsBitset& newLabelElements, const bs_itemsBitset& oldCompatibleItems, int stage);
	bs_itemsBitset ItemsOfStage(int stage);
#endif // LabelingOnElements
	
	vector< pair <pair<double, int>, vector<c_Group_Decision> > >& GetSortedGroupDecisions() { return v_group_decisions; }

#ifdef UseBoundingInLabeling
	void InitBoundingMatrices();
	double GetGlobalBoundSumDuals(int stage) const { return v_GlobalBoundsSumDual[stage]; }
	vector<double>& GetGlobalBoundSumDuals() { return v_GlobalBoundsSumDual; }
	double ComputeLabelingBoundRelaxedSUKP(const bs_elementsBitset& elements, const bs_itemsBitset& compatibleItems, int load);
	double ComputeLabelingBoundSumDuals(const bs_itemsBitset& compatibleItems);
	double ComputeLabelingBoundDP(const bs_elementsBitset& elements, const bs_itemsBitset& compatibleItems, int stage, int load);
#endif // UseBoundingInLabeling

};

#endif // SUBP_INSTANCE_H