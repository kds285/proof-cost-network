#pragma once

#include "boost/atomic.hpp"

/*!
	@brief  SpinLock implementation
	@author kwchen
	@reference  http://stackoverflow.com/questions/10966528/how-to-use-boost-atomic-to-remove-race-condition
*/
class SpinLock
{
private:
	boost::atomic<bool> m_bIsLock;

public:
	SpinLock ()
		: m_bIsLock(false)
	{
	}

	SpinLock ( const SpinLock& lock )
		: m_bIsLock(false)
	{
	}

	SpinLock operator= ( const SpinLock& lock )
	{
		m_bIsLock = false;
		return *this;
	}

	/*!
		@brief  acquire spin-lock
		@author kwchen
	*/
	void lock () 
	{
		while ( m_bIsLock.exchange(true, boost::memory_order_acquire) == true ) {
			// busy waiting
		}
	}
	/*!
		@brief  release spin-lock
		@author kwchen
	*/
	void unlock () 
	{
		m_bIsLock.store(false, boost::memory_order_release);
	}

};
