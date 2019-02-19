#ifndef GUARD_SUITABLY_PANDEROUS_ENFRINGEMENT_OUTMATCHES_9679
#define GUARD_SUITABLY_PANDEROUS_ENFRINGEMENT_OUTMATCHES_9679
#pragma once

#include "libshit/except.hpp"
#include "libshit/platform.hpp"

#include <stdexcept>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/options.hpp>

namespace Libshit
{

  using LinkMode = boost::intrusive::link_mode<
    LIBSHIT_IS_DEBUG ? boost::intrusive::safe_link : boost::intrusive::normal_link>;

  LIBSHIT_GEN_EXCEPTION_TYPE(ContainerConsistency, std::logic_error);
  // trying to add an already linked item to an intrusive container
  LIBSHIT_GEN_EXCEPTION_TYPE(ItemAlreadyAdded, ContainerConsistency);
  // item not linked, linked to a different container, item is a root node, ...
  LIBSHIT_GEN_EXCEPTION_TYPE(ItemNotInContainer, ContainerConsistency);
  // invalid item (nullptr, ...)
  LIBSHIT_GEN_EXCEPTION_TYPE(InvalidItem, ContainerConsistency);

}

#endif
