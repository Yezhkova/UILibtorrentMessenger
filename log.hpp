#pragma once

#include <iostream>
#include <mutex>

inline std::mutex gLogMutex;

// LOG
#ifndef QDBG // ?
    #define LOG(expr) { \
            const std::lock_guard<std::mutex> autolock( gLogMutex ); \
            std::cout << expr << std::endl << std::flush; \
        }
#else
    #define LOG(expr) { \
            std::ostringstream out; \
            out << m_dbgOurPeerName << ": " << expr; \
            ??? \
    }
#endif

