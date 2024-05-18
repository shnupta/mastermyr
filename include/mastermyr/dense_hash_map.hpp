#ifndef __mastermyr_dense_hash_map_hpp
#define __mastermyr_dense_hash_map_hpp

#include <cstddef>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>
#include <assert.h>

namespace myr {

namespace detail {

template<class Key, class T>
union punned_key_value_pair
{
	using pair_type = std::pair<Key, T>;
	using const_pair_type = std::pair<const Key, T>;

	static_assert(offsetof(pair_type, first) == offsetof(const_pair_type, second));
	static_assert(offsetof(pair_type, second) == offsetof(const_pair_type, second));

	// Explicitly makes pair_type (non-const) the active member of the union
	template<typename... Args>
	constexpr punned_key_value_pair(Args&&... args) : m_pair(std::forward<Args>(args)...) {}

	// Explicit constructors and assignment operators ensuring pair_type is the active member
	constexpr punned_key_value_pair(const punned_key_value_pair& other) : m_pair(other.m_pair) {}
	constexpr punned_key_value_pair(punned_key_value_pair&& other) : m_pair(std::move(other.m_pair)) {}

	constexpr punned_key_value_pair& operator=(const punned_key_value_pair& other)
	{
		m_pair = other.m_pair;
		return *this;
	}

	constexpr punned_key_value_pair& operator=(punned_key_value_pair&& other)
	{
		m_pair = std::move(other.m_pair);
		return *this;
	}

	~punned_key_value_pair() { m_pair.~pair_type(); }

	// Accessors for both pair_type and const_pair_type
	constexpr pair_type& pair() { return m_pair; }
	constexpr const pair_type& pair() const { return m_pair; }
	constexpr const_pair_type& const_pair() { return m_const_pair; }
	constexpr const const_pair_type& const_pair() const { return m_const_pair; }

private:
	pair_type m_pair;
	const_pair_type m_const_pair;
};

struct bucket_type {}; // TODO: stores distance from original bucket and fingerprint? if i do robin hood hashing

template<class T>
struct base_table_type_map
{
	using mapped_type = T;
};

struct base_table_type_set
{};

template<class Mapped>
constexpr bool is_map_v = !std::is_void_v<Mapped>;

template<
	class Key,
	class T,
	class Hash,
	class Pred,
	class Allocator,
	class Bucket,
	bool IsSegmented>
class dense_hash_table : public std::conditional_t<is_map_v<T>, base_table_type_map<T>, base_table_type_set>
{
private:
	using internal_value_type = typename std::conditional_t<is_map_v<T>, punned_key_value_pair<Key, T>, T>;
	using value_container_type = std::vector<internal_value_type, Allocator>; // TODO: once I've written a stable vector do conditional here

public:
	using key_type = Key;
	using value_type = typename value_container_type::value_type;
	using allocator_type = typename value_container_type::allocator_type;
};

}

template<
	class Key,
	class T,
	class Hash = std::hash<Key>, // TODO: better hash function
	class Pred = std::equal_to<Key>,
	class Allocator = std::allocator<detail::punned_key_value_pair<Key, T>>, // TODO: does this just work?
	class Bucket = detail::bucket_type>
using dense_hash_map = detail::dense_hash_table<Key, T, Hash, Pred, Allocator, Bucket, false>;

}

#endif
