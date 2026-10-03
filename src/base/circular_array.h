#pragma once

#include "types.h"
#include <array>
#include <optional>

template<typename T, u32 N>
class CircularArray 
{
    static_assert(N > 0, "CircularArray capacity must be greater than 0");

public:
    template<bool IsConst>
    class IteratorImpl
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;
        using reference         = std::conditional_t<IsConst, const T&, T&>;

        IteratorImpl() = default;
        IteratorImpl(std::conditional_t<IsConst, const CircularArray*, CircularArray*> container, u32 index)
            : m_container(container), m_index(index) {}

        template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
        IteratorImpl(const IteratorImpl<OtherConst>& other)
            : m_container(other.m_container), m_index(other.m_index) {}

        auto operator*() const -> reference { return (*m_container)[m_index]; }
        auto operator->() const -> pointer  { return &(*m_container)[m_index]; }

        auto operator++() -> IteratorImpl& { ++m_index; return *this; }
        auto operator++(int) -> IteratorImpl { IteratorImpl tmp = *this; ++m_index; return tmp; }

        auto operator--() -> IteratorImpl& { --m_index; return *this; }
        auto operator--(int) -> IteratorImpl { IteratorImpl tmp = *this; --m_index; return tmp; }

        auto operator+=(difference_type n) -> IteratorImpl& { m_index += n; return *this; }
        auto operator-=(difference_type n) -> IteratorImpl& { m_index -= n; return *this; }

        friend auto operator+(IteratorImpl it, difference_type n) -> IteratorImpl { it += n; return it; }
        friend auto operator+(difference_type n, IteratorImpl it) -> IteratorImpl { it += n; return it; }
        friend auto operator-(IteratorImpl it, difference_type n) -> IteratorImpl { it -= n; return it; }
        friend auto operator-(const IteratorImpl& lhs, const IteratorImpl& rhs) -> difference_type
        {
            return static_cast<difference_type>(lhs.m_index) - static_cast<difference_type>(rhs.m_index);
        }

        auto operator[](difference_type n) const -> reference { return *(*this + n); }
        auto operator==(const IteratorImpl& other) const -> bool { return m_index == other.m_index; }
        auto operator<=>(const IteratorImpl& other) const = default;
    private:
        template<bool> friend class IteratorImpl;
        std::conditional_t<IsConst, const CircularArray*, CircularArray*> m_container{ nullptr };
        u32 m_index;
    };

    using iterator       = IteratorImpl<false>;
    using const_iterator = IteratorImpl<true>;

    auto push_back(T t) -> void
    {
        array[tail] = t;
        tail = (tail + 1) % N;
        if (count == N)
        {
            head = (head + 1) % N;
        }
        else
        {
            ++count;
        }
    }

    auto pop() -> std::optional<T>
    {
        if (count == 0)
        {
            return std::nullopt;
        }

        u32 current = head;
        head = (head + 1) % N;
        --count;
        return array[current];
    }

    auto remove_at(u32 index) -> std::optional<T>
    {
        if (index >= count)
        {
            return std::nullopt;
        }

        u32 target = (head + index) % N;
        T removed = array[target];

        if (index < (u32)(count / 2))
        {
            for (u32 i = index; i > 0; --i)
            {
                u32 current = (head + i) % N;
                u32 prev = (head + i - 1) % N;
                array[current] = array[prev];
            }
            head = (head + 1) % N;
        }
        else
        {
            for (u32 i = index; i + 1 < count; ++i)
            {
                u32 current = (head + i) % N;
                u32 next = (head + i + 1) % N;
                array[current] = array[next];
            }
            tail = (tail + N - 1) % N;
        }

        --count;
        return removed;
    }

    auto swap_remove_at(u32 index) -> std::optional<T>
    {
        if (index >= count)
        {
            return std::nullopt;
        }

        u32 target = (head + index) % N;
        u32 last = (head + count - 1) % N;
        T removed = array[target];

        if (target != last)
        {
            array[target] = array[last];
        }

        tail = (tail + N - 1) % N;
        --count;
        return removed;
    }

    [[nodiscard]] auto operator[](u32 index) -> T&
    {
        return array[(head + index) % N];
    }
    [[nodiscard]] auto operator[](u32 index) const -> const T&
    {
        return array[(head + index) % N];
    }
    [[nodiscard]] auto size() -> u32
    {
        return count;
    }

    [[nodiscard]] auto begin() -> iterator { return iterator(this, 0); }
    [[nodiscard]] auto end()   -> iterator { return iterator(this, count); }

    [[nodiscard]] auto begin() const -> const_iterator { return const_iterator(this, 0); }
    [[nodiscard]] auto end()   const -> const_iterator { return const_iterator(this, count); }

    [[nodiscard]] auto cbegin() const -> const_iterator { return const_iterator(this, 0); }
    [[nodiscard]] auto cend()   const -> const_iterator { return const_iterator(this, count); }

private:
    std::array<T, N> array{};
    u32 head{ 0 };
    u32 tail{ 0 };
    u32 count{ 0 };
};
