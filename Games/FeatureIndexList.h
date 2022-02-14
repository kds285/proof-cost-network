#ifndef FEATUREINDEXLIST_H
#define FEATUREINDEXLIST_H


//m_vFeaturePtr�s��feature IDs�A�]��_FeatureIndex��unsigned char, short �� int �A�ݬ�_max_id���j�p(�W�L256�N�����unsigned char)
template<class _FeatureIndex, int _max_size, int _max_id=_max_size>
class FeatureIndexList
{
private:
    uint m_nFeature ;
    _FeatureIndex m_vFeatureIndex [_max_size] ;
    short m_vIndex [_max_id+1] ;
public:
    FeatureIndexList() 
    {
        clear();
    }
    FeatureIndexList & operator= ( const FeatureIndexList& rhs ){
        assert(sizeof(m_vIndex)==sizeof(rhs.m_vIndex));
        m_nFeature = rhs.m_nFeature;
        //memcpy(m_vFeatureIndex, rhs.m_vFeatureIndex, rhs.m_nFeature*sizeof(_FeatureIndex) );
        //memcpy(m_vIndex, rhs.m_vIndex, sizeof(m_vIndex));
        for(uint i=0 ; i<m_nFeature ; i++ ){
            m_vFeatureIndex[i] = rhs.m_vFeatureIndex[i];
        }
        for(uint i=0 ; i<_max_id+1 ; i++ ){
            m_vIndex[i] = rhs.m_vIndex[i];
        }
        return *this;
    }
    inline void addFeature ( uint id ) 
    {
        assert(invariance());
        if ( m_vIndex[id] != -1 ) return ;
        m_vFeatureIndex[m_nFeature] = id ;
        m_vIndex[id] = m_nFeature ++;
        assert(invariance());
    }

    inline void removeFeature ( uint featureID ) 
    {
        assert ( invariance() ) ;
        if ( m_vIndex[featureID] == -1 ) assert(false) ;
        int index = m_vIndex[featureID] ;
        _FeatureIndex last = m_vFeatureIndex[index] = m_vFeatureIndex[--m_nFeature];
        m_vIndex[last] = index ;
        m_vIndex[featureID] = -1;
        assert ( invariance() ) ;
    }

    const _FeatureIndex& operator[] ( size_t index ) const 
    {
        assert ( index < m_nFeature ) ;
        return m_vFeatureIndex[index] ;
    }
    
    _FeatureIndex& operator[] ( size_t index ) 
    {
        assert ( index < m_nFeature ) ;
        return m_vFeatureIndex[index] ;
    }

    bool contains ( _FeatureIndex featureID ) const 
    {
        assert ( featureID <= _max_id ) ;
        return ( m_vIndex[featureID] != -1 ) ;
    }

    inline uint size() const { return m_nFeature; }
    inline uint getNumFeature () const { return m_nFeature; }
    inline const _FeatureIndex* getFeatureIDs () const { return m_vFeatureIndex ; }

    inline void clear() { m_nFeature = 0; memset ( m_vIndex, -1, sizeof m_vIndex ); }
private:
    bool invariance () 
    {
        uint count = 0;
        for ( uint i=0;i<=_max_id;++i ) {
            if ( m_vIndex[i] != -1 ) {
                int idx = m_vIndex[i] ;
                ++ count ;
                assert ( static_cast<uint>(idx) < m_nFeature ) ;
                assert ( m_vFeatureIndex[idx] == i ) ;
            }
        }
        assert ( count == m_nFeature ) ;
        return true; 
    }
};


#endif
