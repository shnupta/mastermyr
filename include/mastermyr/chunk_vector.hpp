#ifndef __mastermyr_chunk_vector_hpp
#define __mastermyr_chunk_vector_hpp

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace myr {

// Container which stores elements in contiguous, fixed size sub-vectors called chunks.
// Sort of a mix between a deque and a vector. Has one level of indirection but most elements are stored contiguously.
// Provides smoother memory growth compared to the doubling of vector, as chunks can just be appended to the internal
// vector which holds the allocated chunks.
// Iterators are stable on insert but may be invalidated on erasure.
// Meets the requirements of Container, AllocatorAwareContainer, SequenceContainer, and ReversibleContainer.
// Inspired by https://github.com/martinus/unordered_dense and https://github.com/david-grs/stable_vector
template<
	class T,
	class Allocator = std::allocator<T>,
	std::size_t ChunkSize = 4096>
class chunk_vector
{
private:
	template<bool IsConst>
	class iterator_type;

public:
	using value_type = T;
	using allocator_type = Allocator;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = typename std::allocator_traits<Allocator>::pointer;
	using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
	using iterator = iterator_type<false>;
	using const_iterator = iterator_type<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
	using chunk_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<pointer>;
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
	
	constexpr void add_chunk()
	{
		auto allocator = Allocator(m_chunks.get_allocator());
		pointer chunk = std::allocator_traits<Allocator>::allocate(allocator, ChunkSize);
		m_chunks.push_back(chunk);
	}

public:

	constexpr chunk_vector() = default;

	// TODO: constructors (with allocator support) and assignment operators

	constexpr auto begin() noexcept -> iterator
	{
		return {m_chunks.data(), 0};
	}

	constexpr auto begin() const noexcept -> const_iterator
	{
		return {m_chunks.data(), 0};
	}

	constexpr auto cbegin() const noexcept -> const_iterator
	{
		return const_cast<const chunk_vector&>(*this).begin();
	}

	constexpr auto end() noexcept -> iterator
	{
		return {m_chunks.data(), m_size};
	}

	constexpr auto end() const noexcept -> const_iterator
	{
		return {m_chunks.data(), m_size};
	}

	constexpr auto cend() const noexcept -> const_iterator
	{
		return const_cast<const chunk_vector&>(*this).end();
	}

	constexpr auto size() noexcept -> size_type
	{
		return m_size;
	}

	constexpr auto capacity() noexcept -> size_type
	{
		return m_chunks.size() * ChunkSize;
	}

	constexpr auto operator==(const chunk_vector& other) noexcept -> bool
	{
		return m_size == other.m_size && std::equal(begin(), end(), other.begin(), other.end());
	}

	constexpr auto operator!=(const chunk_vector& other) noexcept -> bool
	{
		return !(*this == other);
	}

	constexpr auto operator[](size_type n) noexcept -> reference
	{
		return *(begin() + n);
	}

	template<class... Args>
	constexpr auto emplace_back(Args&&... args) -> reference
	{
		if (m_size == capacity())
				add_chunk();

		auto* ptr = static_cast<void*>(&operator[](m_size));
		auto& ref = *new (ptr) T(std::forward<Args>(args)...);
		++m_size;
		return ref;
	}

	// TODO: other member funcs

	// non-standard API, "the superconstructing super elider"
	// https://quuxplusone.github.io/blog/2018/03/29/the-superconstructing-super-elider/
	template<class Factory>
	constexpr auto emplace_back_with_result_of(const Factory& f) -> reference
	{
		if (m_size == capacity())
			add_chunk();

		auto* ptr = static_cast<void*>(&operator[](m_size));
		auto& ref = *new (ptr) T(f());
		++m_size;
		return ref;
	}


};

}

#endif
