#pragma once

#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "Color.h"

using namespace std;

// parameter setter
template<class T>
bool setParameter(T& ref, const string& sValue)
{
	istringstream iss(sValue);
	iss >> ref;
	return (iss && iss.rdbuf()->in_avail() == 0);
}
template<> bool setParameter<bool>(bool& ref, const string& sValue);
template<> bool setParameter<Color>(Color& ref, const string& sValue);
template<> bool setParameter<string>(string& ref, const string& sValue);

// parameter getter
template<class T>
string getParameter(T& ref)
{
	ostringstream oss;
	oss << ref;
	return oss.str();
}
template<> string getParameter<bool>(bool& ref);
template<> string getParameter<Color>(Color& ref);

// parameter container
class BaseParameter {
public:
	virtual bool operator()(const string& sValue) = 0;
	virtual string toString() = 0;
	virtual ~BaseParameter() {}
};

template<class T, class _Setter, class _Getter>
class Parameter : public BaseParameter {
private:
	string m_sKey;
	T& m_ref;
	_Setter m_setter;
	_Getter m_getter;
	string m_sDescription;
public:
	Parameter(const string sKey, T& ref, const string sDescription, _Setter setter, _Getter getter)
		: m_sKey(sKey)
		, m_ref(ref)
		, m_sDescription(sDescription)
		, m_setter(setter)
		, m_getter(getter)
	{}

	bool operator()(const string& sValue) { return m_setter(m_ref, sValue); }
	string toString() {
		ostringstream oss;
		oss << m_sKey << "=" << m_getter(m_ref);
		if (!m_sDescription.empty()) { oss << " # " << m_sDescription; }
		oss << endl;
		return oss.str();
	}
};

class ConfigureLoader {
private:
	map<string, int> m_parameterMapId;
	map<string, vector<int>> m_groups;
	vector<string> m_vGroupOrder;
	vector<BaseParameter*> m_vParameters;

public:
	ConfigureLoader() {}
	~ConfigureLoader() { for (int i = 0; i < m_vParameters.size(); ++i) { delete m_vParameters[i]; } }

	template<class T, class _Setter, class _Getter>
	inline void addParameter(const string& sKey, T& value, const string sDescription, const string& sGroup, _Setter setter, _Getter getter) {
		if (m_groups.count(sGroup) == 0) { m_vGroupOrder.push_back(sGroup); }
		m_groups[sGroup].push_back(m_vParameters.size());
		m_parameterMapId[sKey] = m_vParameters.size();
		m_vParameters.push_back(new Parameter<T, _Setter, _Getter>(sKey, value, sDescription, setter, getter));
	}

	template<class T>
	inline void addParameter(const string& sKey, T& value, const string sDescription, const string& sGroup) {
		addParameter(sKey, value, sDescription, sGroup, setParameter<T>, getParameter<T>);
	}

	string toString();
	bool loadFromFile(string sConfFile);
	bool loadFromString(string sConfString);

private:
	void trim(string& s);
	bool setValue(string sLine);
};
