#pragma once

#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <climits>

using namespace std;

class SgfNode {
public:
	string m_comment;
	pair<string, string> m_move;
	vector<pair<string, string>> m_vPreset;

	SgfNode() {
		m_comment = "";
		m_move = {};
		m_vPreset.clear();
	}
};

class SgfLoader {
	string m_sFileName;
	string m_sSgfString;
	vector<SgfNode> m_vNode;
	map<string, string> m_tag;

public:
	SgfLoader() { clear(); }

	bool parseFromFile(const string& sgffile, int limit = INT_MAX) {
		this->clear();

		ifstream fin(sgffile.c_str());
		if (!fin) { return false; }

		string sgf;
		string line;
		while (getline(fin, line)) { sgf += line; }

		bool bIsSuccess = parseFromString(sgf, limit);
		m_sFileName = sgffile;

		return bIsSuccess;
	}

	bool parseFromString(const string& sgfString, int limit = INT_MAX) {
		this->clear();

		m_sSgfString = sgfString;
		int index = sgfString.find(";") + 1;
		while (index < sgfString.length()) {
			int endIndex = sgfString.find("];", index);
			if (endIndex == string::npos) { endIndex = sgfString.find("])", index); }
			if (endIndex == string::npos) { return false; }

			addSgfNode(sgfString, index, endIndex);
			index = endIndex + 2;

			if (m_vNode.size() > limit) { break; }
		}

		return true;
	}

	inline void addTag(const string sKey, string sValue) { m_tag[sKey] = sValue; }

	inline string getFileName() const { return m_sFileName; }
	inline string getSgfString() const { return m_sSgfString; }
	inline vector<SgfNode>& getSgfNode() { return m_vNode; }
	inline const vector<SgfNode>& getSgfNode() const { return m_vNode; }
	inline bool hasTag(const string& sTag) const { return m_tag.find(sTag) != m_tag.end(); }
	inline string getTag(const string& sTag) const { return hasTag(sTag) ? m_tag.at(sTag) : ""; }

private:
	bool addSgfNode(const string& sgfString, int start_index, int end_index) {
		SgfNode node;
		bool bracket = false;
		string sKey = "", sValue = "", sBuffer = "";
		for (int i = start_index; i <= end_index; ++i) {
			if (sgfString[i] == '[') {
				if (!bracket) {
					if (!sBuffer.empty()) { sKey = sBuffer; }
					bracket = true;
					sBuffer.clear();
				} else { return false; }
			} else if (sgfString[i] == ']') {
				sValue = sBuffer;
				bracket = false;
				sBuffer.clear();

				if (sKey == "B" || sKey == "W") { node.m_move = {sKey, sValue}; }
				else if (sKey == "C") { node.m_comment = sValue; }
				else if (sKey == "AB" || sKey == "AW") { node.m_vPreset.push_back({sKey, sValue}); }
				else { addTag(sKey, sValue); }
			} else {
				if (bracket) { sBuffer += sgfString[i]; }
				else if (isupper(sgfString[i])) { sBuffer += sgfString[i]; }
			}
		}

		m_vNode.push_back(node);
		return true;
	}

	void clear() {
		m_vNode.clear();
		m_sFileName = "";
		m_sSgfString = "";
		m_tag.clear();
	}
};
