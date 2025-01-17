#ifdef CPPTRACE_FULL_TRACE_WITH_LIBBACKTRACE

#include <cpptrace/cpptrace.hpp>
#include "cpptrace_full_trace.hpp"
#include "../platform/cpptrace_program_name.hpp"
#include "../platform/cpptrace_common.hpp"

#include <vector>

#ifdef CPPTRACE_BACKTRACE_PATH
#include CPPTRACE_BACKTRACE_PATH
#else
#include <backtrace.h>
#endif

namespace cpptrace {
    namespace detail {
        struct trace_data {
            std::vector<stacktrace_frame>& frames;
            size_t& skip;
        };

        int full_callback(void* data_pointer, uintptr_t address, const char* file, int line, const char* symbol) {
            trace_data& data = *reinterpret_cast<trace_data*>(data_pointer);
            if(data.skip > 0) {
                data.skip--;
            } else {
                data.frames.push_back({
                    address,
                    line,
                    -1,
                    file ? file : "",
                    symbol ? symbol : ""
                });
            }
            return 0;
        }

        void syminfo_callback(void* data, uintptr_t, const char* symbol, uintptr_t, uintptr_t) {
            stacktrace_frame& frame = *static_cast<stacktrace_frame*>(data);
            frame.symbol = symbol ? symbol : "";
        }

        void error_callback(void*, const char*, int) {
            // nothing for now
        }

        backtrace_state* get_backtrace_state() {
            // backtrace_create_state must be called only one time per program
            static backtrace_state* state = nullptr;
            static bool called = false;
            if(!called) {
                state = backtrace_create_state(program_name(), true, error_callback, nullptr);
                called = true;
            }
            return state;
        }

        CPPTRACE_FORCE_NO_INLINE
        std::vector<stacktrace_frame> generate_trace(size_t skip) {
            std::vector<stacktrace_frame> frames;
            skip++; // add one for this call
            trace_data data { frames, skip };
            backtrace_full(get_backtrace_state(), 0, full_callback, error_callback, &data);
            for(auto& frame : frames) {
                if(frame.symbol.empty()) {
                    // fallback, try to at least recover the symbol name with backtrace_syminfo
                    backtrace_syminfo(
                        get_backtrace_state(),
                        reinterpret_cast<uintptr_t>(frame.address),
                        syminfo_callback,
                        error_callback,
                        &frame
                    );
                }
            }
            return frames;
        }
    }
}

#endif
