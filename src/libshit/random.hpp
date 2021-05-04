#ifndef GUARD_UNMALLEABLY_DEFLECTIVE_SMOKED_MEAT_UNDERPOTS_9143
#define GUARD_UNMALLEABLY_DEFLECTIVE_SMOKED_MEAT_UNDERPOTS_9143
#pragma once

#include "libshit/assert.hpp"
#include "libshit/span.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace Libshit
{
  void FillRandom(Span<std::byte> buf);

  template <typename T, size_t s>
  static std::array<T, s> FillRandom()
  {
    std::array<T, s> ret;
    FillRandom({
      reinterpret_cast<std::byte*>(ret.data()), sizeof(T) * ret.size()});
    return ret;
  }

  /// Like std::make_unsigned but works on bool (returns bool)
  template <typename T> struct MakeUnsignedBool : std::make_unsigned<T> {};
  template <> struct MakeUnsignedBool<bool> { using type = bool; };
  template <typename T>
  using MakeUnsignedBoolT = typename MakeUnsignedBool<T>::type;

  /// CHAR_BIT * sizeof(T), except 1 for bool
  template <typename T> struct Bits
    : std::integral_constant<std::size_t, CHAR_BIT * sizeof(T)> {};
  template<> struct Bits<bool> : std::integral_constant<std::size_t, 1> {};

  template <typename T, typename Derived>
  struct RandomGeneratorBase
  {
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_unsigned_v<T>);
    using NativeType = T;

    // Same as Gen<U, 1 << Bits<U>::value>, except handles overflow in <<.
    template <typename U>
    constexpr std::enable_if_t<std::is_integral_v<U>, U> Gen() noexcept
    {
      static_assert(Bits<U>::value <= Bits<T>::value);
      T t = static_cast<Derived*>(this)->GenBlock();
      return t >> (Bits<T>::value - Bits<U>::value);
    }

    /**
     * Generate bool or integer in range [0, mod).
     * @tparam U integral type
     * @tparam mod Return a number modulo mod.
     */
    template <typename U, MakeUnsignedBoolT<U> mod>
    constexpr std::enable_if_t<std::is_integral_v<U>, U> Gen() noexcept
    {
      static_assert(mod >= 2);
      static_assert(mod <= std::numeric_limits<T>::max());

      constexpr T div = (std::numeric_limits<T>::max() - (mod-1)) / mod + 1;
      T g;
      do
        g = static_cast<Derived*>(this)->GenBlock();
      while (g > div * mod - 1);

      return g / div;
    }

    /**
     * Generate a random enum (class) value. It expects an enum starting from 0
     * without holes.
     * @tparam mod Number of elements of the enum (probably U::NumElements or
     *   something similar).
     */
    template <typename U, U mod>
    constexpr std::enable_if_t<std::is_enum_v<U>, U> Gen() noexcept
    {
      using Ut = std::underlying_type_t<U>;
      return static_cast<U>(Gen<Ut, static_cast<Ut>(mod)>());
    }

    /// Generate a floating point in range [0, mul).
    template <typename U>
    constexpr std::enable_if_t<std::is_floating_point_v<U>, U>
      Gen(U mul = 1) noexcept
    {
      static_assert(sizeof(U) <= sizeof(T));
      T g = static_cast<Derived*>(this)->GenBlock();
      constexpr auto dig = std::numeric_limits<U>::digits;
      return (g >> (sizeof(T)*CHAR_BIT - dig)) * (mul / (T(1)<<dig));
    }
  };

  /**
   * A ref-like wrapper to turn our own randoms into UniformRandomBitGenerator
   * used by stl (like std::shuffle, because they had to remove random_shuffle
   * taking a single function instead of this complicated mess).
   */
  template <typename Rand>
  class StdRandomRef
  {
  public:
    using result_type = typename Rand::NativeType;

    constexpr StdRandomRef(Rand& rnd) noexcept : rnd{&rnd} {}

    static constexpr result_type min() noexcept
    { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() noexcept
    { return std::numeric_limits<result_type>::max(); }

    result_type operator()() noexcept { return rnd->GenBlock(); }

  private:
    Rand* rnd;
  };

  /**
   * Random generator based on `xoshiro128+` implementation at
   * <http://xoshiro.di.unimi.it/xoshiro128plus.c>.
   *
   * @par Original license
   * Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
   * To the extent possible under law, the author has dedicated all copyright
   * and related and neighboring rights to this software to the public domain
   * worldwide. This software is distributed without any warranty.
   * See <http://creativecommons.org/publicdomain/zero/1.0/>.
  */
  class Xoshiro128p : public RandomGeneratorBase<std::uint32_t, Xoshiro128p>
  {
  public:
    Xoshiro128p() noexcept : Xoshiro128p{FillRandom<std::uint32_t, 4>()} {}

    /// The state must be seeded so that it is not everywhere zero.
    constexpr Xoshiro128p(const std::array<std::uint32_t, 4>& state) noexcept
      : state{state}
    {
      LIBSHIT_RASSERT(std::all_of(state.begin(), state.end(),
                                  [](auto x) { return x > 0; }));
    }

    Xoshiro128p(const Xoshiro128p&) = delete;
    void operator=(const Xoshiro128p&) = delete;

    constexpr std::uint32_t GenBlock() noexcept
    {
      const std::uint32_t result_plus = state[0] + state[3];

      const std::uint32_t t = state[1] << 9;

      state[2] ^= state[0];
      state[3] ^= state[1];
      state[1] ^= state[2];
      state[0] ^= state[3];

      state[2] ^= t;

      state[3] = rotl(state[3], 11);

      return result_plus;
    }

  private:
    static constexpr inline std::uint32_t rotl(std::uint32_t x, int k) noexcept
    { return (x << k) | (x >> (32 - k)); }

    std::array<std::uint32_t, 4> state;
  };

}

#endif
