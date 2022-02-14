#ifndef BUFFER_H
#define BUFFER_H

#include "list.h"

template<typename _Feature, int max_size>
class FeatureList
{
public:
    FeatureList() 
        : m_capacity ( 0 ), m_size ( 0 )
    {
        m_freeArr.init();
    }

    FeatureList ( const FeatureList& rhs ) 
        : m_capacity ( rhs.m_capacity ), m_size ( rhs.m_size )
    {
        for ( uint i=0;i<m_capacity;++i ) {
            m_vElements[i] = rhs.m_vElements[i] ;
        }
        /// replace it if List implement copy constructor
        this->m_freeArr.clone(rhs.m_freeArr) ; 
    }

    FeatureList& operator= ( const FeatureList& rhs ) 
    {
        m_capacity = rhs.m_capacity ;
        m_size = rhs.m_size;
        for ( uint i=0;i<m_capacity;++i ) {
            m_vElements[i] = rhs.m_vElements[i] ;
        }
        /// replace it if List implement copy assignment
        this->m_freeArr.clone(rhs.m_freeArr) ; 
        return *this;
    }

    inline _Feature* NewOne () 
    {
        assert(m_freeArr.getSize()>=0);
        _Feature* ptr ;
        int iCurIdx ;
        if ( m_freeArr.getSize() != 0 ) {
            iCurIdx = m_freeArr.removeLast() ;
        } else {
            iCurIdx = m_capacity ++ ;
        }
        ptr = m_vElements + iCurIdx;
        ptr->SetID ( iCurIdx ) ;
        m_size ++;
        assert(m_freeArr.getSize()>=0);
        return ptr ;
    }
    inline void FreeOne ( _Feature* ptr ) 
    {
        m_size -- ;
        m_freeArr.insert(ptr->GetID());
        ptr->Clear();
    }

    inline _Feature* getArray() 
    {
        return m_vElements ;
    }
    inline _Feature* getAt ( uint idx ) 
    {
        assert(isValidIdx(idx));
        return m_vElements+idx ;
    }

    inline const _Feature* getAt ( uint idx ) const
    {
        assert(isValidIdx(idx));
        return m_vElements+idx ;
    }
    
    //get the largest available ID
    inline uint getCapacity () const 
    {
        return m_capacity ;
    }

    //get the real number of features in use
    inline uint getSize () const
    {
        return m_size ;
    }

    inline void reset ()
    {
        m_capacity = 0 ;
        m_size = 0 ;
        m_freeArr.init() ;
    }
    
    bool isValidIdx( uint idx ) const {
        assert(idx < m_capacity);
        for( int i=0; i<m_freeArr.getSize(); i++)
            assert( (m_vElements+idx)->GetID() == static_cast<uint>(-1)
                   ||   m_freeArr.elementAt(i) != idx );
        return (m_vElements+idx)->GetID() != static_cast<uint>(-1);
    }

private:
    uint m_capacity ;
    uint m_size ;
    _Feature m_vElements[max_size] ;
    List<uint, max_size> m_freeArr ;
};

#endif 
