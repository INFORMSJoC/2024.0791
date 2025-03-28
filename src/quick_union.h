#ifndef QUICK_UNION_H
#define QUICK_UNION_H

#include <vector>
#include <set>

namespace quick_union {

class c_QuickUnionFind {
	//
	// code taken from presentation
	// http://www.cs.princeton.edu/~rs/AlgsDS07/01UnionFind.pdf
	// 
public:
	std::vector<int> id;
	std::vector<int> sz;
public:
	c_QuickUnionFind(int N) : id( N ), sz( N )
	{
		reset();
	}
	void reset() 
	{
		for (int i = 0; i <(int)id.size(); i++) 
		{
			id[i] = i;
			sz[i] = 1;
		}
	}
	void reset( int i ) 
	{
		id[i] = i;
		sz[i] = 1;
	}
	int root(int i)
	{
		while (i != id[i]) 
		{
			id[i] = id[id[i]];
			i = id[i];
		}
		return i;
	}
	bool find(int p, int q)
	{
		return root(p) == root(q);
	}
	void merge(int p, int q)
	{
		int i = root(p);
		int j = root(q);
		if ( i==j )
			return;
		if ( sz[i] < sz[j] )
		{
			id[i] = j;
			sz[j] += sz[i];
		}
		else
		{
			id[j] = i;
			sz[i] += sz[j];
		}
	}
	int component_size( int i ) { return sz[root(i)]; }
	std::set<int> component(int p)
	{
		int j = root(p);
		std::set<int> comp;
		for (int q = 0; q<(int)id.size(); q++)
			if (root(q) == j)
				comp.insert(q);
		return comp;
	}
};

} // end of namespace

#endif // of QUICK_UNION_H
