// Auto generated code, do not edit. See gen_binding in project root.
#ifndef LIBSHIT_WITHOUT_LUA
#include <libshit/lua/user_type.hpp>


const char ::Libshit::Lua::Test::Smart::TYPE_NAME[] = "libshit.lua.test.smart";

const char ::Libshit::Lua::Test::Foo::TYPE_NAME[] = "libshit.lua.test.foo";

const char ::Libshit::Lua::Test::Bar::Baz::Asdfgh::TYPE_NAME[] = "libshit.lua.test.bar.baz.asdfgh";

const char ::Libshit::Lua::Test::Baz::TYPE_NAME[] = "libshit.lua.test.baz";

const char ::Libshit::Lua::Test::A::TYPE_NAME[] = "libshit.lua.test.a";

const char ::Libshit::Lua::Test::B::TYPE_NAME[] = "libshit.lua.test.b";

const char ::Libshit::Lua::Test::Multi::TYPE_NAME[] = "libshit.lua.test.multi";

namespace Libshit::Lua
{

  // class libshit.lua.test.smart
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::Smart>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::Smart, int, &::Libshit::Lua::Test::Smart::x>
    >("get_x");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::Smart, int, &::Libshit::Lua::Test::Smart::x>
    >("set_x");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::Smart> reg_libshit_lua_test_smart;

  // class libshit.lua.test.foo
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::Foo>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::Foo, int, &::Libshit::Lua::Test::Foo::local_var>
    >("get_local_var");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::Foo, int, &::Libshit::Lua::Test::Foo::local_var>
    >("set_local_var");
    bld.AddFunction<
      &::Libshit::Lua::GetRefCountedOwnedMember<::Libshit::Lua::Test::Foo, ::Libshit::Lua::Test::Smart, &::Libshit::Lua::Test::Foo::smart>
    >("get_smart");
    bld.AddFunction<
      static_cast<void (::Libshit::Lua::Test::Foo::*)(int)>(&::Libshit::Lua::Test::Foo::DoIt)
    >("do_it");
    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::Libshit::Lua::Test::Foo>::Make<>
    >("new");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::Foo> reg_libshit_lua_test_foo;

  // class libshit.lua.test.bar.baz.asdfgh
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::Bar::Baz::Asdfgh>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::Libshit::Lua::Test::Bar::Baz::Asdfgh>::Make<>
    >("new");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::Bar::Baz::Asdfgh> reg_libshit_lua_test_bar_baz_asdfgh;

  // class libshit.lua.test.baz
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::Baz>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::Libshit::Lua::Test::Baz>::Make<>
    >("new");
    bld.AddFunction<
      static_cast<void (::Libshit::Lua::Test::Baz::*)(int)>(&::Libshit::Lua::Test::Baz::SetGlobal)
    >("set_global");
    bld.AddFunction<
      static_cast<int (::Libshit::Lua::Test::Baz::*)()>(&::Libshit::Lua::Test::Baz::GetRandom)
    >("get_random");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::Baz> reg_libshit_lua_test_baz;

  // class libshit.lua.test.a
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::A>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::A, int, &::Libshit::Lua::Test::A::x>
    >("get_x");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::A, int, &::Libshit::Lua::Test::A::x>
    >("set_x");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::A> reg_libshit_lua_test_a;

  // class libshit.lua.test.b
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::B>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::B, int, &::Libshit::Lua::Test::B::y>
    >("get_y");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::B, int, &::Libshit::Lua::Test::B::y>
    >("set_y");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::B> reg_libshit_lua_test_b;

  // class libshit.lua.test.multi
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::Multi>::Register(TypeBuilder& bld)
  {
    bld.Inherit<::Libshit::Lua::Test::Multi, ::Libshit::Lua::Test::A, ::Libshit::Lua::Test::B>();

    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::Libshit::Lua::Test::Multi>::Make<>
    >("new");
    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::Multi, SharedPtr<::Libshit::Lua::Test::B>, &::Libshit::Lua::Test::Multi::ptr>
    >("get_ptr");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::Multi, SharedPtr<::Libshit::Lua::Test::B>, &::Libshit::Lua::Test::Multi::ptr>
    >("set_ptr");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::Multi> reg_libshit_lua_test_multi;

}
#endif
