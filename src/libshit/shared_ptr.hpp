#ifndef GUARD_TO_WIT_GROPEY_PINCER_PHONES_IN_7930
#define GUARD_TO_WIT_GROPEY_PINCER_PHONES_IN_7930
#pragma once

#include "libshit/assert.hpp"
#include "libshit/ref_counted.hpp" // IWYU pragma: export
#include "libshit/except.hpp"
#include "libshit/not_null.hpp" // IWYU pragma: export

#include <cstddef>
#include <memory>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Libshit
{

  namespace Detail
  {
    template <typename T>
    struct RefCountedPtrContainer : RefCounted
    {
      T* ptr;

      RefCountedPtrContainer(T* ptr) noexcept : ptr{ptr} {}
      void Dispose() noexcept override { static_assert(sizeof(T)); delete ptr; }
    };
  }

  template <typename T>
  class RefCountedContainer : public RefCounted
  {
  public:
    template <typename... Args>
    RefCountedContainer(Args&&... args)
    { new (t) T(std::forward<Args>(args)...); }

    T* Get() noexcept { return reinterpret_cast<T*>(t); }
    void Dispose() noexcept override { Get()->~T(); }

  private:
    alignas(T) char t[sizeof(T)];
  };

  namespace Detail
  {
    template <typename T> struct IsRefCountedContainer : std::false_type
    { using Type = T; };
    template <typename T>
    struct IsRefCountedContainer<RefCountedContainer<T>> : std::true_type
    { using Type = T; };
    template <typename T>
    struct IsRefCountedContainer<const RefCountedContainer<T>> : std::true_type
    { using Type = const T; };
  }

  // RefCounted objects only need one pointer, others need two
  template <typename T>
  struct SharedPtrStorageNormal
  {
    SharedPtrStorageNormal() noexcept = default;
    SharedPtrStorageNormal(RefCounted* ctrl, T* ptr) noexcept
      : ctrl{ctrl}, ptr{ptr} {}

    RefCounted* GetCtrl() const noexcept { return ctrl; }
    T* GetPtr() const noexcept { return ptr; }
    T* GetRetPtr() const noexcept { return ptr; }
    void Reset() noexcept { ctrl = nullptr; ptr = nullptr; }
    void Swap(SharedPtrStorageNormal& o) noexcept
    {
      std::swap(ctrl, o.ctrl);
      std::swap(ptr, o.ptr);
    }

    RefCounted* ctrl = nullptr;
    T* ptr = nullptr;
  };

  template <typename T>
  struct SharedPtrStorageRefCounted
  {
    using U = std::remove_const_t<T>;
    using RetT = typename Detail::IsRefCountedContainer<T>::Type;

    // can't put the IS_REFCOUNTED static assert into the class body, as that
    // would break when using this on incomplete types
    SharedPtrStorageRefCounted() noexcept
    { static_assert(std::is_same_v<T, void> || IS_REFCOUNTED<T>); }
    SharedPtrStorageRefCounted(RefCounted* ctrl, T* ptr) noexcept
      : ptr{const_cast<U*>(ptr)}
    {
      (void) ctrl;
      static_assert(std::is_same_v<T, void> || IS_REFCOUNTED<T>);
      // ptr == nullptr && ctrl != nullptr: valid in DynamicPointerCast
      LIBSHIT_ASSERT(!ptr || ctrl == ptr);
    }

    RefCounted* GetCtrl() const noexcept
    { return static_cast<RefCounted*>(ptr); } // static_cast needed for void
    T* GetPtr() const noexcept { return ptr; }
    RetT* GetRetPtr() const noexcept
    {
      if constexpr (Detail::IsRefCountedContainer<T>::value) return ptr->Get();
      return ptr;
    }
    void Reset() noexcept { ptr = nullptr; }
    void Swap(SharedPtrStorageRefCounted& o) noexcept { std::swap(ptr, o.ptr); }

    U* ptr = nullptr;
  };

  template <typename T, template<typename> class Storage> class WeakPtrBase;

  // shared (strong) ptr
  template <typename T, template<typename> class Storage>
  class SharedPtrBase : private Storage<T>
  {
    template <typename U = T>
    static constexpr bool IS_NORMAL_S = std::is_same_v<
      Storage<T>, SharedPtrStorageNormal<U>>;
    template <typename U = T>
    static constexpr bool IS_REFCOUNTED_S = std::is_same_v<
      Storage<T>, SharedPtrStorageRefCounted<U>>;
    static_assert(IS_NORMAL_S<> || IS_REFCOUNTED_S<>);

  public:
    using element_type = typename Detail::IsRefCountedContainer<T>::Type;

    // delay ctor instantiation, so SharedPtrStorageRefCounted won't fail on
    // incomplete types when instantiating this class
    template <typename U = T> SharedPtrBase() noexcept {}
    template <typename U = T> SharedPtrBase(std::nullptr_t) noexcept {}

    // alias ctor
    template <typename U, template<typename> typename UStorage, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    SharedPtrBase(SharedPtrBase<U, UStorage> o, T* ptr) noexcept
      : Storage<T>{o.GetCtrl(), ptr}
    { o.UStorage<U>::Reset(); }

    // weak->strong, throws on error
    template <typename U>
    SharedPtrBase(const WeakPtrBase<U, Storage>& o);

    // copy/move/conv ctor
    SharedPtrBase(const SharedPtrBase& o) noexcept
      : SharedPtrBase{o.GetCtrl(), o.GetPtr(), true} {}
    template <typename U>
    SharedPtrBase(const SharedPtrBase<U, Storage>& o) noexcept
      : SharedPtrBase{o.GetCtrl(), o.GetPtr(), true} {}
    // refcounted->refcounted/shared ok
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    SharedPtrBase(const SharedPtrBase<U, SharedPtrStorageRefCounted>& o) noexcept
      : SharedPtrBase{o.GetCtrl(), o.GetPtr(), true} {}

    SharedPtrBase(SharedPtrBase&& o) noexcept : Storage<T>{o}
    { o.Storage<T>::Reset(); }
    template <typename U>
    SharedPtrBase(SharedPtrBase<U, Storage>&& o) noexcept
      : Storage<T>{o.GetCtrl(), o.GetPtr()}
    { o.Storage<U>::Reset(); }
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    SharedPtrBase(SharedPtrBase<U, SharedPtrStorageRefCounted>&& o) noexcept
      : Storage<T>{o.GetCtrl(), o.GetPtr()}
    { o.SharedPtrStorageRefCounted<U>::Reset(); }


    // raw ptr for RefCounted objects
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_REFCOUNTED_S<V>>>
    SharedPtrBase(U* ptr, bool add_ref = true) noexcept
      : SharedPtrBase{const_cast<std::remove_const_t<U>*>(ptr), ptr, add_ref} {}

    // manage existing non refcounted ptrs
    template <typename U,
              typename = std::enable_if_t<IS_NORMAL_S<T> && IS_NORMAL_S<U>>>
    explicit SharedPtrBase(U* ptr)
      try : SharedPtrBase{new Detail::RefCountedPtrContainer<U>{ptr}, ptr, false}
    {} catch (...) { delete ptr; }

    // misc standard members
    SharedPtrBase& operator=(SharedPtrBase p) noexcept
    {
      Storage<T>::Swap(p);
      return *this;
    }
    ~SharedPtrBase() noexcept
    { if (GetCtrl()) GetCtrl()->RemoveRef(); }

    // shared_ptr members
    void reset() noexcept { SharedPtrBase{}.Storage<T>::Swap(*this); }
    template <typename U> void reset(U* ptr)
    { SharedPtrBase{ptr}.Storage<T>::Swap(*this); }

    void swap(SharedPtrBase& o) noexcept { Storage<T>::Swap(o); }

    element_type* get() const noexcept { return GetRetPtr(); }
    template <typename U = element_type,
              typename = std::enable_if_t<!std::is_same_v<U, void>>>
    U& operator*() const noexcept { return *GetRetPtr(); }
    element_type* operator->() const noexcept { return GetRetPtr(); }
    unsigned use_count() const noexcept
    { return GetCtrl() ? GetCtrl()->use_count() : 0; }
    bool unique() const noexcept { return use_count() == 1; }
    explicit operator bool() const noexcept { return GetPtr(); }

    // casts
#define LIBSHIT_GEN(camel, snake)                                 \
    template <typename U>                                         \
    SharedPtrBase<U, Storage> camel##PointerCast() const noexcept \
    { return {GetCtrl(), snake##_cast<U*>(get()), true}; }
    LIBSHIT_GEN(Static, static)
    LIBSHIT_GEN(Dynamic, dynamic)
    LIBSHIT_GEN(Const, const)
#undef LIBSHIT_GEN

    // low level stuff
    SharedPtrBase(RefCounted* ctrl, T* ptr, bool incr) noexcept
    : Storage<T>{ctrl, ptr}
    { if (incr && ctrl) ctrl->AddRef(); }

    using Storage<T>::GetCtrl;
    using Storage<T>::GetPtr;
    using Storage<T>::GetRetPtr;

  private:
    template <typename U, template<typename> class UStorage>
    friend class SharedPtrBase;
  };

  template <typename T, template<typename> class Storage>
  inline void swap(
    SharedPtrBase<T, Storage>& a, SharedPtrBase<T, Storage>& b) noexcept
  { a.swap(b); }

  // comparison operators
#define LIBSHIT_GEN(type, get, op)                               \
  template <typename T, template<typename> class TStorage,       \
            typename U, template<typename> class UStorage>       \
  inline bool operator op(                                       \
    const type##PtrBase<T, TStorage>& a,                         \
    const type##PtrBase<U, UStorage>& b) noexcept                \
  { return a.get() op b.get(); }                                 \
  template <typename T, template<typename> class Storage>        \
  inline bool operator op(                                       \
    const type##PtrBase<T, Storage>& p, std::nullptr_t) noexcept \
  { return p.get() op nullptr; }                                 \
  template <typename T, template<typename> class Storage>        \
  inline bool operator op(                                       \
    std::nullptr_t, const type##PtrBase<T, Storage>& p) noexcept \
  { return nullptr op p.get(); }
#define LIBSHIT_GEN2(type, get)                         \
  LIBSHIT_GEN(type, get, ==) LIBSHIT_GEN(type, get, !=) \
  LIBSHIT_GEN(type, get, <)  LIBSHIT_GEN(type, get, <=) \
  LIBSHIT_GEN(type, get, >)  LIBSHIT_GEN(type, get, >=)
  LIBSHIT_GEN2(Shared, GetRetPtr)

  // use these types. usually SmartPtr; use SharedPtr when you need aliasing
  // with an otherwise RefCounted type
  template <typename T>
  using SharedPtr = SharedPtrBase<T, SharedPtrStorageNormal>;
  template <typename T>
  using RefCountedPtr = SharedPtrBase<T, SharedPtrStorageRefCounted>;
  template <typename T>
  using SmartPtr = std::conditional_t<
    IS_REFCOUNTED<T>,
    SharedPtrBase<T, SharedPtrStorageRefCounted>,
    SharedPtrBase<T, SharedPtrStorageNormal>>;
  // mix between RefCountedPtr and SharedPtr: size is one pointer like with
  // RefCounted, T doesn't need to inherit from RefCounted like SharedPtr, but
  // no aliasing, casting, or random pointer to *Ptr conversion
  template <typename T>
  using WrappedPtr = RefCountedPtr<RefCountedContainer<T>>;

  template <typename T>
  using NotNullSharedPtr = NotNull<SharedPtr<T>>;
  template <typename T>
  using NotNullRefCountedPtr = NotNull<RefCountedPtr<T>>;
  template <typename T>
  using NotNullSmartPtr = NotNull<SmartPtr<T>>;
  template <typename T>
  using NotNullWrappedPtr = NotNull<WrappedPtr<T>>;

  template <typename T>
  RefCountedPtr<T> ToRefCountedPtr(T* t) noexcept { return {t}; }
  template <typename T>
  NotNullRefCountedPtr<T> ToNotNullRefCountedPtr(T* t) noexcept
  { return NotNullRefCountedPtr<T>{t}; }

  // weak ptr
  template <typename T, template<typename> class Storage>
  class WeakPtrBase : private Storage<T>
  {
    template <typename U = T>
    static constexpr bool IS_NORMAL_S = std::is_same_v<
      Storage<T>, SharedPtrStorageNormal<U>>;
    template <typename U = T>
    static constexpr bool IS_REFCOUNTED_S = std::is_same_v<
      Storage<T>, SharedPtrStorageRefCounted<U>>;
    static_assert(IS_NORMAL_S<> || IS_REFCOUNTED_S<>);

  public:
    using element_type = typename Detail::IsRefCountedContainer<T>::Type;

    WeakPtrBase() = default;
    WeakPtrBase(std::nullptr_t) noexcept {}

    template <typename U>
    WeakPtrBase(const SharedPtrBase<U, Storage>& o) noexcept
      : WeakPtrBase{o.GetCtrl(), o.get(), true} {}
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    WeakPtrBase(const SharedPtrBase<U, SharedPtrStorageRefCounted>& o) noexcept
      : WeakPtrBase{o.GetCtrl(), o.get(), true} {}

    // copy/move/conv ctor
    WeakPtrBase(const WeakPtrBase& o) noexcept
      : WeakPtrBase{o.GetCtrl(), o.GetPtr(), true} {}
    template <typename U>
    WeakPtrBase(const WeakPtrBase<U, Storage>& o) noexcept
      : WeakPtrBase{o.GetCtrl(), o.GetPtr(), true} {}
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    WeakPtrBase(const WeakPtrBase<U, SharedPtrStorageRefCounted>& o) noexcept
      : WeakPtrBase{o.GetCtrl(), o.GetPtr(), true} {}

    WeakPtrBase(WeakPtrBase&& o) noexcept : Storage<T>{o}
    { o.Storage<T>::Reset(); }
    template <typename U>
    WeakPtrBase(WeakPtrBase<U, Storage>&& o) noexcept
      : Storage<T>{o.GetCtrl(), o.GetPtr()}
    { o.Storage<U>::Reset(); }
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_NORMAL_S<V>>>
    WeakPtrBase(WeakPtrBase<U, SharedPtrStorageRefCounted>&& o) noexcept
      : Storage<T>{o.GetCtrl(), o.GetPtr()}
    { o.SharedPtrStorageRefCounted<U>::Reset(); }

    // raw ptr for RefCounted objects
    // assume there's a strong reference to it...
    template <typename U, typename V = T,
              typename = std::enable_if_t<IS_REFCOUNTED_S<V>>>
    WeakPtrBase(U* ptr, bool add_ref = true) noexcept
      : WeakPtrBase{const_cast<std::remove_const_t<U>*>(ptr), ptr, add_ref} {}

    // misc standard members
    WeakPtrBase& operator=(WeakPtrBase o) noexcept
    {
      Storage<T>::Swap(o);
      return *this;
    }
    ~WeakPtrBase() noexcept { if (GetCtrl()) GetCtrl()->RemoveWeakRef(); }

    // weak_ptr members
    void reset() noexcept { WeakPtrBase{}.Storage<T>::Swap(*this); }
    void swap(WeakPtrBase& o) noexcept { Storage<T>::Swap(o); }

    unsigned use_count() const noexcept
    { return GetCtrl() ? GetCtrl()->use_count() : 0; }
    bool expired() const noexcept { return use_count() == 0; }

    SharedPtrBase<T, Storage> lock() const noexcept
    {
      auto ctrl = GetCtrl();
      if (ctrl && ctrl->LockWeak()) return {ctrl, GetPtr(), false};
      else return {};
    }

    // not in weak_ptr -- like the shared ctor, but you don't have to specify
    // the kilometer long typename
    SharedPtrBase<T, Storage> lock_throw() const
    {
      auto ctrl = GetCtrl();
      if (ctrl && ctrl->LockWeak()) return {ctrl, GetPtr(), false};
      else LIBSHIT_THROW(std::bad_weak_ptr, std::tuple<>{});
    }

    // not in weak_ptr
    SharedPtrBase<T, Storage> unsafe_lock() const noexcept
    {
      // not bulletproof, but should catch most problems
      LIBSHIT_ASSERT(!expired());
      return {GetCtrl(), GetPtr(), true};
    }

    element_type* unsafe_get() const noexcept
    {
      LIBSHIT_ASSERT(!expired());
      return GetRetPtr();
    }

    // low level stuff
    WeakPtrBase(RefCounted* ctrl, T* ptr, bool incr) noexcept
      : Storage<T>{ctrl, ptr}
    { if (incr && ctrl) ctrl->AddWeakRef(); }

    using Storage<T>::GetCtrl;
    using Storage<T>::GetPtr;
    using Storage<T>::GetRetPtr;

  private:
    template <typename U, template<typename> class UStorage>
    friend class WeakPtrBase;
  };

  LIBSHIT_GEN2(Weak, GetRetPtr)
#undef LIBSHIT_GEN2
#undef LIBSHIT_GEN

  template <typename T, template<typename> class Storage>
  inline void swap(
    WeakPtrBase<T, Storage>& a, WeakPtrBase<T, Storage>& b) noexcept
  { a.swap(b); }


  template <typename T>
  using WeakPtr = WeakPtrBase<T, SharedPtrStorageNormal>;
  template <typename T>
  using WeakRefCountedPtr = WeakPtrBase<T, SharedPtrStorageRefCounted>;
  template <typename T>
  using WeakSmartPtr = std::conditional_t<
    IS_REFCOUNTED<T>,
    WeakPtrBase<T, SharedPtrStorageRefCounted>,
    WeakPtrBase<T, SharedPtrStorageNormal>>;
  template <typename T>
  using WeakWrappedPtr = WeakRefCountedPtr<RefCountedContainer<T>>;

  template <typename T>
  WeakRefCountedPtr<T> ToWeakRefCountedPtr(T* t) noexcept
  { return {t}; }


  template <typename T, template<typename> class Storage>
  template <typename U>
  inline SharedPtrBase<T, Storage>::SharedPtrBase(
    const WeakPtrBase<U, Storage>& o)
    : SharedPtrBase{o.lock_throw()} {}

  // make_shared like helper
  template <typename T, typename Ret, typename = void>
  struct MakeSharedHelper
  {
    template <typename... Args>
    static NotNull<Ret> Make(Args&&... args)
    {
      auto a = new RefCountedContainer<T>{std::forward<Args>(args)...};
      return NotNull<Ret>{a, a->Get(), false};
    }
  };

  template <typename T>
  struct MakeSharedHelper<T, RefCountedPtr<T>>
  {
    template <typename... Args>
    static NotNull<RefCountedPtr<T>> Make(Args&&... args)
    {
      return NotNull<RefCountedPtr<T>>{
        new T(std::forward<Args>(args)...), false};
    }
  };


#define LIBSHIT_GEN(variant)                             \
  template <typename T, typename... Args>                \
  NotNull<variant##Ptr<T>> Make##variant(Args&&... args) \
  {                                                      \
    return MakeSharedHelper<T, variant##Ptr<T>>::Make(   \
      std::forward<Args>(args)...);                      \
  }
  LIBSHIT_GEN(Shared) LIBSHIT_GEN(RefCounted) LIBSHIT_GEN(Smart)
  LIBSHIT_GEN(Wrapped)
#undef LIBSHIT_GEN

}

namespace std
{
  template <typename T, template<typename> class Storage>
  struct hash<Libshit::SharedPtrBase<T, Storage>>
  {
    using Ptr = Libshit::SharedPtrBase<T, Storage>;
    std::size_t operator()(const Ptr& ptr) const noexcept
    { return std::hash<typename Ptr::element_type*>(ptr.get()); }
  };
}

#endif
