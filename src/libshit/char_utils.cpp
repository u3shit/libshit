#include <iomanip>
#include "char_utils.hpp"

namespace Libshit
{
  bool DumpByte(std::ostream& os, char c, bool prev_hex_escape)
  {
    switch (c)
    {
    case '"':  os << "\\\""; break;
    case '\\': os << "\\\\"; break;
    case '\a': os << "\\a";  break;
    case '\b': os << "\\b";  break;
    case '\f': os << "\\f";  break;
    case '\n': os << "\\n";  break;
    case '\r': os << "\\r";  break;
    case '\t': os << "\\t";  break;
    case '\v': os << "\\v";  break;

    default:
      if (Ascii::IsPrint(c) && (!prev_hex_escape || !Ascii::IsXDigit(c)))
        os << c;
      else
      {
        auto flags = os.flags();
        os << "\\x" << std::setw(2) << std::setfill('0') << std::hex <<
          unsigned(static_cast<unsigned char>(c));
        os.flags(flags);
        return true;
      }
    }
    return false;
  }

  void DumpBytes(std::ostream& os, Libshit::StringView data)
  {
    os << '"';
    bool hex = false;
    for (size_t i = 0; i < data.length(); ++i)
      hex = DumpByte(os, data[i], hex);
    os << '"';
  }
}
