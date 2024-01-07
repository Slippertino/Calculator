#pragma once

#include <list>
#include <cstddef>
#include <array>
#include <memory_resource>

template<typename T, size_t Size>
class OptimizedListWrapper {
public:
	using ListType = std::pmr::list<T>;

private:
#ifdef _MSC_VER
	static constexpr size_t TotalNodeSize = sizeof(
		std::_List_node<
			T, 
			typename std::allocator_traits<typename ListType::allocator_type>::void_pointer
		>
	);
#else
	static constexpr size_t TotalNodeSize = sizeof(std::_List_node<T>);
#endif

	std::array<std::byte, Size * TotalNodeSize> buffer_;
	std::pmr::monotonic_buffer_resource resource_{ buffer_.data(), buffer_.size() };
	std::pmr::polymorphic_allocator<T> allocator_{ &resource_ };

public:
	OptimizedListWrapper() = default;

	template<size_t TSize>
	OptimizedListWrapper(const OptimizedListWrapper<T, TSize> &lst) {
		std::copy(
			lst.list.begin(),
			lst.list.end(),
			std::back_inserter(list)
		);
	}

	template<size_t TSize>
	OptimizedListWrapper(OptimizedListWrapper<T, TSize>&& lst) {
		std::copy(
			std::make_move_iterator(lst.list.begin()),
			std::make_move_iterator(lst.list.end()),
			std::back_inserter(list)
		);
	}

	auto begin() {
		return list.begin();
	}

	auto end() {
		return list.end();
	}

public:
	ListType list{ allocator_ };
};