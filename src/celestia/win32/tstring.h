#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>

#include <windows.h>

namespace celestia::win32
{

using tstring = std::basic_string<TCHAR>;
using tstring_view = std::basic_string_view<TCHAR>;

// UTF-8 to TCHAR, fixed buffer
int UTF8ToTChar(std::string_view str, tstring::value_type* dest, int destSize);

// UTF-8 to TCHAR, growable
template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, TCHAR>, int> = 0>
int
UTF8ToTChar(std::string_view source, T& destination)
{
    if (source.empty())
    {
        destination.clear();
        return 0;
    }

    const auto sourceSize = static_cast<int>(source.size());
    int wideLength = MultiByteToWideChar(CP_UTF8, 0, source.data(), sourceSize, nullptr, 0);
    if (wideLength <= 0)
    {
        destination.clear();
        return 0;
    }
#ifdef _UNICODE
    destination.resize(static_cast<std::size_t>(wideLength));
    return MultiByteToWideChar(CP_UTF8, 0, source.data(), sourceSize, destination.data(), wideLength);
#else
    fmt::basic_memory_buffer<wchar_t> buffer;
    buffer.resize(static_cast<std::size_t>(wideLength));
    MultiByteToWideChar(CP_UTF8, 0, source.data(), sourceSize, buffer.data(), wideLength);

    int destLength = WideCharToMultiByte(CP_ACP, 0, buffer.data(), wideLength, nullptr, 0, nullptr, nullptr);
    if (destLength <= 0)
    {
        destination.clear();
        return 0;
    }
    destination.resize(static_cast<std::size_t>(destLength));
    return WideCharToMultiByte(CP_ACP, 0, buffer.data(), wideLength, destination.data(), destLength, nullptr, nullptr);
#endif
}

inline tstring
UTF8ToTString(std::string_view str)
{
    tstring result;
    UTF8ToTChar(str, result);
    return result;
}

#ifndef _UNICODE
template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, char>, int> = 0>
int
WideToCurrentCP(std::wstring_view source, T& destination)
{
    if (source.empty())
    {
        destination.clear();
        return 0;
    }
    const auto sourceSize = static_cast<int>(source.size());
    int destLength = WideCharToMultiByte(CP_ACP, 0, source.data(), sourceSize, nullptr, 0, nullptr, nullptr);
    if (destLength <= 0)
    {
        destination.clear();
        return 0;
    }

    destination.resize(static_cast<std::size_t>(destLength));
    return WideCharToMultiByte(CP_ACP, 0, source.data(), sourceSize, destination.data(), destLength, nullptr, nullptr);
}

inline std::string
WideToCurrentCP(std::wstring_view wstr)
{
    std::string result;
    WideToCurrentCP(wstr, result);
    return result;
}

#endif

int CompareUTF8Localized(std::string_view, std::string_view);

} // end namespace celestia::win32
