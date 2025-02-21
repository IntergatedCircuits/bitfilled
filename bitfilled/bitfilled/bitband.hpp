// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_BITBAND_HPP__
#define __BITFILLED_BITBAND_HPP__

#include "bitfilled/bits.hpp"

namespace bitfilled
{

template <BitwiseAccessible T, enum access ACCESS, std::uintptr_t BASE_ADDRESS>
struct bitband_ops : private bitfield_ops<T, ACCESS>
{
  private:
    using base_ops = bitfield_ops<T, ACCESS>;
    using base_ops::access;
    using base_ops::int_type;
    using base_ops::memory;
    static_assert((BASE_ADDRESS & 0x9fffffff) == 0);
    static constexpr std::uintptr_t BITBAND_BASE_ADDRESS = BASE_ADDRESS | 0x02000000;

    template <typename Tptr>
    static auto& bitmemory(Tptr& ptr, std::size_t bit_index)
    {
        auto address = BITBAND_BASE_ADDRESS;
        address |= (((std::uintptr_t)&ptr & 0xfffff) << 5);
        address |= (bit_index << 2);
        return *((copy_cv_t<Tptr*, std::uint32_t*>)address);
    }

  public:
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
        requires(is_writeable<base_ops::access()>)
    {
        if constexpr (FIRST_BIT == LAST_BIT)
        {
            const auto v = static_cast<base_ops::int_type>(value);
            bitmemory(bf, FIRST_BIT) = v;
        }
        else
        {
            return base_ops::set_field(bf, value);
        }
    }
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
        requires(is_writeable<base_ops::access()>)
    {
        if constexpr (FIRST_BIT == LAST_BIT)
        {
            const auto v = static_cast<base_ops::int_type>(value);
            bitmemory(bf, FIRST_BIT) = v;
        }
        else
        {
            return base_ops::set_field(bf, value);
        }
    }
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const bitfield_props<FIRST_BIT, LAST_BIT>& bf)
        requires(is_readable<base_ops::access()>)
    {
        if constexpr (FIRST_BIT == LAST_BIT)
        {
            auto x = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
            return bf.sign_extend(x);
        }
        else
        {
            return base_ops::template get_field<TVal>(bf);
        }
    }
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf)
        requires(is_readable<base_ops::access()>)
    {
        if constexpr (FIRST_BIT == LAST_BIT)
        {
            auto x = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
            return bf.sign_extend(x);
        }
        else
        {
            return base_ops::template get_field<TVal>(bf);
        }
    }
    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf, std::size_t index,
                         TVal value)
        requires(is_writeable<base_ops::access()>)
    {
        if constexpr (ITEM_SIZE == 1)
        {
            const auto v = static_cast<base_ops::int_type>(value);
            bitmemory(bf, OFFSET + index) = v;
        }
        else
        {
            return base_ops::set_item(bf, index, value);
        }
    }
    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                         std::size_t index, TVal value)
        requires(is_writeable<base_ops::access()>)
    {
        if constexpr (ITEM_SIZE == 1)
        {
            const auto v = static_cast<base_ops::int_type>(value);
            bitmemory(bf, OFFSET + index) = v;
        }
        else
        {
            return base_ops::set_item(bf, index, value);
        }
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                         std::size_t index)
        requires(is_readable<base_ops::access()>)
    {
        if constexpr (ITEM_SIZE == 1)
        {
            auto x = static_cast<TVal>(bitmemory(bf, OFFSET + index));
            return bf.sign_extend(x);
        }
        else
        {
            return base_ops::template get_item<TVal>(bf, index);
        }
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                         std::size_t index)
        requires(is_readable<base_ops::access()>)
    {
        if constexpr (ITEM_SIZE == 1)
        {
            auto x = static_cast<TVal>(bitmemory(bf, OFFSET + index));
            return bf.sign_extend(x);
        }
        else
        {
            return base_ops::template get_item<TVal>(bf, index);
        }
    }
};

} // namespace bitfilled

#endif // __BITFILLED_BITBAND_HPP__
