#include "ConfigureLoader.h"
#include <fstream>
#include <algorithm>

template<>
bool setParameter<bool>(bool& ref, const string& sValue)
{
	string sTmp = sValue;
	transform(sTmp.begin(), sTmp.end(), sTmp.begin(), ::toupper);
	ref = (sTmp == "TRUE" || sTmp == "1");
	return true;
}

template<>
bool setParameter<Color>(Color& ref, const string& sValue)
{
	if (sValue.length() != 1) { return false; }
	ref = charToColor(sValue[0]);
	return true;
}

template<>
bool setParameter<string>(string& ref, const string& sValue)
{
	ref = sValue;
	return true;
}

template<>
string getParameter<bool>(bool& ref)
{
	ostringstream oss;
	oss << (ref == true ? "true" : "false");
	return oss.str();
}

template<>
string getParameter<Color>(Color& ref)
{
	ostringstream oss;
	oss << string{colorToChar(ref)};
	return oss.str();
}

string ConfigureLoader::toString()
{
	ostringstream oss;
	for (auto groupName : m_vGroupOrder) {
		oss << "# " << groupName << endl;
		for (auto id : m_groups[groupName]) { oss << m_vParameters[id]->toString(); }
		oss << endl;
	}
	return oss.str();
}

bool ConfigureLoader::loadFromFile(string sConfFile)
{
	if (sConfFile.empty()) { return true; }

	string sLine;
	ifstream file(sConfFile);
	if (file.fail()) { return false; }
	while (getline(file, sLine)) {
		if (!setValue(sLine)) { return false; }
	}

	return true;
}

bool ConfigureLoader::loadFromString(string sConfString)
{
	if (sConfString.empty()) { return true; }

	string sLine;
	istringstream iss(sConfString);
	while (getline(iss, sLine, ':')) {
		if (!setValue(sLine)) { return false; }
	}

	return true;
}

void ConfigureLoader::trim(string& s)
{
	if (s.empty()) { return; }
	s.erase(0, s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
}

bool ConfigureLoader::setValue(string sLine)
{
	if (sLine.empty() || sLine[0] == '#') { return true; }

	string sKey = sLine.substr(0, sLine.find("="));
	string sValue = sLine.substr(sLine.find("=") + 1);
	if (sValue.find("#") != string::npos) { sValue = sValue.substr(0, sValue.find("#")); }

	trim(sKey);
	trim(sValue);

	if (m_parameterMapId.count(sKey) == 0) {
		cerr << "Invalid key \"" + sKey + "\" and value \"" << sValue << "\"" << endl;
		return false;
	}

	if (!(*m_vParameters[m_parameterMapId[sKey]])(sValue)) {
		cerr << "Unsatisfiable value \"" + sValue + "\" for option \"" + sKey + "\"" << endl;
		return false;
	}

	return true;
}
