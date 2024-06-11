#ifndef __mastermyr_runtime_hpp
#define __mastermyr_runtime_hpp

#include <deque>
#include <functional>

namespace myr {

class runtime_base
{
public:
	virtual ~runtime_base() {}

	using callback = std::function<void()>;

	virtual void start() = 0;
	virtual void stop() = 0;

	virtual void register_callback(callback) = 0;
};

class runtime final : public runtime_base
{
public:
	void start() final override;
	void stop() final override;

	void register_callback(callback) final override;

private:
	bool work_to_do() const;
	void loop();

	bool m_running = false;

	std::deque<callback> m_callbacks;
};

}

#endif
