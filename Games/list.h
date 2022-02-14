#ifndef LIST_H
#define LIST_H

template <class T, int max_size> class List 
{
protected:
    int size; 
    T elems[max_size];

#ifdef DEBUG_LIST
    // Following are used for statistics. 
    int accessCount; 
    int errorCount;
    int maxEverSize; 
#endif

public:
    List() { init(); }
    //vector<Segment> elem;
    void init(){
        size = 0;

#ifdef DEBUG_LIST
        // Following are used for statistics. 
        accessCount = 0; 
        errorCount=0;
        maxEverSize=0;
#endif
    }

#ifdef DEBUG_LIST
    void errorProcessing() {
        errorCount++; 
    }
#endif

    // return false, if the insertion fails. 
    T& allocateOne() {
#ifdef DEBUG_LIST
        accessCount++; 
        if (maxEverSize < size+1) maxEverSize = size+1; 

        if (size >= max_size) {
            //char str_buf[256];
            // error.
            //sprintf(str_buf,"*** Serious Warning: the size of SegmentList %s exceeds the limit\n", name);
            //PRINT_ERROR_MSG(str_buf);
            errorProcessing();
            return elems[size-1]; 
        }
#endif
        return elems[size++];
    }

    // return false, if the insertion fails. 
    T& insert(T elem) {
        T& new_elem = allocateOne();
        new_elem = elem;
        return new_elem;
    }

    T& elementAt(int i){
#ifdef DEBUG_LIST
        accessCount++; 
        if (i >= size || i<0) {
            errorProcessing();
        }
#endif
        return elems[i];
    }

  const T& elementAt(int i) const {
#ifdef DEBUG_LIST
      accessCount++; 
      if (i >= size || i<0) {
          errorProcessing();
      }
#endif
      return elems[i];
  }

    T& operator[](int i){
        return elementAt(i);
    }

    T& removeLast() {
#ifdef DEBUG_LIST
        if (size == 0) {
            errorProcessing();
            // error 
            return elems[0];
        }
#endif
        return elems[--size];
    }

    T& remove(int i) {
        T& elem = elementAt(i);
        elems[i] = elems[--size];
        return elem;
    }

    int getSize() const {
        return size; 
    }

    void setSize(int iSize) {
        size = iSize;
    }

    void swap(uint i, uint j) {
        T temp;
        temp = elementAt(i);
        elementAt(i) = elementAt(j);
        elementAt(j) = temp;
    }

    T& getLast() {
        return elems[size-1];
    }

    T& getLast2() {
        return elems[size-2];
    }
    T& getLast3() {
        return elems[size-3];
    }

#define ELEM_NUM_IN_WORD (sizeof(int)/sizeof(T))

    inline void clone(const List<T,max_size>& src_list) {
        size = src_list.size;
#ifdef DEBUG_LIST
        accessCount = src_list.accessCount;
        errorCount = src_list.errorCount;
        maxEverSize = src_list.maxEverSize;
#endif
        int *src_start = (int*)&src_list.elementAt(0);
        int *dest_start = (int*)&elementAt(0);
        int count = (size+ELEM_NUM_IN_WORD-1)/ELEM_NUM_IN_WORD;
        for (int i=0; i<count; i++) {
            dest_start[i] = src_start[i];
        }
    }
};

#endif