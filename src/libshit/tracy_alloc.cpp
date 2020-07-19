#include "libshit/platform.hpp"

// unfortunately asan on linux creates link errors if we try to override global
// alloc funcs
#if !LIBSHIT_HAS_ASAN

#include <Tracy.hpp>

#include <cstdlib>
#include <new>

// Experimental, only supported on Linux.
#define OVERRIDE_MALLOC 0

// Set to true if you want zones around new/delete
static constexpr const bool ZONES = false;

#if OVERRIDE_MALLOC && LIBSHIT_OS_IS_LINUX
#include <dlfcn.h>
#include <malloc.h>

static bool loaded = false, load_in_progress = false;
#define FUNS(x)                                                       \
  x(malloc) x(free) x(calloc) x(realloc) x(reallocarray)              \
  x(posix_memalign) x(aligned_alloc) x(valloc) x(memalign) x(pvalloc)
#define DECL(name) static decltype(&name) orig_##name;
FUNS(DECL);
#undef DECL

static bool LoadIfNeeded()
{
  if (loaded) return true;
  if (load_in_progress) return false;
  load_in_progress = true;
#define LOAD(name)                                                          \
  orig_##name = reinterpret_cast<decltype(&name)>(dlsym(RTLD_NEXT, #name)); \
  if (!orig_##name) std::abort();
  FUNS(LOAD);
#undef LOAD
  load_in_progress = false;
  loaded = true;
  return true;
}

#define NOT_ALLOCD_DEBUG 0

#if NOT_ALLOCD_DEBUG
static constexpr std::size_t MAX_ALLOCS = 1024 * 100;
pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;
static void* allocs[MAX_ALLOCS];
#endif

// On my machine at the time of writing this code, the required minimum value is
// 7, but let's be generous here in case tracy changes
static constexpr std::size_t IGNORED_SIZE = 32;
static void* ignored_allocs[IGNORED_SIZE];
static std::size_t ignored_i = 0;

static void MarkAlloc(void* ptr, std::size_t size)
{
#if NOT_ALLOCD_DEBUG
  pthread_mutex_lock(&alloc_mutex);
  for (std::size_t i = 0; i < MAX_ALLOCS; ++i)
    if (allocs[i] == ptr)
    {
      fprintf(stderr, "Allocing to the same ptr %p %zu\n", ptr, size);
      TracyFreeS(ptr, 5);
      allocs[i] = nullptr;
    }

  bool found = false;
  for (std::size_t i = 0; i < MAX_ALLOCS; ++i)
  {
    if (allocs[i] == nullptr)
    {
      allocs[i] = ptr;
      found = true;
      break;
    }
  }
  if (!found) abort();
  pthread_mutex_unlock(&alloc_mutex);
#endif
  TracyAllocS(ptr, size, 6);
}

static void MarkFree(void* ptr)
{
  if (!ptr) return;
#if NOT_ALLOCD_DEBUG
  pthread_mutex_lock(&alloc_mutex);
  for (std::size_t i = 0; i < MAX_ALLOCS; ++i)
  {
    if (allocs[i] == ptr)
    {
      allocs[i] = nullptr;
      pthread_mutex_unlock(&alloc_mutex);
      TracyFreeS(ptr, 6);
      return;
    }
  }
  pthread_mutex_unlock(&alloc_mutex);
#endif

  for (std::size_t i = 0; i < ignored_i; ++i)
    if (ignored_allocs[i] == ptr)
    {
      fprintf(stderr, "ignored size %zu\n", ignored_i);
      ignored_allocs[i] = nullptr;
      return;
    }

#if NOT_ALLOCD_DEBUG
  fprintf(stderr, "Freeing not allocd memory %d %p\n", ignored, ptr);
#else
  TracyFreeS(ptr, 6);
#endif
}

static thread_local bool inside_malloc = false;

__attribute__((visibility("default")))
void free(void* ptr)
{
  if (!LoadIfNeeded())
  {
    if (ptr) std::abort();
    return;
  }

  MarkFree(ptr);
  ZoneNamedNC(x, "free", 0x003300, ZONES);
  orig_free(ptr);
}

#define UNPAREN(...) __VA_ARGS__
#define ALLOC_LIKE(name, params, call, size_param)                       \
  __attribute__((visibility("default")))                                 \
  void* name(UNPAREN params)                                             \
  {                                                                      \
    if (!LoadIfNeeded()) return nullptr;                                 \
    void* ptr;                                                           \
    if (inside_malloc)                                                   \
    {                                                                    \
      /* The first TracyAlloc call will initialize tracy, which will     \
       * will call malloc again. If we call TracyAlloc here too, tracy   \
       * just deadlocks, so just ignore these few allocations... */      \
      ptr = orig_##name call;                                            \
      if (ignored_i == IGNORED_SIZE) std::abort();                       \
      if (ptr) ignored_allocs[ignored_i++] = ptr;                        \
      return ptr;                                                        \
    }                                                                    \
                                                                         \
    inside_malloc = true;                                                \
    {                                                                    \
      ZoneNamedNC(x, #name, 0x330000, ZONES); ZoneValueV(x, size_param); \
      ptr = orig_##name call;                                            \
    }                                                                    \
    MarkAlloc(ptr, size_param);                                          \
    inside_malloc = false;                                               \
    return ptr;                                                          \
  }
ALLOC_LIKE(malloc, (size_t size), (size), size);
ALLOC_LIKE(calloc, (size_t nmemb, size_t size), (nmemb, size), nmemb * size);

#define REALLOC_LIKE(name, params, call, size_param)                     \
  __attribute__((visibility("default")))                                 \
  void* name(void* ptr, UNPAREN params)                                  \
  {                                                                      \
    if (!LoadIfNeeded()) return nullptr;                                 \
    auto old_size = malloc_usable_size(ptr);                             \
    /* To prevent race condition with another thread just allocing the   \
     * the ptr again, mark as freed here. In the unlikely situation of   \
     * running out memory, we re-mark as allocd below... */              \
    MarkFree(ptr);                                                       \
    void* res_ptr;                                                       \
    {                                                                    \
      ZoneNamedNC(x, #name, 0x330000, ZONES); ZoneValueV(x, size_param); \
      res_ptr = orig_##name(ptr, UNPAREN call);                          \
    }                                                                    \
    if (res_ptr) MarkAlloc(res_ptr, size_param);                         \
    else if (ptr && (size_param) != 0) MarkAlloc(ptr, old_size);         \
    return res_ptr;                                                      \
  }

REALLOC_LIKE(realloc, (size_t size), (size), size);
// todo: glibc's reallocarray just calls realloc inside
// REALLOC_LIKE(reallocarray, (size_t nmemb, size_t size), (nmemb, size),
//              nmemb * size);
#undef REALLOC_LIKE

__attribute__((visibility("default")))
int posix_memalign(void** ptr, size_t align, size_t size)
{
  if (!LoadIfNeeded()) return ENOMEM;
  {
    ZoneNamedNC(x, "posix_memalign", 0x330000, ZONES); ZoneValueV(x, size);
    if (int res = orig_posix_memalign(ptr, align, size)) return res;
  }
  MarkAlloc(*ptr, size);
  return 0;
}

ALLOC_LIKE(aligned_alloc, (size_t align, size_t size), (align, size), size);
ALLOC_LIKE(valloc, (size_t size), (size), size);
ALLOC_LIKE(memalign, (size_t align, size_t size), (align, size), size);
ALLOC_LIKE(pvalloc, (size_t size), (size), size);
#undef ALLOC_LIKE

#else

void* operator new(std::size_t count)
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    ptr = std::malloc(count);
  }
  if (!ptr) throw std::bad_alloc{};

  TracyAllocS(ptr, count, 5);
  return ptr;
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    ptr = std::malloc(count);
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}


void* operator new(std::size_t count, std::align_val_t al)
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    if (posix_memalign(&ptr, static_cast<std::size_t>(al), count))
      throw std::bad_alloc{};
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}

void* operator new(std::size_t count, std::align_val_t al,
                   const std::nothrow_t&) noexcept
{
  void* ptr;
  {
    ZoneNamedNC(x, "new", 0x330000, ZONES); ZoneValueV(x, count);
    if (posix_memalign(&ptr, static_cast<std::size_t>(al), count))
      return nullptr;
  }
  TracyAllocS(ptr, count, 5);
  return ptr;
}


void operator delete(void* ptr) noexcept
{
  TracyFreeS(ptr, 5);
  ZoneNamedNC(x, "delete", 0x003300, ZONES);
  std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t al) noexcept
{
  TracyFreeS(ptr, 5);
  ZoneNamedNC(x, "delete", 0x003300, ZONES);
  std::free(ptr);
}
#endif
#endif
