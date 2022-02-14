#pragma once

#include <boost/random.hpp>
#include <boost/thread/tss.hpp>

class Random
{
//private:
public:
    typedef boost::mt11213b IntegerGenerator ;
    typedef boost::uniform_01<double> RealGenerator ;
    static boost::thread_specific_ptr<IntegerGenerator> integer_generator ;
    static boost::thread_specific_ptr<RealGenerator> real_generator ;

public:
    typedef IntegerGenerator::result_type integer_type ;
    typedef RealGenerator::result_type real_type ;

    static void reset(unsigned int s) ;

    /************************************************************************/
    /* random integer generator                                             */
    /************************************************************************/
    
    /*!
        @brief  generate a random integer
        @author T.F. Liao
        @return random integer generated
    */
    inline static integer_type nextInt() {
        return (*integer_generator)() ;
    }

    /*!
        @brief  generate a random integer in [0, range)
        @author T.F. Liao
        @return random integer generated
    */
    inline static integer_type nextInt(int range) {
        return (*integer_generator)()%range ;
    }

    /*!
        @brief  generate a random integer in [_min, _max)
        @author T.F. Liao
        @return random integer generated
    */
    inline static integer_type nextInt(int _min, int _max) {
        return (*integer_generator)()%(_max-_min) + _min ;
    }

    /************************************************************************/
    /* random read generator                                                */
    /************************************************************************/
    
    /*!
        @brief  generate a random real number in [0, 1)
        @author T.F. Liao
        @return random real number generated
    */
    inline static real_type nextReal() {
        return (*real_generator)(*integer_generator) ;
    }

     /*!
        @brief  generate a random real number in [0, range)
        @author T.F. Liao
        @return random real number generated
    */
    inline static real_type nextReal(real_type range) {
        return (*real_generator)(*integer_generator) * range ;
    }
};
