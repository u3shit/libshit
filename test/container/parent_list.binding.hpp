// Auto generated code, do not edit. See gen_binding in project root.
#if LIBSHIT_WITH_LUA
#include <libshit/lua/user_type.hpp>


const char ::Libshit::Test::ParentListItem::TYPE_NAME[] = "libshit.test.parent_list_item";
template <>
const char ::parent_list_item::TYPE_NAME[] = "libshit.parent_list_parent_list_item";

namespace Libshit::Lua
{

  // class libshit.test.parent_list_item
  template<>
  void TypeRegisterTraits<::Libshit::Test::ParentListItem>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::Libshit::Test::ParentListItem>::Make<>,
      &::Libshit::Lua::TypeTraits<::Libshit::Test::ParentListItem>::Make<LuaGetRef<int>>
    >("new");
    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Test::ParentListItem, int, &::Libshit::Test::ParentListItem::i>
    >("get_i");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Test::ParentListItem, int, &::Libshit::Test::ParentListItem::i>
    >("set_i");
    bld.AddFunction<
      &::Libshit::Lua::GetMember<::Libshit::Test::ParentListItem, int, &::Libshit::Test::ParentListItem::data>
    >("get_data");
    bld.AddFunction<
      &::Libshit::Lua::SetMember<::Libshit::Test::ParentListItem, int, &::Libshit::Test::ParentListItem::data>
    >("set_data");

  }
  static TypeRegister::StateRegister<::Libshit::Test::ParentListItem> reg_libshit_test_parent_list_item;

  // class libshit.parent_list_parent_list_item
  template<>
  void TypeRegisterTraits<::parent_list_item>::Register(TypeBuilder& bld)
  {

    bld.AddFunction<
      &::Libshit::Lua::TypeTraits<::parent_list_item>::Make<>
    >("new");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item &) noexcept>(&::parent_list_item::swap)
    >("swap");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::reference)>(&::parent_list_item::push_back<Check::Throw>)
    >("push_back");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)()>(&::parent_list_item::pop_back<Check::Throw>)
    >("pop_back");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::reference)>(&::parent_list_item::push_front<Check::Throw>)
    >("push_front");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)()>(&::parent_list_item::pop_front<Check::Throw>)
    >("pop_front");
    bld.AddFunction<
      static_cast<::parent_list_item::reference (::parent_list_item::*)()>(&::parent_list_item::front<Check::Throw>)
    >("front");
    bld.AddFunction<
      static_cast<::parent_list_item::reference (::parent_list_item::*)()>(&::parent_list_item::back<Check::Throw>)
    >("back");
    bld.AddFunction<
      static_cast<::parent_list_item::size_type (::parent_list_item::*)() const noexcept>(&::parent_list_item::size)
    >("size");
    bld.AddFunction<
      static_cast<bool (::parent_list_item::*)() const noexcept>(&::parent_list_item::empty)
    >("empty");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::size_type) noexcept>(&::parent_list_item::shift_backwards)
    >("shift_backwards");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::size_type) noexcept>(&::parent_list_item::shift_forward)
    >("shift_forward");
    bld.AddFunction<
      static_cast<::parent_list_item::iterator (::parent_list_item::*)(::parent_list_item::const_iterator)>(&::parent_list_item::erase<Check::Throw>),
      static_cast<::parent_list_item::iterator (::parent_list_item::*)(::parent_list_item::const_iterator, ::parent_list_item::const_iterator)>(&::parent_list_item::erase<Check::Throw>)
    >("erase");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)() noexcept>(&::parent_list_item::clear)
    >("clear");
    bld.AddFunction<
      static_cast<::parent_list_item::iterator (::parent_list_item::*)(::parent_list_item::const_iterator, ::parent_list_item::reference)>(&::parent_list_item::insert<Check::Throw>)
    >("insert");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::const_iterator, ::parent_list_item &)>(&::parent_list_item::splice<Check::Throw>),
      static_cast<void (::parent_list_item::*)(::parent_list_item::const_iterator, ::parent_list_item &, ::parent_list_item::const_iterator)>(&::parent_list_item::splice<Check::Throw>),
      static_cast<void (::parent_list_item::*)(::parent_list_item::const_iterator, ::parent_list_item &, ::parent_list_item::const_iterator, ::parent_list_item::const_iterator)>(&::parent_list_item::splice<Check::Throw>)
    >("splice");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)()>(&::parent_list_item::sort),
      static_cast<void (::parent_list_item::*)(::Libshit::Lua::FunctionWrapGen<bool>)>(&::parent_list_item::sort<::Libshit::Lua::FunctionWrapGen<bool>>)
    >("sort");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item &)>(&::parent_list_item::merge<Check::Throw>),
      static_cast<void (::parent_list_item::*)(::parent_list_item &, ::Libshit::Lua::FunctionWrapGen<bool>)>(&::parent_list_item::merge<::Libshit::Check::Throw, ::Libshit::Lua::FunctionWrapGen<bool>>)
    >("merge");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)() noexcept>(&::parent_list_item::reverse)
    >("reverse");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::parent_list_item::const_reference)>(&::parent_list_item::remove)
    >("remove");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)(::Libshit::Lua::FunctionWrapGen<bool>)>(&::parent_list_item::remove_if<::Libshit::Lua::FunctionWrapGen<bool>>)
    >("remove_if");
    bld.AddFunction<
      static_cast<void (::parent_list_item::*)()>(&::parent_list_item::unique),
      static_cast<void (::parent_list_item::*)(::Libshit::Lua::FunctionWrapGen<bool>)>(&::parent_list_item::unique<::Libshit::Lua::FunctionWrapGen<bool>>)
    >("unique");
    bld.AddFunction<
      static_cast<::Libshit::Lua::RetNum (*)(::Libshit::Lua::StateRef, ::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits, ::Libshit::ParentListBaseHookTraits<::Libshit::Test::ParentListItem, ::Libshit::DefaultTag> >::FakeClass &, ::Libshit::Test::ParentListItem &)>(::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits>::Next)
    >("next");
    bld.AddFunction<
      static_cast<::Libshit::Lua::RetNum (*)(::Libshit::Lua::StateRef, ::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits, ::Libshit::ParentListBaseHookTraits<::Libshit::Test::ParentListItem, ::Libshit::DefaultTag> >::FakeClass &, ::Libshit::Test::ParentListItem &)>(::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits>::Prev)
    >("prev");
    bld.AddFunction<
      static_cast<::Libshit::Lua::RetNum (*)(::Libshit::Lua::StateRef, ::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits, ::Libshit::ParentListBaseHookTraits<::Libshit::Test::ParentListItem, ::Libshit::DefaultTag> >::FakeClass &)>(::Libshit::ParentListLua<::Libshit::Test::ParentListItem, ::Libshit::Test::ParentListItemTraits>::ToTable)
    >("to_table");

  }
  static TypeRegister::StateRegister<::parent_list_item> reg_libshit_parent_list_parent_list_item;

}
#endif
