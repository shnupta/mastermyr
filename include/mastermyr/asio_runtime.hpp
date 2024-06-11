#ifndef __mastermyr_asio_runtime_hpp
#define __mastermyr_asio_runtime_hpp

#include "runtime.hpp"

#include <boost/asio.hpp>

namespace myr {

class asio_runtime : public myr::runtime_base
{
public:
	void start() final override;
	void stop() final override;

	void register_callback(callback) final override;

	boost::asio::io_context& get_io_context() { return m_context; }

private:
	boost::asio::io_context m_context;
};

inline void asio_runtime::start()
{
	m_context.run();
}

inline void asio_runtime::stop()
{
	m_context.stop();
}

inline void asio_runtime::register_callback(callback cb)
{
	boost::asio::post(m_context, cb);
}

}

#endif
