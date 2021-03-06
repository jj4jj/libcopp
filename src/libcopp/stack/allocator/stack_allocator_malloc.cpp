#include <memory>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <assert.h>
#include <limits>

#include "libcopp/stack/stack_context.h"
#include "libcopp/stack/allocator/stack_allocator_malloc.h"

// x86_64
// test x86_64 before i386 because icc might
// define __i686__ for x86_64 too
#if defined(__x86_64__) || defined(__x86_64) \
    || defined(__amd64__) || defined(__amd64) \
    || defined(_M_X64) || defined(_M_AMD64)

// Windows seams not to provide a constant or function
// telling the minimal stacksize
# define MIN_STACKSIZE  8 * 1024
#else
# define MIN_STACKSIZE  4 * 1024
#endif

#ifdef COPP_HAS_ABI_HEADERS
# include COPP_ABI_PREFIX
#endif

namespace copp { 
    namespace allocator {
        namespace sys
        {
            static std::size_t pagesize()
            {
                return MIN_STACKSIZE;
            }

            static std::size_t round_to_page_size(std::size_t stacksize)
            {
                // page size must be 2^N
                return static_cast<std::size_t>((stacksize + pagesize() - 1) & (~(pagesize() - 1)));
            }
        }

        stack_allocator_malloc::stack_allocator_malloc(){}
        stack_allocator_malloc::~stack_allocator_malloc() { }


        bool stack_allocator_malloc::is_stack_unbound() { return true; }

        std::size_t stack_allocator_malloc::default_stacksize() {
            std::size_t size = 64 * 1024; // 64 KB
            if (is_stack_unbound())
                return (std::max)(size, minimum_stacksize() );

            assert(maximum_stacksize() >= minimum_stacksize());
            return maximum_stacksize() == minimum_stacksize()
                ? minimum_stacksize()
                : (std::min)(size, maximum_stacksize());
        }

        std::size_t stack_allocator_malloc::minimum_stacksize() { return MIN_STACKSIZE; }

        std::size_t stack_allocator_malloc::maximum_stacksize() {
            assert(is_stack_unbound());
            return std::numeric_limits<std::size_t>::max();
        }

        void stack_allocator_malloc::allocate(stack_context & ctx, std::size_t size)
        {
            size = (std::max)(size, minimum_stacksize());
            size = (std::min)(size, maximum_stacksize());

            std::size_t size_ = sys::round_to_page_size(size);

            void* start_ptr = malloc(size_);

            if (!start_ptr) {
                ctx.sp = NULL;
                return;
            }

            ctx.size = size_;
            ctx.sp = static_cast<char *>(start_ptr) + ctx.size; // stack down
        }

        void stack_allocator_malloc::deallocate(stack_context & ctx)
        {
            assert(ctx.sp);
            assert(minimum_stacksize() <= ctx.size);
            assert(is_stack_unbound() || (maximum_stacksize() >= ctx.size));

            void* start_ptr = static_cast< char * >(ctx.sp) - ctx.size;
            free(start_ptr);
        }

    } 
}

#ifdef COPP_HAS_ABI_HEADERS
# include COPP_ABI_SUFFIX
#endif
