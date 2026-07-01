#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
extern "C" const char *__lsan_default_suppressions() {
  return "leak:libvulkan.so\n"
         "leak:radv\n"
         "leak:radeonsi\n";
}
#endif
#endif
