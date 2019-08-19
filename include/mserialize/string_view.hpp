#ifndef MSERIALIZE_STRING_VIEW_H_
#define MSERIALIZE_STRING_VIEW_H_

#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

/*
 * Based on the impelmentation of Boost string_view,
 * created by Marshall Clow and Beman Dawes.
 * (http://www.boost.org/doc/libs/master/libs/utility/doc/html/string_ref.html)
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Rationale: exposing boost types on the interface can quickly
 * yield hard to resolve version conflicts in dependant projects.
 * To avoid such, this class is used instead of boost::string_view.
 * As C++17 becomes mandatory, std::string_view can be used instead.
 *
 * Additional advantage of this over boost is that it does not
 * include <algorithm>, which is huge.
 */

namespace mserialize {

class string_view
{
public:
  // types
  using value_type = char;
  using pointer = char*;
  using const_pointer = const char*;
  using reference = char&;
  using const_reference = const char&;
  using const_iterator = const_pointer;
  using iterator = const_iterator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  static constexpr size_type npos = size_type(-1);

  // constructors
  constexpr string_view() = default;

  constexpr string_view(const string_view&) = default;
  string_view& operator=(const string_view&) = default;

  constexpr string_view(string_view&&) = default;
  string_view& operator=(string_view&&) = default;

  /*explicit*/ string_view(const std::string& str) noexcept // NOLINT(google-explicit-constructor)
   :_ptr(str.data()), _len(str.size())
  {}

  /*explicit*/ string_view(const char* str) // NOLINT(google-explicit-constructor)
    :_ptr(str), _len(strlen(str))
  {}

  constexpr string_view(const char* str, size_type len)
    :_ptr(str), _len(len)
  {}

  // destructor to make the linter happy
  ~string_view() = default;

  // iterators
  constexpr const_iterator  begin() const noexcept { return _ptr; }
  constexpr const_iterator cbegin() const noexcept { return _ptr; }
  constexpr const_iterator    end() const noexcept { return _ptr + _len; }
  constexpr const_iterator   cend() const noexcept { return _ptr + _len; }

  // capacity
  constexpr size_type size() const noexcept { return _len; }
  constexpr bool empty()     const noexcept { return _len == 0; }

  // element access
  constexpr const_reference operator[](size_type pos) const noexcept { return _ptr[pos]; }

  constexpr const_reference front() const          { return _ptr[0]; }
  constexpr const_reference back()  const          { return _ptr[_len-1]; }
  constexpr const_pointer   data()  const noexcept { return _ptr; }

  // modifiers
  void clear() noexcept { _len = 0; }

  void remove_prefix(size_type n)
  {
    if (n > _len) { n = _len; }
    _ptr += n;
    _len -= n;
  }

  void remove_suffix(size_type n)
  {
    if (n > _len) { n = _len; }
    _len -= n;
  }

  void swap(string_view& s) noexcept
  {
    std::swap(_ptr, s._ptr);
    std::swap(_len, s._len);
  }

  // string operations
  std::string to_string() const
  {
    return std::string(begin(), end());
  }

  string_view substr(size_type pos, size_type n=npos) const
  {
    if (pos > size())
    {
      throw std::out_of_range("mserialize::string_view::substr");
    }

    if (n == npos || pos + n > size())
    {
      n = size() - pos;
    }

    return string_view(data() + pos, n);
  }

  // searches
  constexpr bool starts_with(char c) const noexcept
  {
    return !empty() && c == front();
  }

  constexpr bool starts_with(string_view x) const noexcept
  {
    return _len >= x._len && std::char_traits<char>::compare(_ptr, x._ptr, x._len) == 0;
  }

  constexpr bool ends_with(char c) const noexcept
  {
    return !empty() && c == back();
  }

  constexpr bool ends_with(string_view x) const noexcept
  {
    return _len >= x._len &&
      std::char_traits<char>::compare(_ptr + _len - x._len, x._ptr, x._len) == 0;
  }

  size_type find(string_view s, size_type pos = 0) const noexcept
  {
    if (pos > size()) { return npos; }
    if (s.empty()) { return pos; }
    const_iterator iter = search(cbegin() + pos, cend(), s.cbegin(), s.cend());
    return iter == cend() ? npos : size_type(iter - cbegin());
  }

  size_type find(char c, size_type pos = 0) const noexcept
  { return find(string_view(&c, 1), pos); }

  size_type find(const char* s, size_type pos, size_type n) const noexcept
  { return find(string_view(s, n), pos); }

  size_type find(const char* s, size_type pos = 0) const noexcept
  { return find(string_view(s), pos); }

private:
  static const_iterator
  search(const_iterator first, const_iterator last, const_iterator s_first, const_iterator s_last)
  {
    for (; ; ++first)
    {
      const_iterator it = first;
      for (const_iterator s_it = s_first; ; ++it, ++s_it)
      {
        if (s_it == s_last) { return first; }

        if (it == last) { return last; }

        if (*it != *s_it) { break; }
      }
    }
  }

  const char *_ptr = nullptr;
  std::size_t _len = 0;
};

inline bool operator==(string_view x, string_view y) noexcept
{
  if (x.size () != y.size ()) { return false; }
  return std::char_traits<char>::compare(x.data(), y.data(), x.size()) == 0;
}

inline bool operator!=(string_view x, string_view y) noexcept
{
  return !(x == y);
}

inline std::ostream& operator<<(std::ostream& out, string_view x)
{
  return out.write(x.data(), std::streamsize(x.size()));
}

} // namespace mserialize

#endif // MSERIALIZE_STRING_VIEW_H_
