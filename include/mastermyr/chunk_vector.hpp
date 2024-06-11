#ifndef __mastermyr_chunk_vector_hpp
#define __mastermyr_chunk_vector_hpp

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>
#include <stdexcept>

namespace myr {

namespace detail {

// TODO: move to some util file
template<class Number>
constexpr bool is_powerof2(Number v) {
	return v && ((v & (v - 1)) == 0);
}

}

// Container which stores elements in contiguous, fixed size sub-vectors called chunks.
// Sort of a mix between a deque and a vector. Has one level of indirection but most elements are stored contiguously.
// Provides smoother memory growth compared to the doubling of vector, as pointers to chunks can just be appended to 
// the internal vector.
// Iterators are stable on appending but may be invalidated on erasure.
// Meets the requirements of Container, AllocatorAwareContainer, SequenceContainer, and ReversibleContainer.
// Inspired by https://github.com/martinus/unordered_dense and https://github.com/david-grs/stable_vector
template<
	class T,
	class Allocator = std::allocator<T>,
	std::size_t ChunkSize = 4096>
class chunk_vector
{
	static_assert(detail::is_powerof2(ChunkSize), "ChunkSize must be a power of 2");
	static_assert(std::is_same_v<typename Allocator::value_type, T>, "Provided Allocator must allocate the same value type as T");

private:
	template<bool IsConst>
	class iterator_type;

public:
	using value_type = T;
	using allocator_type = Allocator;
	using allocator_traits = std::allocator_traits<allocator_type>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename allocator_traits::pointer;
	using const_pointer = typename allocator_traits::const_pointer;
	using iterator = iterator_type<false>;
	using const_iterator = iterator_type<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
	using chunk_allocator_type = typename allocator_traits::template rebind_alloc<pointer>;
	std::vector<pointer, chunk_allocator_type> m_chunks;
	size_type m_size{};

	// Iterator type which doubles as both const and non-const
	// Meets the requirements of LegacyRandomAccessIterator
	template<bool IsConst>
	class iterator_type
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using difference_type = chunk_vector::difference_type;
		using value_type = chunk_vector::value_type;
		using reference = std::conditional_t<IsConst, value_type const&, value_type&>;
		using pointer = std::conditional_t<IsConst, chunk_vector::const_pointer const*, chunk_vector::pointer*>;

	private:
		using size_type = chunk_vector::size_type;

		pointer m_data{};
		size_type m_index{};

	public:
		constexpr iterator_type() noexcept = default;
		constexpr iterator_type(pointer ptr, size_type idx) noexcept
			: m_data(ptr), m_index(idx) {}

		template<bool OtherIsConst, typename = typename std::enable_if_t<IsConst && !OtherIsConst>>
		constexpr iterator_type(iterator_type<OtherIsConst> const& other) noexcept
			: m_data(other.m_data), m_index(other.m_index) {}

		template<bool OtherIsConst, typename = typename std::enable_if_t<IsConst && !OtherIsConst>>
		constexpr auto operator=(iterator_type<OtherIsConst> const& other) noexcept -> iterator_type&
		{
			m_data = other.m_data;
			m_index = other.m_index;
			return *this;
		}

		constexpr auto operator*() const noexcept -> reference
		{
			return m_data[m_index / ChunkSize][m_index % ChunkSize];
		}

		constexpr auto operator->() const noexcept -> pointer
		{
			return &m_data[m_index / ChunkSize][m_index % ChunkSize];
		}

		constexpr auto operator++() noexcept -> iterator_type&
		{
			++m_index;
			return *this;
		}

		constexpr auto operator++(int) noexcept -> iterator_type
		{
			auto it = *this;
			++m_index;
			return it;
		}

		constexpr auto operator--() noexcept -> iterator_type&
		{
			--m_index;
			return *this;
		}

		constexpr auto operator--(int) noexcept -> iterator_type
		{
			auto it = *this;
			--m_index;
			return it;
		}

		constexpr auto operator+=(difference_type n) noexcept -> iterator_type&
		{
			m_index += n;
			return *this;
		}

		constexpr auto operator+(difference_type n) noexcept -> iterator_type
		{
			auto it = *this;
			return it += n;
		}

		constexpr auto operator-=(difference_type n) noexcept -> iterator_type&
		{
			return *this += -n;
		}

		constexpr auto operator-(difference_type n) noexcept -> iterator_type
		{
			auto it = *this;
			return it -= n;
		}

		template<bool OtherIsConst>
		constexpr auto operator-(iterator_type<OtherIsConst> const& other) noexcept -> difference_type
		{
			return static_cast<difference_type>(m_index) - static_cast<difference_type>(other.m_index);
		}

		constexpr auto operator[](difference_type n) noexcept -> reference
		{
			return *(*this + n);
		}

		template<bool OtherIsConst>
		constexpr auto operator<(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return other - *this > 0;
		}

		template<bool OtherIsConst>
		constexpr auto operator>(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return other < *this;
		}

		template<bool OtherIsConst>
		constexpr auto operator<=(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return !(*this > other);
		}

		template<bool OtherIsConst>
		constexpr auto operator>=(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return !(*this < other);
		}

		template<bool OtherIsConst>
		constexpr auto operator==(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return m_index == other.m_index;
		}

		template<bool OtherIsConst>
		constexpr auto operator!=(iterator_type<OtherIsConst> const& other) noexcept -> bool
		{
			return !(*this == other);
		}

	}; // iterator_type
	
	void add_chunk()
	{
		auto allocator = allocator_type(m_chunks.get_allocator());
		pointer chunk = allocator_traits::allocate(allocator, ChunkSize);
		m_chunks.push_back(chunk);
	}

	void maybe_increase_capacity()
	{
		if (m_size == capacity())
			add_chunk();
	}

	void deallocate()
	{
		auto allocator = allocator_type(m_chunks.get_allocator());
		for (auto* chunk_ptr : m_chunks)
			allocator_traits::deallocate(allocator, chunk_ptr, ChunkSize);
	}

	void emplace_back_all_from_other(const chunk_vector& other)
	{
		for (const auto& v : other)
			emplace_back(v);
	}

	void emplace_back_all_from_other(chunk_vector&& other)
	{
		for (auto&& v : other)
			emplace_back(std::move(v));
	}

public:

	//
	// constructors
	//

	chunk_vector() noexcept(noexcept(Allocator())) = default;

	explicit chunk_vector(const allocator_type& alloc) noexcept
		: m_chunks(alloc)
	{}

	chunk_vector(size_type count, const value_type& v = value_type(), const allocator_type& alloc = allocator_type())
		: m_chunks(alloc)
	{
		for (size_type i = 0; i < count; ++i)
		{
			emplace_back(v);
		}
	}

	chunk_vector(size_type count, const allocator_type& alloc = allocator_type())
		: m_chunks(alloc)
	{
		for (size_type i = 0; i < count; ++i)
		{
			emplace_back(value_type());
		}
	}

	template<class InputIt>
	chunk_vector(InputIt first, InputIt last, const allocator_type& alloc = allocator_type())
		: m_chunks(alloc)
	{
		for (; first != last; ++first)
		{
			emplace_back(*first);
		}
	}

	chunk_vector(const chunk_vector& other)
		: m_chunks(allocator_traits::select_on_container_copy_construction(other.get_allocator()))
	{
		emplace_back_all_from_other(other);
	}

	chunk_vector(const chunk_vector& other, const allocator_type& alloc)
		: m_chunks(alloc)
	{
		emplace_back_all_from_other(other);
	}

	chunk_vector(chunk_vector&& other) noexcept
		: chunk_vector(std::move(other), get_allocator())
	{}

	chunk_vector(chunk_vector&& other, const allocator_type& alloc)
		: m_chunks(alloc)
	{
		emplace_back_all_from_other(other);
	}

	chunk_vector(std::initializer_list<value_type> init, allocator_type alloc = allocator_type())
		: chunk_vector(init.begin(), init.end(), alloc)
	{}

	~chunk_vector()
	{
		clear();
		deallocate();
	}

	auto operator=(const chunk_vector& other) -> chunk_vector&
	{
		if (this == &other)
			return *this;

		clear();
		if (allocator_traits::propagate_on_container_copy_assignment::value && get_allocator() != other.get_allocator())
		{
			m_chunks = std::vector<pointer, chunk_allocator_type>(other.get_allocator());
		}

		emplace_back_all_from_other(other);
		return *this;
	}

	auto operator=(chunk_vector&& other) noexcept -> chunk_vector&
	{
		clear();
		deallocate();
		if (get_allocator() == other.get_allocator())
		{
			m_chunks = std::move(other.m_chunks);
			m_size = std::exchange(other.m_size, {});
			return *this;
		}

		if (allocator_traits::propagate_on_container_move_assignment::value)
		{
			m_chunks = std::vector<pointer, chunk_allocator_type>(other.get_allocator());
		}
		emplace_back_all_from_other(std::move(other));
		return *this;
	}

	auto operator=(std::initializer_list<value_type> ilist) -> chunk_vector&
	{
		clear();
		for (const auto& v : ilist)
			emplace_back(v);
		return *this;
	}

	constexpr auto get_allocator() const noexcept -> allocator_type
	{
		return m_chunks.get_allocator();
	}


	//
	// element access
	//

	[[nodiscard]]
	constexpr auto at(size_type pos) -> reference
	{
		if (pos >= size())
			throw std::out_of_range("index out of range");

		return operator[](pos);
	}

	[[nodiscard]]
	constexpr auto at(size_type pos) const -> const_reference
	{
		if (pos >= size())
			throw std::out_of_range("index out of range");

		return operator[](pos);
	}

	[[nodiscard]]
	constexpr auto operator[](size_type n) noexcept -> reference
	{
		return *(begin() + n);
	}

	[[nodiscard]]
	constexpr auto front() noexcept -> reference
	{
		return *begin();
	}

	[[nodiscard]]
	constexpr auto front() const noexcept -> const_reference
	{
		return *begin();
	}

	[[nodiscard]]
	constexpr auto back() noexcept -> reference
	{
		return *--end();
	}

	[[nodiscard]]
	constexpr auto back() const noexcept -> const_reference
	{
		return *--end();
	}


	//
	// iterators
	//

	[[nodiscard]]
	constexpr auto begin() noexcept -> iterator
	{
		return {m_chunks.data(), 0};
	}

	[[nodiscard]]
	constexpr auto begin() const noexcept -> const_iterator
	{
		return {m_chunks.data(), 0};
	}

	[[nodiscard]]
	constexpr auto cbegin() const noexcept -> const_iterator
	{
		return const_cast<const chunk_vector&>(*this).begin();
	}

	[[nodiscard]]
	constexpr auto end() noexcept -> iterator
	{
		return {m_chunks.data(), m_size};
	}

	[[nodiscard]]
	constexpr auto end() const noexcept -> const_iterator
	{
		return {m_chunks.data(), m_size};
	}

	[[nodiscard]]
	constexpr auto cend() const noexcept -> const_iterator
	{
		return const_cast<const chunk_vector&>(*this).end();
	}

	[[nodiscard]]
	constexpr auto rbegin() noexcept -> reverse_iterator
	{
		return std::make_reverse_iterator(end());
	}

	[[nodiscard]]
	constexpr auto rbegin() const noexcept -> const_reverse_iterator
	{
		return std::make_reverse_iterator(end());
	}

	[[nodiscard]]
	constexpr auto crbegin() const noexcept -> const_reverse_iterator
	{
		return std::make_reverse_iterator(const_cast<const chunk_vector&>(*this).end());
	}

	[[nodiscard]]
	constexpr auto rend() noexcept -> reverse_iterator
	{
		return std::make_reverse_iterator(begin());
	}

	[[nodiscard]]
	constexpr auto rend() const noexcept -> const_reverse_iterator
	{
		return std::make_reverse_iterator(begin());
	}

	[[nodiscard]]
	constexpr auto crend() const noexcept -> const_reverse_iterator
	{
		return std::make_reverse_iterator(const_cast<const chunk_vector&>(*this).begin());
	}

	// 
	// capacity
	//

	[[nodiscard]]
	constexpr auto empty() noexcept -> bool
	{
		return m_size == 0;
	}

	[[nodiscard]]
	constexpr auto size() noexcept -> size_type
	{
		return m_size;
	}

	void reserve(size_type new_cap)
	{
		const auto old_cap = capacity();
		for (size_type i = new_cap - old_cap; i > 0; i -= ChunkSize)
		{
			add_chunk();
		}
	}

	[[nodiscard]]
	constexpr auto capacity() noexcept -> size_type
	{
		return m_chunks.size() * ChunkSize;
	}

	//
	// modifiers
	//

	constexpr void clear()
	{
		if constexpr (!std::is_trivially_destructible_v<value_type>)
		{
			for (size_type i = 0; i < m_size; ++i)
			{
				operator[](i).~value_type();
			}
		}
		m_size = 0;
	}

	// no insert or emplace to ensure all insert methods won't invalidate iterators
	// potentially might add this later on anyway
	
	auto erase(iterator pos) -> iterator
	{
		if constexpr (!std::is_trivially_destructible_v<value_type>)
		{
			*pos.~value_type();
		}
		std::move(pos + 1, end(), pos);
		pop_back();

		return pos;
	}

	auto erase(const_iterator pos) -> iterator
	{
		return erase(const_cast<iterator>(pos));
	}

	auto erase(iterator first, iterator last) -> iterator
	{
		if (first >= last)
			return;

		if constexpr (!std::is_trivially_destructible_v<value_type>)
		{
			for (auto it = first; it != last; ++it)
				*it.~value_type();
		}

		std::move(last, end(), first);
		erase(end() - (last - first), end());
		m_size -= last - first;
	}

	auto erase(const_iterator first, const_iterator last)
	{
		return erase(const_cast<iterator>(first), const_cast<iterator>(last));
	}

	void push_back(const value_type& v)
	{
		maybe_increase_capacity();
		operator[](m_size) = v;
		++m_size;
	}

	void push_back(value_type&& v)
	{
		maybe_increase_capacity();
		operator[](m_size) = std::move(v);
		++m_size;
	}

	template<class... Args>
	auto emplace_back(Args&&... args) -> reference
	{
		maybe_increase_capacity();

		auto* ptr = static_cast<void*>(&operator[](m_size));
		auto& ref = *new (ptr) T(std::forward<Args>(args)...);
		++m_size;
		return ref;
	}

	// non-standard API, "the superconstructing super elider"
	// https://quuxplusone.github.io/blog/2018/03/29/the-superconstructing-super-elider/
	template<class Factory>
	auto emplace_back_with_result_of(const Factory& f) -> reference
	{
		maybe_increase_capacity();

		auto* ptr = static_cast<void*>(&operator[](m_size));
		auto& ref = *new (ptr) T(f());
		++m_size;
		return ref;
	}

	void pop_back()
	{
		if (m_size == 0)
			return;

		if (!std::is_trivially_destructible_v<value_type>)
		{
			operator[](m_size - 1).~value_type();
		}
		--m_size;
	}

	//
	// comparators
	//

	constexpr auto operator==(const chunk_vector& other) noexcept -> bool
	{
		return m_size == other.m_size && std::equal(begin(), end(), other.begin(), other.end());
	}

	constexpr auto operator!=(const chunk_vector& other) noexcept -> bool
	{
		return !(*this == other);
	}
};

}

#endif
