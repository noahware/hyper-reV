#include "slab.hpp"
#include "../../arch/defs.hpp"
#include "page.hpp"
#include <heap.hpp>
#include <array.hpp>
#include <mutex.hpp>

namespace
{
	constexpr std::size_t slab_pages = hva_heap_page_count;

	constexpr cstd::array<std::size_t, 8> size_classes = { 16, 32, 64, 128, 256, 512, 1024, 2048 };
	constexpr std::size_t bucket_count = size_classes.size();
	constexpr std::size_t invalid_bucket = bucket_count;
	
	constexpr std::size_t max_raw_pages = 63; // page_info::value is 6 bits

	struct page_info
	{
		enum class kind : std::uint8_t
		{
			not_managed = 0,
			slab,
			raw_pages,
			reserved
		};

		kind         kind : 2 = kind::not_managed;
		std::uint8_t value : 6 = 0;
	};

	static_assert(sizeof(page_info) == 1);

	struct slot
	{
		slot* next;
	};

	struct bucket
	{
		slot* head = nullptr;
		cstd::mutex mutex;
	};

	cstd::array<page_info, slab_pages> page_table;
	cstd::array<bucket, bucket_count> buckets;

	[[nodiscard]] std::size_t bucket_for(const std::size_t size) noexcept
	{
		for (std::size_t i = 0; i < bucket_count; i++)
			if (size <= size_classes[i])
				return i;

		return invalid_bucket;
	}

	[[nodiscard]] slot* partition_page(const std::size_t bucket) noexcept
	{
		void* const p = mem::page::alloc();

		if (!p)
			return nullptr;

		page_table[mem::page::index_of(p)] = page_info{
			.kind = page_info::kind::slab,
			.value = static_cast<std::uint8_t>(bucket)
		};

		const std::size_t slot_size = size_classes[bucket];
		const std::size_t slot_count = page_size / slot_size;

		slot* head = nullptr;

		const auto begin = static_cast<std::uint8_t*>(p);
		const auto end = begin + (slot_size * slot_count);

		for (auto curr = begin; curr < end; curr += slot_size)
		{
			const auto s = reinterpret_cast<slot*>(curr);

			s->next = head;

			head = s;
		}

		return head;
	}


	[[nodiscard]] slot* gen_bucket_head(bucket& b, const std::size_t bi) noexcept
	{
		if (!b.head)
			b.head = partition_page(bi);

		// paritioning failed
		if (!b.head)
			return nullptr;

		return b.head;
	}

	void* alloc_slab(const std::size_t bi) noexcept
	{
		bucket& b = buckets[bi];

		cstd::lock_guard lock(b.mutex);

		slot* head = gen_bucket_head(b, bi);

		if (!head)
			return nullptr;

		b.head = head->next;

		return head;
	}

	void* alloc_raw_pages(const std::size_t size) noexcept
	{
		const std::size_t pages = (size + page_size - 1) / page_size;

		if (pages == 0 || pages > max_raw_pages)
			return nullptr;

		void* const p = mem::page::alloc(pages);

		if (!p)
			return nullptr;

		page_table[mem::page::index_of(p)] = page_info{
			page_info::kind::raw_pages,
			static_cast<std::uint8_t>(pages),
		};

		return p;
	}
}

void* mem::slab::alloc(const std::size_t size) noexcept
{
	if (size == 0)
		return nullptr;

	const std::size_t bi = bucket_for(size);

	if (bi == invalid_bucket)
		return alloc_raw_pages(size);

	return alloc_slab(bi);
}

void mem::slab::free(void* const p) noexcept
{
	if (!p)
		return;

	const auto& page = page_table[page::index_of(p)];

	if (page.kind == page_info::kind::slab)
	{
		const std::size_t bi = page.value;
		bucket& b = buckets[bi];

		cstd::lock_guard lock(b.mutex);

		const auto s = static_cast<slot*>(p);
		s->next = b.head;

		b.head = s;
	}
	else if (page.kind == page_info::kind::raw_pages)
	{
		const std::size_t page_count = page.value;

		page_table[page::index_of(p)] = {};

		page::free(p, page_count);
	}
}
