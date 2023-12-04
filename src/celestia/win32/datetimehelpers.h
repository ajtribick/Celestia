#pragma once

#include <celutil/array_view.h>
#include "tstring.h"

namespace celestia::win32
{

util::array_view<tstring> GetLocalizedMonthNames();

} // end namespace celestia::win32
