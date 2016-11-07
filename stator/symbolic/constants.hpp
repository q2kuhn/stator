/*
  Copyright (C) 2015 Marcus N Campbell Bannerman <m.bannerman@gmail.com>

  This file is part of stator.

  stator is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  stator is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with stator. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <ratio>

namespace stator {
  namespace symbolic {    
    /*! \brief A class representing a compile-time constant.

      Fundamentally this is a std::ratio which has been extended to
      support standard "run-time" ops, such as multiplication.
    */
    template<std::intmax_t Num, std::intmax_t Denom = 1>
    struct C: std::ratio<Num, Denom> {
      operator double() const {
	return double(Num) / double(Denom);
      }
    };

    /*! \brief Conversion operator from std::ratio to C.*/
    template<class stdratio>
    struct C_wrap {
      typedef C<stdratio::num, stdratio::den> type;
    };
    
    /*! \brief Compile time type-test for compile-time constants \ref C.*/
    template<class T> struct is_C { static const bool value = false; };

    template<std::intmax_t N, std::intmax_t D> struct is_C<C<N,D> > { static const bool value = true; };

    /*! \brief A symbolic representation of zero. */
    typedef C<0> Null;
    /*! \brief A symbolic representation of one. */
    typedef C<1> Unity;
    
    /*! \brief A symbolic/compile-time rational approximation of \f$\pi\f$. */
    typedef C_wrap<constant_ratio::pi>::type pi;

    /*! \brief A symbolic/compile-time rational approximation of \f$\mathrm{e}\f$. */
    typedef C_wrap<constant_ratio::e>::type e;

    /*! \brief Output operator for compile-time constant (\ref C types).*/
    template<std::intmax_t Num, std::intmax_t Denom>
    inline std::ostream& operator<<(std::ostream& os, const C<Num, Denom>) {
      os << "C<" << Num;
      if (Denom != 1)
	os << ", " << Denom;
      return os << ">()";
    }
    
    /*! \brief Specialized output operator for \f$\pi\f$.*/
    inline std::ostream& operator<<(std::ostream& os, const pi) { os << "π"; return os; }
    /*! \brief Specialized output operator for \f$\mathrm{e}\f$.*/
    inline std::ostream& operator<<(std::ostream& os, const e) { os << "e"; return os; }

    /*! \brief Symbolic Factorial function.
     
      This template implementation returns Unity for 0! and 1!,
      allowing simplification of symbolic expressions.
    */
    template<size_t i> struct Factorial {
      typedef C<i * Factorial<i - 1>::value::num, 1> value;
    };
    
    template<> struct Factorial<1> {
      typedef C<1, 1> value;
    };

    template<> struct Factorial<0> {
      typedef C<1, 1> value;
    };

    template<std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2>
    constexpr auto operator+(C<n1, d1>, C<n2, d2>) -> STATOR_AUTORETURN((typename C_wrap<std::ratio_add<std::ratio<n1,d1>, std::ratio<n2,d2> > >::type()));

    template<std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2>
    constexpr auto operator-(C<n1, d1>, C<n2, d2>) -> STATOR_AUTORETURN((typename C_wrap<std::ratio_subtract<std::ratio<n1,d1>, std::ratio<n2,d2> > >::type()));

    template<std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2>
    constexpr auto operator*(C<n1, d1>, C<n2, d2>) -> STATOR_AUTORETURN((typename C_wrap<std::ratio_multiply<std::ratio<n1,d1>, std::ratio<n2,d2> > >::type()));

    template<std::intmax_t n1, std::intmax_t d1, std::intmax_t n2, std::intmax_t d2>
    constexpr auto operator/(C<n1, d1>, C<n2, d2>) -> STATOR_AUTORETURN((typename C_wrap<std::ratio_divide<std::ratio<n1,d1>, std::ratio<n2,d2> > >::type()));

    template<std::intmax_t Num1, std::intmax_t Denom1, std::intmax_t Num2, std::intmax_t Denom2>
    constexpr bool operator==(const C<Num1, Denom1>&, const C<Num2, Denom2>&)
    { return std::ratio_equal<std::ratio<Num1, Denom1>, std::ratio<Num2, Denom2> >::value; }
    
    /*! \brief Symbolic Inverse factorial function.
     
      This template implementation returns Unity for 1/0! and 1/1!,
      allowing simplification of symbolic expressions.
    */
    template<size_t i> struct InvFactorial {
      typedef C<Factorial<i>::value::den, Factorial<i>::value::num> value;
    };
    
    template<class T>
    inline std::ostream& operator<<(std::ostream& os, const std::complex<T>& c) {
      if (c.imag() == 0)
	return (os << c.real());
      if (c.imag() < 0)
	return (os << "(" <<c.real() << " - " << std::abs(c.imag()) << "i)");
      return (os << "(" <<c.real() << " + " << c.imag() << "i)");
    }
  }
}
