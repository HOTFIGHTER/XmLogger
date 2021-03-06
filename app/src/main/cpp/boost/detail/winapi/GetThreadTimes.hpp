//  GetThreadTimes.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba
//  Copyright 2015 Andrey Semashev

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_GETTHREADTIMES_HPP
#define BOOST_DETAIL_WINAPI_GETTHREADTIMES_HPP

#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/time.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
BOOST_SYMBOL_IMPORT mars_boost::detail::winapi::BOOL_
WINAPI
GetThreadTimes(
    mars_boost::detail::winapi::HANDLE_ hThread,
    ::_FILETIME *lpCreationTime,
    ::_FILETIME *lpExitTime,
    ::_FILETIME *lpKernelTime,
    ::_FILETIME *lpUserTime);
}
#endif

namespace mars_boost {}
namespace boost = mars_boost;
namespace mars_boost {
namespace detail {
namespace winapi {

BOOST_FORCEINLINE BOOL_
GetThreadTimes(
    HANDLE_
hThread,
LPFILETIME_ lpCreationTime,
    LPFILETIME_
lpExitTime,
LPFILETIME_ lpKernelTime,
    LPFILETIME_
lpUserTime) {
return
::GetThreadTimes(
    hThread,
reinterpret_cast
<::_FILETIME *>(lpCreationTime),
reinterpret_cast
<::_FILETIME *>(lpExitTime),
reinterpret_cast
<::_FILETIME *>(lpKernelTime),
reinterpret_cast
<::_FILETIME *>(lpUserTime)
);
}

}
}
}

#endif // BOOST_DETAIL_WINAPI_GETTHREADTIMES_HPP
