#ifndef SUBP_SETTINGS_H
#define SUBP_SETTINGS_H

#define UseBoundingInLabeling	
#define UseElementsOfItemsHashmap

// define settings here:
/////////////////////////////////////////////////////////////////////////////
// 
//// instance type 
// #define UnionWeightInstance		// activate for pagination, deactivate for general weight instances
// 
//// labeling features
// #define UseLabelingWithDominance	// apply dominance relations (deactivate for standard labeling)
// #define LabelingOnElements		// element-based labeling (deactivate for standard labeling)
//
//// completion bounds for labeling
#define UseLabelingBoundDP					// knapsack DP with modified item weight (B_1)
#define UseLabelingBoundSumCompDuals		// sum duals of all compatible items (B_2)
// #define UseLabelingBoundGlobalSumDuals	// sum duals of all remaining items (B_3)
// #define UseLabelingBoundRelaxedSUKP		// solve LP relaxation of SUKP with compatible items (B_4)
// 
//////////////////////////////////////////////////////////////////////////////

#ifdef UnionWeightInstance
const int maxNumElements = 128;
const int maxNumItems = 128;
#else
const int maxNumElements = 512;
const int maxNumItems = 512;
#endif // UnionWeightInstance
typedef BitField<maxNumElements> bs_elementsBitset;
typedef BitField<maxNumItems> bs_itemsBitset;

// start heuristic
const double NoiseFactorStartHeuristic = 0.15;

// pricing general
const double pricing_tolerance = 0.001;
const double MaxNumNegRedCostPathFactor = 0.35;

// pricing approach
const bool UsePricingHeuristic = true;
const int PricingApproach = 0;	// 0: labeling, 1: MIP

#endif // of SUBP_SETTINGS_H
