#include "tstring.h"

namespace celestia::win32
{

namespace
{

template<std::size_t SIZE, typename Allocator>
int
UTF8ToWideBuffer(std::string_view str, fmt::basic_memory_buffer<wchar_t, SIZE, Allocator>& buffer)
{
    if (str.empty())
        return 0;

    const auto srcLength = static_cast<int>(str.size());
    const auto wideLength = MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLength, nullptr, 0);
    if (wideLength <= 0)
        return wideLength;

    buffer.resize(static_cast<std::size_t>(wideLength));
    return MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLength, buffer.data(), wideLength);
}

} // end unnamed namespace

int
UTF8ToTChar(std::string_view str, tstring::value_type* dest, int destSize)
{
    if (str.empty())
        return 0;
    const auto srcLength = static_cast<int>(str.size());
#ifdef _UNICODE
    return MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLength, dest, destSize);
#else
    fmt::basic_memory_buffer<wchar_t> wbuffer;
    int wideLength = UTF8ToWideBuffer(str, wbuffer);
    if (wideLength <= 0)
        return wideLength;

    wbuffer.resize(static_cast<std::size_t>(wideLength));

    MultiByteToWideChar(CP_UTF8, 0, str.data(), srcLength, wbuffer.data(), wideLength);

    return WideCharToMultiByte(CP_ACP, 0, wbuffer.data(), wideLength, dest, destSize, nullptr, nullptr);
#endif
}


int
CompareUTF8Localized(std::string_view lhs, std::string_view rhs)
{
    if (lhs.empty())
        return rhs.empty() ? 0 : -1;
    if (rhs.empty())
        return 1;

    fmt::basic_memory_buffer<wchar_t, 256> wbuffer0;
    int wlength0 = UTF8ToWideBuffer(lhs, wbuffer0);
    if (wlength0 <= 0)
        return 0;

    fmt::basic_memory_buffer<wchar_t, 256> wbuffer1;
    int wlength1 = UTF8ToWideBuffer(rhs, wbuffer1);
    if (wlength1 <= 0)
        return 0;

    int result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_LINGUISTIC_CASING,
                                 wbuffer0.data(), wlength0,
                                 wbuffer1.data(), wlength1,
                                 nullptr, nullptr, 0);
    if (result > 0)
        result -= 2;

    return result;
}

}
