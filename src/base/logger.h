#pragma once

#include "types.h"

#include <print>
#include <format>
#include <string_view>

namespace logger
{
    enum class TextColor
    {
        Red,
        Green,
        Yellow,
        None
    };

    constexpr static std::string_view colorTable[] = {
        "\033[31m", // Red
        "\033[32m", // Green
        "\033[33m", // Yellow
        "\033[0m",  // None
    };

    inline auto colorToString(TextColor color) -> std::string_view
    {
        return colorTable[(i32)color];
    }

    enum class Level
    {
        Debug = 1,
        Info = 2,
        Warning = 4,
        Error = 8,
    };

    inline auto operator&(Level a, Level b) -> Level
    {
        return (Level)((i32)a & (i32)b);
    }

    inline auto operator|(Level a, Level b) -> Level
    {
        return (Level)((i32)a | (i32)b);
    }

#ifdef DEBUG
    constexpr static bool debug = true;
#else
    constexpr static bool debug = false;
#endif

    auto setFilter(Level level, bool isInclusive) -> void;

    auto checkLevel(Level level) -> bool;

    inline auto _print(TextColor color, std::string_view tag, std::string&& msg) -> void
    {
        std::print("{}{} {}{}\n", colorToString(color), tag, msg, colorToString(TextColor::None));
    }

    template<typename... T>
    inline auto log(std::format_string<T...> msg, T&&... args) -> void
    {
        if constexpr (debug)
        {
            if (checkLevel(Level::Debug))
            {
                _print(TextColor::None, "[DEBUG]", std::format(msg, std::forward<T>(args)...));
            }
        }
    }

    template<typename... T>
    inline auto logInfo(std::format_string<T...> msg, T&&... args) -> void
    {
        if constexpr (debug)
        {
            if (checkLevel(Level::Info))
            {
                _print(TextColor::Green, "[INFO]", std::format(msg, std::forward<T>(args)...));
            }
        }
    }

    template<typename... T>
    inline auto logWarning(std::format_string<T...> msg, T&&... args) -> void
    {
        if constexpr (debug)
        {
            if (checkLevel(Level::Warning))
            {
                _print(TextColor::Yellow, "[WARNING]", std::format(msg, std::forward<T>(args)...));
            }
        }
    }

    template<typename... T>
    inline auto logError(std::format_string<T...> msg, T&&... args) -> void
    {
        if constexpr (debug)
        {
            if (checkLevel(Level::Error))
            {
                _print(TextColor::Red, "[ERROR]", std::format(msg, std::forward<T>(args)...));
            }
        }
    }

    template<typename... T>
    inline auto log(Level level, std::format_string<T...> msg, T&&... args) -> void
    {
        switch (level)
        {
            case Level::Debug:
                log(msg, args...);
                break;
            case Level::Info:
                logInfo(msg, args...);
                break;
            case Level::Warning:
                logWarning(msg, args...);
                break;
            case Level::Error:
                logError(msg, args...);
                break;
            default:
                break;
        }
    }
}
