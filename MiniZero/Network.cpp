#include "Network.h"
#include "Random.h"
#include <fstream>
#include <numeric>

void Network::initialize()
{
	// set GPU device & inputs
	assert(("Invalid GPU device number", m_gpuId >= 0));
	m_batchSize = (Configure::NET_ROTATION == 0) ? Configure::NET_BATCH_SIZE : Configure::NET_BATCH_SIZE * Configure::NET_ROTATION;
	m_inputs = torch::zeros({m_batchSize, Game::getNumChannels(), Game::getBoardSize(), Game::getBoardSize()}, torch::Device(torch::kCUDA, m_gpuId));

	if (!loadModel(m_sModelName)) {
		cerr << "Error when loading the model \"" << m_sModelName << "\"" << endl;
		return;
	}	

	m_vColor.resize(m_batchSize);
	m_vBadMove.resize(m_batchSize * Game::getMaxNumLegalAction());
	m_vLegalMove.resize(m_batchSize * Game::getMaxNumLegalAction());
	m_vSymmetry.resize(m_batchSize);
}

bool Network::loadModel(string sModelName)
{
	m_sModelName = sModelName;

	try {
		m_module = torch::jit::load(m_sModelName, torch::Device(torch::kCUDA, m_gpuId));
		m_module.eval();
	}
	catch (const c10::Error& e) {
		return false;
	}

	return true;
}

void Network::forward()
{
	auto res = m_module.forward(vector<torch::jit::IValue>{m_inputs});
	auto res_tuple = res.toTuple();

	// policy
	auto probability = torch::softmax(res_tuple->elements()[0].toTensor(), 1).to(at::kCPU);
	m_vProbability.resize(probability.numel());
	copy(probability.data_ptr<float>(), probability.data_ptr<float>() + probability.numel(), m_vProbability.begin());

	if (Configure::NET_VALUE_WINLOSS) {
		// value
		auto value = res_tuple->elements()[1].toTensor().to(at::kCPU);
		m_vValue.resize(value.numel());
		copy(value.data_ptr<float>(), value.data_ptr<float>() + value.numel(), m_vValue.begin());
	} else if (Configure::NET_VALUE_SPACE_COMPLEXITY) {
		// black value
		auto blackValue = torch::softmax(res_tuple->elements()[1].toTensor(), 1).to(at::kCPU);
		m_vBlackValue.resize(blackValue.numel());
		copy(blackValue.data_ptr<float>(), blackValue.data_ptr<float>() + blackValue.numel(), m_vBlackValue.begin());

		// white value
		auto whiteValue = torch::softmax(res_tuple->elements()[2].toTensor(), 1).to(at::kCPU);
		m_vWhiteValue.resize(whiteValue.numel());
		copy(whiteValue.data_ptr<float>(), whiteValue.data_ptr<float>() + whiteValue.numel(), m_vWhiteValue.begin());
	} else { assert(("Error configuration for training value target!", false)); }
}

void Network::set_data(int batchID, const Game& game, SymmetryType type/* = SYM_NORMAL*/)
{
	assert(("Batch ID overflow", batchID < m_batchSize));

	if (Configure::NET_ROTATION == 0) {
		set_data_(batchID, game, type);
	} else {
		vector<int> vSymmetry(8);
		iota(vSymmetry.begin(), vSymmetry.end(), 0);
		for (int i = 0; i < Configure::NET_ROTATION; ++i) {
			int index = Random::nextInt(SYMMETRY_SIZE - i);
			set_data_(batchID * Configure::NET_ROTATION + i, game, static_cast<SymmetryType>(vSymmetry[index]));
			swap(vSymmetry[index], vSymmetry[SYMMETRY_SIZE - 1 - i]);
		}
	}
}

vector<pair<Move, float>> Network::getProbability(int batchID)
{
	assert(("Network probability size mismatch!", m_vProbability.size() % m_batchSize == 0));
	assert(("Network probability size mismatch!", m_vProbability.size() / m_batchSize == Game::getMaxNumLegalAction()));

	vector<double> vProbability(Game::getMaxNumLegalAction(), 0.0f);
	int rotation = (Configure::NET_ROTATION == 0) ? 1 : Configure::NET_ROTATION;
	int startBatchID = batchID * rotation;
	for (int i = 0; i < rotation; ++i) {
		int shift = (startBatchID + i) * Game::getMaxNumLegalAction();
		for (int pos = 0; pos < Game::getMaxNumLegalAction(); ++pos) {
			if (!m_vLegalMove[shift + pos]) { vProbability[pos] = -1; continue; }
			int rotatePos = getRotatePosition(pos, Game::getBoardSize(), m_vSymmetry[startBatchID + i]);
			vProbability[pos] += m_vProbability[shift + rotatePos] / rotation;
		}
	}

	vector<pair<Move, float>> vProb;
	for (int pos = 0; pos < Game::getMaxNumLegalAction(); ++pos) {
		if (vProbability[pos] < 0) { continue; }
		vProb.push_back({Move(m_vColor[batchID], pos), vProbability[pos]});
	}
	sort(vProb.begin(), vProb.end(), [](const pair<Move, float>& a, const pair<Move, float>& b) {
		return a.second > b.second;
	});
	return vProb;
}

float Network::getValue(int batchID)
{
	assert(("Network value size mismatch!", m_vValue.size() % m_batchSize == 0));
	assert(("Network value size mismatch!", m_vValue.size() / m_batchSize == Configure::NET_NUM_OUTPUT_V));

	double dValue = 0.0f;
	int rotation = (Configure::NET_ROTATION == 0) ? 1 : Configure::NET_ROTATION;
	int startBatchID = batchID * rotation;
	for (int i = 0; i < rotation; ++i) {
		dValue += m_vValue[(startBatchID + i) * Configure::NET_NUM_OUTPUT_V] / rotation;
	}
	return static_cast<float>(dValue);
}

float Network::getValue(int batchID, Color c)
{
	if (c == COLOR_BLACK) { return getValue(batchID, m_vBlackValue); }
	else if (c == COLOR_WHITE) { return getValue(batchID, m_vWhiteValue); }
	else { assert(("Unknown Color for getValue in Network", false)); }

	return -1;
}

string Network::getRotationString(int batchID)
{
	ostringstream oss;
	int rotation = (Configure::NET_ROTATION == 0) ? 1 : Configure::NET_ROTATION;
	int startBatchID = batchID * rotation;
	
	oss << getSymmetryTypeString(m_vSymmetry[startBatchID]);
	for (int i = 1; i < rotation; ++i) { oss << " " << getSymmetryTypeString(m_vSymmetry[startBatchID + i]); }
	return oss.str();
}

void Network::set_data_(int batchID, const Game& game, SymmetryType type)
{
	Color turnColor = game.getTurnColor();
	m_vColor[batchID] = turnColor;
	m_vSymmetry[batchID] = type;
	m_inputs[batchID] = torch::from_blob(game.getFeatures(type).data(), {Game::getNumChannels(), Game::getBoardSize(), Game::getBoardSize()});
	int shift = batchID * Game::getMaxNumLegalAction();
	for (int pos = 0; pos < Game::getMaxNumLegalAction(); ++pos) {
		const Move m(turnColor, pos);
		m_vBadMove[shift + pos] = game.isBadMove(m);
		m_vLegalMove[shift + pos] = game.isLegalMove(m);
	}
}

float Network::getValue(int batchID, const vector<float>& value)
{
	assert(("Network value size mismatch!", value.size() % m_batchSize == 0));
	assert(("Network value size mismatch!", value.size() / m_batchSize == Configure::NET_NUM_OUTPUT_V));

	double dValue = 0.0f;
	int rotation = (Configure::NET_ROTATION == 0) ? 1 : Configure::NET_ROTATION;
	int startBatchID = batchID * rotation;
	for (int i = 0; i < rotation; ++i) {
		float fArea = 0.0f;
		int shift = (startBatchID + i) * Configure::NET_NUM_OUTPUT_V;
		for (int j = 0; j < Configure::NET_NUM_OUTPUT_V; ++j) { fArea += value[shift + j] * j; }
		dValue += fArea / rotation;
	}

	return dValue;
}