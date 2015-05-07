/*
  Copyright (C) 2015 Marcus Bannerman <m.bannerman@gmail.com>
                2015 Severin Strobl <severin.strobl@fau.de>

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

// C++
#include <type_traits>

namespace stator {

  /*! \brief Physical constants in different floating point representations.
    
    This overload is for floating point types. The physical constants
    are taken from the CODATA 2010 recommended values. These are
    listed here: http://physics.nist.gov/cuu/index.html
    
    \tparam Scalar The representation to return the physical constants
    in.
  */
  template<typename Scalar,
           typename = typename std::is_floating_point<Scalar>::type>
  struct constant {

    /*! \brief Boltzmann's constant \f$k_B\f$. */
    static inline Scalar k() {
      return Scalar(1.3806488e-23);
    }
    
    /*! \brief \f$\pi\f$. */
    static inline Scalar pi() {
      return Scalar(3.1415926535897932384626433832795029L);                    
    }

    /*! \brief Avogadro's constant \f$N_A\f$. */
    static inline Scalar avogadro() {
      return Scalar(6.02214129e23);
    }

    /*! \brief The universal gas constant \f$R\f$. */
    static inline Scalar R() {
      return avogadro() * k();
    }

    /*! \brief The base of the natural logarithm \f$e\f$. */
    static inline Scalar e() {
      return Scalar(2.7182818284590452353602874713526624L);
    }
  };

  /*! \brief Physical constants approximated in std::ratio
      representation.
    
    C++11 requires implementations to support at least the long long
    type (64bits). std::ratio always uses the maximum size integer to
    represent its numerator/denominator, therefore std::ratio may use
    any natural number in \f$[-2^{63},+2^{63}-1]\f$ for the numerator
    and/or denominator. However, when calculations are performed using
    std::ratio, sufficient range must be reserved for the calculation.

    Because of these reasons, we choose rational approximations close
    to the limits of double precision, or slightly less accurate
    approximations where increased precision would require a dramatic
    increase in the size of the numerator/denominator.
    
  */
  struct constant_ratio {
    /*! \brief Rational approximation of pi with an error
        \f$<1.60e-14\f$. */
    typedef std::ratio<47627751, 15160384> pi; 

    /*! \brief approximation of e with an error < 8.89e-16. */
    typedef std::ratio<28245729, 10391023> e; 
  };
} // namespace stator
