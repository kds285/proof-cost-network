#pragma once

#include <cassert>
#include <sstream>
#include "SpinLock.h"

/*!
	@brief  extract operations from statistic data
	@author T.F. Liao
*/
class StatisticData
{
public:
	typedef double data_type ;
public:
	/*!
		@brief  constructor with initial mean & count = 0
		@author T.F. Liao
	*/
	inline StatisticData () ;
	/*!
		@brief  constructor with initial mean & count
		@author T.F. Liao
		@param  mean [in] initial mean
		@param  count [in] initial count
	*/
	inline StatisticData ( data_type mean, data_type count ) ;

	/*!
		@brief  add a data value with weight 
		@author T.F. Liao
		@param  val [in] data value
		@param  weight [in] weight of this data value
	*/
	inline void add ( data_type val, data_type weight = (data_type)1.0 ) ;

	/*!
		@brief  add a data value with another statistic data
		@author kwchen
		@param  data [in] the statistic data
	*/
	inline void add ( const StatisticData& data ) ;

	/*!
		@brief  remove a data value with weight 
		@author T.F. Liao
		@param  val [in] data value
		@param  weight [in] weight of this data value
	*/
	inline void remove ( data_type val, data_type weight = (data_type)1.0 ) ;
	/*!
		@brief  remove a data value with another statistic data
		@author kwchen
		@param  data [in] the statistic data
	*/
	inline void remove ( const StatisticData& data ) ;
	/*!
		@brief  reset by given value
		@author T.F. Liao
		@param  mean [in] initial mean
		@param  count [in] initial count
	*/
	inline void reset ( data_type mean = (data_type)0, data_type count = (data_type)0 ) ;

	/*!
		@brief  get mean of statistic data
		@author T.F. Liao
		@return mean of statistic data as float
	*/
	inline data_type getMean () const ;
	/*!
		@brief  get count of statistic data
		@author T.F. Liao
		@return count of statistic data as float
	*/
	inline data_type getCount () const ;

	std::string toString(bool displayInPercentage = false) const {
		std::ostringstream oss ;
		if (displayInPercentage) { oss << m_mean * 100.0f << "%/" << m_count; }
		else { oss << m_mean << "/" << m_count; }
		return oss.str();
	}

private:
	volatile data_type m_mean ;
	volatile data_type m_count ;
	SpinLock m_lock;
};

inline StatisticData::StatisticData () 
{
	reset();
}
inline StatisticData::StatisticData ( data_type mean, data_type count ) 
	: m_mean ( mean ), m_count ( count )
{
}

inline void StatisticData::add ( data_type val, data_type weight ) 
{
	if( weight+m_count <= 0 ) reset();
	else {
		m_lock.lock();
		m_count += weight ;
		val -= m_mean;
		m_mean +=  weight * val / m_count ;
		m_lock.unlock();
	}
}

inline void StatisticData::add ( const StatisticData& data ) 
{
	add( data.getMean(), data.getCount() );
}

inline void StatisticData::remove ( data_type val, data_type weight ) 
{
	if ( m_count - weight <= 0 ) reset() ;
	else {
		m_lock.lock();
		m_count -= weight ;
		m_mean += weight * (m_mean - val) / m_count;
		m_lock.unlock();
	}
}

inline void StatisticData::remove ( const StatisticData& data ) 
{
	remove( data.getMean(), data.getCount() );
}

inline void StatisticData::reset ( data_type mean, data_type count ) 
{
	m_lock.lock();
	m_mean = mean ;
	m_count = count ;
	m_lock.unlock();
}

inline StatisticData::data_type StatisticData::getMean () const 
{
	return m_mean;
}

inline StatisticData::data_type StatisticData::getCount () const 
{
	return m_count;
}
