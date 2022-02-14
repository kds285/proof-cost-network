#include <iostream>
#include <fstream>
#include "GTPEngine.h"
#include "MCTSSolver.h"
#include "DFPNSolver.h"
#include "ZeroServer.h"
#include "ZeroSelfPlay.h"
#include "GameConfigure.h"
#include "ConfigureLoader.h"

using namespace std;

void gtp() {
	GTPEngine engine;
	engine.run();
}

void server() {
	ZeroServer server;
	server.run();
}

void selfPlay() {
	ZeroSelfPlayMaster spMaster;
	spMaster.run();
}

void mctsSolver() {
	MCTSSolver solver;
	solver.runSolver();
}

void dfpnSolver() {
	DFPNSolver solver;
	solver.runSolver();
}

void genConfiguration(ConfigureLoader& cl, string sConfFile) {
	// check configure file is exist
	ifstream f(sConfFile);
	if (f.good()) {
		char ans = '0';
		while (ans != 'y' && ans != 'n') {
			cerr << sConfFile << " already exist, do you want to overwrite it? [y/n]" << endl;
			cin >> ans;
		}
		if (ans == 'y') { cerr << "overwrite " << sConfFile << endl; }
		if (ans == 'n') { cerr << "didn't overwrite " << sConfFile << endl; f.close(); return; }
	}
	f.close();

	ofstream fout(sConfFile);
	fout << cl.toString();
	fout.close();
}

bool readConfiguration(ConfigureLoader& cl, string sConfFile, string sConfString) {
	if (!cl.loadFromFile(sConfFile)) { cerr << "Failed to read configuration file." << endl; return false; }
	if (!cl.loadFromString(sConfString)) { cerr << "Failed to read configuration string." << endl; return false; }

	cerr << cl.toString();
	return true;
}

int main(int argc, char* argv[]) {
	if (argc % 2 != 1) { cerr << "error command" << endl; return -1; }

	string sMode = "gtp";
	string sConfFile = "";
	string sConfString = "";
	ConfigureLoader cl;
	GameConfigure::setConfiguration(cl);
	Configure::setConfiguration(cl);

	for (int i = 1; i < argc; i += 2) {
		string sCommand = string(argv[i]);
		if (sCommand == "-mode") { sMode = argv[i + 1]; }
		else if (sCommand == "-conf_file") { sConfFile = argv[i + 1]; }
		else if (sCommand == "-conf_str") { sConfString = argv[i + 1]; }
		else if (sCommand == "-gen") { genConfiguration(cl, argv[i + 1]); return 0; }
		else { cerr << "error command" << endl; return -1; }
	}

	if (!readConfiguration(cl, sConfFile, sConfString)) { return -1; }
	Random::reset(Configure::USE_TIME_SEED ? static_cast<int>(time(NULL)) : Configure::SEED);
	
	if (sMode == "gtp") { gtp(); }
	else if (sMode == "server") { server(); }
	else if (sMode == "sp") { selfPlay(); }
	else if (sMode == "mcts_solver") { mctsSolver(); }
	else if (sMode == "dfpn_solver") { dfpnSolver(); }
	else { cerr << "error mode with " << sMode << endl; }

	return 0;
}
