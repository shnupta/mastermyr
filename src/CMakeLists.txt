file(GLOB headers *.hpp)
file(GLOB sources *.cpp)

add_library(libmastermyr ${sources} ${headers})

target_include_directories(libmastermyr
	PUBLIC
		$<BUILD_INTERFACE:${MASTERMYR_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:include>
		${Boost_INCLUDE_DIRS}
	PRIVATE
		${CMAKE_SOURCE_DIR}/src
)
