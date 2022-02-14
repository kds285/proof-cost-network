#pragma once

#include "OpenAddressHashTable.h"
#include "TreeNode.h"
using namespace std;

class TTentry
{
public:
	TTentry() { clear(); }
	void clear() {
		m_solutionStatus = SOLUTION_UNKNOWN;
		return;
	};

public:	
	SOLUTION_STATUS m_solutionStatus;
};

class TranspositionTable {

public:
	TranspositionTable() : m_table(23) { clear(); }

public:
	inline void clear() {
		m_table.clear();
	}
	inline uint lookup(HashKey hashkey) {
		uint index = m_table.lookup(hashkey);
		return index;
	}
	inline void store(HashKey hashkey, TTentry entry) {
		m_table.store(hashkey, entry);
	}
	inline TTentry& getEntry(uint index) { return m_table.m_entry[index].m_data; }
	inline uint getSize() { return m_table.getCount(); }
	inline bool isFull() const { return m_table.isFull(); }

private:
	OpenAddressHashTable<TTentry> m_table;
};
