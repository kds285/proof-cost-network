#pragma once

typedef unsigned long long HashKey;

class OAEntry {
public:
	OAEntry() : m_isFree(true) {}
	HashKey m_key;
	bool m_isFree;
};

class HashTable {
	typedef unsigned int IndexType;

private:
	const IndexType m_mask;
	const size_t m_size;
	vector<OAEntry> m_vEntry;

public:
	HashTable(int bitSize = 12)
		: m_mask((1 << bitSize) - 1)
		, m_size(1 << bitSize)
	{
		m_vEntry.resize(1ULL << bitSize, OAEntry());
	}

	~HashTable() {}

	bool lookup(HashKey key) const {
		IndexType index = static_cast<IndexType>(key)&m_mask;

		while (true) {
			const OAEntry& entry = m_vEntry[index];
			if (!entry.m_isFree) {
				if (entry.m_key == key) { return true; } else { index = (index + 1)&m_mask; }
			} else {
				return false;
			}
		}
		return false;
	}

	void store(HashKey key) {
		IndexType index = static_cast<IndexType>(key)&m_mask;
		while (true) {
			assert(("index out of boundary",index<m_vEntry.size()));
			OAEntry& entry = m_vEntry[index];

			if (entry.m_isFree) {
				entry.m_key = key;
				entry.m_isFree = false;
				break;
			} else {
				index = (index + 1)&m_mask;
			}
		}
	}

	void erase(HashKey key) {
		assert(("key not in hash table", lookup(key)));
		IndexType index = static_cast<IndexType>(key)&m_mask;
		while (true) {
			OAEntry& entry = m_vEntry[index];

			if (entry.m_key == key) {
				entry.m_isFree = true;
				return;
			}
			index = (index + 1)&m_mask;
		}
	}

	void clear() {
		for (unsigned int i = 0; i < m_size; ++i) {
			m_vEntry[i].m_isFree = true;
		}
	}
};
