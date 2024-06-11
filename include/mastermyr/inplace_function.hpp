#ifndef __mastermyr_inplace_function_hpp
#define __mastermyr_inplace_function_hpp

#include <cstddef>

namespace myr {

template<typename Signature, size_t FunctorSize>
class inplace_function;

template<typename ReturnType, typename ...Args, size_t FunctorSize>
class inplace_function<ReturnType(Args...), FunctorSize>
{

};

}

#endif
