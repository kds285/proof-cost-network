#pragma once

#include <memory>
#include <torch/script.h>
#include "Configure.h"

class Network {
private:
	int m_gpuId;
	int m_batchSize;
	string m_sModelName;
	torch::Tensor m_inputs;
	torch::jit::script::Module m_module;

	vector<float> m_vValue;
	vector<float> m_vBlackValue;
	vector<float> m_vWhiteValue;
	vector<float> m_vProbability;

	vector<Color> m_vColor;
	vector<int> m_vBadMove;
	vector<int> m_vLegalMove;
	vector<SymmetryType> m_vSymmetry;

public:
	Network(int gpu_id, string sModelName)
		: m_gpuId(gpu_id)
		, m_sModelName(sModelName)
	{
	}
	~Network() {}

	void initialize();
	bool loadModel(string sModelName);
	void forward();
	void set_data(int batchID, const Game& game, SymmetryType type = SYM_NORMAL);
	
	vector<pair<Move, float>> getProbability(int batchID);
	float getValue(int batchID);
	float getValue(int batchID, Color c);
	string getRotationString(int batchID);

	inline int getGPUID() const { return m_gpuId; }
	inline string getModelName() const { return m_sModelName; }

private:
	void set_data_(int batchID, const Game& game, SymmetryType type);
	float getValue(int batchID, const vector<float>& value);
};