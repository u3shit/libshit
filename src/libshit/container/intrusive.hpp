#ifndef GUARD_SUITABLY_PANDEROUS_ENFRINGEMENT_OUTMATCHES_9679
#define GUARD_SUITABLY_PANDEROUS_ENFRINGEMENT_OUTMATCHES_9679
#pragma once

#include "libshit/except.hpp"

#include <stdexcept>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/options.hpp>

namespace Libshit
{

#ifdef NDEBUG
  using LinkMode = boost::intrusive::link_mode<boost::intrusive::normal_link>;
#else
  using LinkMode = boost::intrusive::link_mode<boost::intrusive::safe_link>;
#endif

  LIBSHIT_GEN_EXCEPTION_TYPE(ContainerConsistency, std::logic_error);
  // trying to add an already linked item to an intrusive container
  LIBSHIT_GEN_EXCEPTION_TYPE(ItemAlreadyAdded, ContainerConsistency);
  // item not linked, linked to a different container, item is a root node, ...
  LIBSHIT_GEN_EXCEPTION_TYPE(ItemNotInContainer, ContainerConsistency);
  // invalid item (nullptr, ...)
  LIBSHIT_GEN_EXCEPTION_TYPE(InvalidItem, ContainerConsistency);

}

#endif
