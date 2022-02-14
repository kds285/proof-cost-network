#include "Random.h"

boost::thread_specific_ptr<Random::IntegerGenerator> Random::integer_generator ;

boost::thread_specific_ptr<Random::RealGenerator> Random::real_generator ;

void Random::reset( unsigned int s )
{
    if ( integer_generator.get() == NULL ) {
        integer_generator.reset(new IntegerGenerator());
    }
    integer_generator->seed(s) ;
}
