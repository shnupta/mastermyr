#ifndef __mastermyr_application_hpp
#define __mastermyr_application_hpp

#include "runtime.hpp"

#include <type_traits>

namespace myr {

enum class run_result : int
{
	success = 0,
	failure = 1
};


template<typename Runtime = runtime>
class application
{
	static_assert(std::is_base_of_v<runtime_base, Runtime>, "Runtime type must derive from runtime_base");

public:
	using runtime_type = Runtime;


	int run(int argc, char** argv);

	runtime_base* get_runtime();
	runtime_type* get_runtime_impl();

	// called just before the runtime starts to loop
	std::function<void()> start_function;

private:
	bool parse_arguments(int argc, char** argv);

	runtime_type m_runtime;
	run_result m_run_result = run_result::success;
};

template<typename Runtime>
int application<Runtime>::run(int argc, char** argv)
{
	if (!parse_arguments(argc, argv))
		return static_cast<int>(m_run_result);

	if (start_function)
		start_function();


	m_runtime.start();

	return static_cast<int>(m_run_result);
}

template<typename Runtime>
bool application<Runtime>::parse_arguments(int argc, char** argv)
{
	return true;
}

template<typename Runtime>
runtime_base* application<Runtime>::get_runtime()
{
	return &m_runtime;
}

template<typename Runtime>
auto application<Runtime>::get_runtime_impl() -> runtime_type*
{
	return &m_runtime;
}

}

#endif
