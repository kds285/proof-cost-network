#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cmath>
#include <random>
#include <fstream>
#include "SgfLoader.h"
#include "../MiniZero/Configure.h"

using namespace std;
namespace py = pybind11;

class DataLoader {
public:
	Game m_game;
	int m_dataSize;
	string m_sTrainDir;
	vector<pair<SgfLoader, int>> m_vGameCollection;
	
	// random generator
	mt19937 m_gen;
	uniform_int_distribution<> m_dis;
	
	// feature & label
	vector<float> m_vFeatures;
	vector<float> m_vPolicy;
	float m_fValueWinLoss;
	vector<float> m_vBlackValue;
	vector<float> m_vWhiteValue;
	
	
public:
	DataLoader(string sTrainDir, int seed)
		: m_sTrainDir(sTrainDir)
	{
		m_gen = mt19937(seed);
		m_dis = uniform_int_distribution<>(1, INT_MAX);
	}
	
	void load(int start, int end) {
		string sgf;
		SgfLoader sgfLoader;
		
		m_dataSize = 0;
		for(int i=start; i<=end; ++i) {
			ifstream file(m_sTrainDir + "/sgf/" + to_string(i) + ".sgf", std::ifstream::in);
			while(getline(file, sgf)) {
				sgfLoader.parseFromString(sgf);
				m_dataSize += sgfLoader.getSgfNode().size() - 1;
				m_vGameCollection.push_back({sgfLoader, m_dataSize});
			}
		}
	}
	
	void calculateFeaturesAndLabels() {
		m_game.reset();
		pair<int, int> p = randomPickPosition();
		const SgfLoader& sgfLoader = m_vGameCollection[p.first].first;
		for (int i = 1; i < p.second + 1; ++i) { m_game.play(Move(sgfLoader.getSgfNode()[i].m_move, m_game.getBoardSize())); }

		SymmetryType type = static_cast<SymmetryType>(m_dis(m_gen) % SYMMETRY_SIZE);
		m_vFeatures = m_game.getFeatures(type);
		calculatePolicy(sgfLoader.getSgfNode()[p.second + 1], type);

		// value
		Color winner = charToColor(sgfLoader.getTag("RE")[0]);
		if (Configure::NET_VALUE_WINLOSS) {
			// value (win loss)
			Color nextColor = charToColor(sgfLoader.getSgfNode()[p.second + 1].m_move.first[0]);
			m_fValueWinLoss = (winner == COLOR_NONE ? 0.0f : (winner == nextColor ? 1.0f : -1.0f));
		}
		
		if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
			// value (space complexity)
			float fSpaceComplexity = 0.0f;
			for (int i = static_cast<int>(sgfLoader.getSgfNode().size()) - 2; i >= p.second + 1; i -= 2) {
				string sComment = sgfLoader.getSgfNode()[i].m_comment;
				fSpaceComplexity += log10(stoi(sComment.substr(sComment.find("%") + 1)));
			}
			fSpaceComplexity = fmax(0, fmin(fSpaceComplexity, Configure::NET_NUM_OUTPUT_V - 1));
			setSpaceComplexityValue(m_vBlackValue, (winner == COLOR_BLACK ? fSpaceComplexity : Configure::NET_NUM_OUTPUT_V - 1));
			setSpaceComplexityValue(m_vWhiteValue, (winner == COLOR_WHITE ? fSpaceComplexity : Configure::NET_NUM_OUTPUT_V - 1));
		}
	}
	
	inline py::list getFeatures() { return py::cast(m_vFeatures); }
	inline py::list getPolicy() { return py::cast(m_vPolicy); }
	inline float getValueWinLoss() { return m_fValueWinLoss; }
	inline py::list getBlackValue() { return py::cast(m_vBlackValue); }
	inline py::list getWhiteValue() { return py::cast(m_vWhiteValue); }
	
private:
	pair<int, int> randomPickPosition() {
		int l = 0;
		int r = m_vGameCollection.size();
		int randPos = m_dis(m_gen) % m_dataSize;

		while(l<r) {
			int m = l + (r-l)/2;
			if(randPos>=m_vGameCollection[m].second) { l = m + 1; }
			else { r = m; }
		}

		return {l, (l==0? randPos: randPos - m_vGameCollection[l-1].second)};
	}
	
	std::vector<string> splitToVector(const string& sParam, const char& cToken)
	{
		string s;
		std::vector<string> vArgument;
		std::istringstream iss(sParam);
		while( std::getline(iss,s,cToken) ) { vArgument.push_back(s); }

		return vArgument;
	}
	
	void calculatePolicy(const SgfNode& sgfNode, SymmetryType type) {
		string sComment = sgfNode.m_comment;
		m_vPolicy.clear();
		m_vPolicy.resize(Game::getMaxNumLegalAction(), 0);
		if(sComment.empty() || sComment[0]=='%') {
			int pos = Move(sgfNode.m_move, Game::getBoardSize()).getPosition();
			pos = getRotatePosition(pos, Game::getBoardSize(), type);
			m_vPolicy[pos] = 1.0f;
		}
		
		// calculate distribution
		sComment = sComment.substr(0, sComment.find("%"));
		vector<string> vMoveCount = splitToVector(sComment, ',');

		float total_count = 0;
		for (int i = 0; i < vMoveCount.size(); ++i) {
			if (vMoveCount[i].find(":") == string::npos) { continue; }

			int split_index = static_cast<int>(vMoveCount[i].find(":"));
			int pos = atoi(vMoveCount[i].substr(0, split_index).c_str());
			pos = getRotatePosition(pos, Game::getBoardSize(), type);
			float count = atof(vMoveCount[i].substr(split_index + 1).c_str());
			m_vPolicy[pos] = count;
			total_count += count;
		}

		// normalization
		for (int i = 0; i < m_vPolicy.size(); ++i) { m_vPolicy[i] /= total_count; }
	}
	
	void setSpaceComplexityValue(vector<float>& vValue, float fValue) {
		vValue.clear();
		vValue.resize(Configure::NET_NUM_OUTPUT_V, 0);
		assert(("NET_NUM_OUTPUT_V should be larger than 1", Configure::NET_NUM_OUTPUT_V > 1));

		float fLowerBound = floor(fValue);
		float fUpperBound = ceil(fValue);

		if (fLowerBound == fUpperBound) {
			assert(("Space complexity value should range in [0," + to_string(Configure::NET_NUM_OUTPUT_V) + ")", fLowerBound >= 0 && fLowerBound < Configure::NET_NUM_OUTPUT_V));
			vValue[fLowerBound] = 1.0f;
		} else {
			vValue[fLowerBound] = fUpperBound - fValue;
			vValue[fUpperBound] = fValue - fLowerBound;
		}
	}
};

class Conf {
public:
	Conf(string sFile) {
		ConfigureLoader cl;
		Configure::setConfiguration(cl);
		GameConfigure::setConfiguration(cl);
		cl.loadFromFile(sFile);
	}
	inline int getNNInputChannel() { return Game::getNumChannels(); }
	inline int getNNFilterSize() { return Configure::NET_FILTER_SIZE; }
	inline int getNNNumOuputValue() { return Configure::NET_NUM_OUTPUT_V; }
	inline int getBoardSize() { return Game::getBoardSize(); }
	inline int getMaxNumLegalAction() { return Game::getMaxNumLegalAction(); }
	inline bool isTrainWinLoss() { return Configure::NET_VALUE_WINLOSS; }
};

namespace py = pybind11;
PYBIND11_MODULE(miniZeroPy, m) {
	py::class_<DataLoader>(m, "DataLoader")
		.def(py::init<string, int>())
		.def("load", &DataLoader::load)
		.def("calculateFeaturesAndLabels", &DataLoader::calculateFeaturesAndLabels)
		.def("getFeatures", &DataLoader::getFeatures)
		.def("getPolicy", &DataLoader::getPolicy)
		.def("getValueWinLoss", &DataLoader::getValueWinLoss)
		.def("getBlackValue", &DataLoader::getBlackValue)
		.def("getWhiteValue", &DataLoader::getWhiteValue);

	py::class_<Conf>(m, "Conf")
		.def(py::init<string>())
		.def("getNNInputChannel", &Conf::getNNInputChannel)
		.def("getNNFilterSize", &Conf::getNNFilterSize)
		.def("getNNNumOuputValue", &Conf::getNNNumOuputValue)
		.def("getBoardSize", &Conf::getBoardSize)
		.def("getMaxNumLegalAction", &Conf::getMaxNumLegalAction)
		.def("isTrainWinLoss", &Conf::isTrainWinLoss);
}
