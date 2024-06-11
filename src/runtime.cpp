#include <mastermyr/runtime.hpp>

namespace myr {

void runtime::start()
{
	m_running = true;
	loop();
}

void runtime::stop()
{
	m_running = false;
}

bool runtime::work_to_do() const
{
	return !(m_callbacks.empty());
}

void runtime::loop()
{
	while (m_running)
	{
		if (!work_to_do())
			break;

		while (!m_callbacks.empty())
		{
			auto&& callback = m_callbacks.front();
			callback();
			m_callbacks.pop_front();
		}
	}

	m_running = false;
}

void runtime::register_callback(callback cb)
{
	m_callbacks.push_back(cb);
}

}
