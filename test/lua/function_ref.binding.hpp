// Auto generated code, do not edit. See gen_binding in project root.
#ifndef LIBSHIT_WITHOUT_LUA
#include <libshit/lua/user_type.hpp>


const char ::Libshit::Lua::Test::FunctionRefTest::TYPE_NAME[] = "libshit.lua.test.function_ref_test";

namespace Libshit::Lua
{

  // class libshit.lua.test.function_ref_test
  template<>
  void TypeRegisterTraits<::Libshit::Lua::Test::FunctionRefTest>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      static_cast<void (::Libshit::Lua::Test::FunctionRefTest::*)(::Libshit::Lua::FunctionWrapGen<int>)>(&::Libshit::Lua::Test::FunctionRefTest::Cb<::Libshit::Lua::FunctionWrapGen<int>>)
    >("cb");
    bld.AddFunction<
      static_cast<void (::Libshit::Lua::Test::FunctionRefTest::*)(::Libshit::Lua::FunctionWrap<double(double)>)>(&::Libshit::Lua::Test::FunctionRefTest::Cb2<::Libshit::Lua::FunctionWrap<double(double)>>)
    >("cb2");
    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::FunctionRefTest, int, &::Libshit::Lua::Test::FunctionRefTest::x>
    >("get_x");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::FunctionRefTest, int, &::Libshit::Lua::Test::FunctionRefTest::x>
    >("set_x");
    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Lua::Test::FunctionRefTest, double, &::Libshit::Lua::Test::FunctionRefTest::y>
    >("get_y");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Lua::Test::FunctionRefTest, double, &::Libshit::Lua::Test::FunctionRefTest::y>
    >("set_y");

  }
  static TypeRegister::StateRegister<::Libshit::Lua::Test::FunctionRefTest> reg_libshit_lua_test_function_ref_test;

}
#endif
