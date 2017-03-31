/*! \file symbolic.hpp
  \brief Main header for the stator::symbolic library.
*/
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

// stator
#include "stator/config.hpp"
#include "stator/constants.hpp"
#include "stator/orphan/stack_vector.hpp"

// Eigen
#include "Eigen/Dense"

// C++
#include <complex>
#include <ratio>
#include <type_traits>

/*! \namespace sym
  \brief The stator symbolic math library.

  This namespace encapsulates the symbolic math functionality of
  stator. This library allows you to perform compile-time operations
  on mathematical expressions, as well as perform run-time
  calculations, such as root finding on polynomials.    
*/
namespace sym {
  using stator::orphan::StackVector;
  using stator::detail::store;
  
  namespace detail {
    using stator::detail::choice;
    using stator::detail::last_choice;
    using stator::detail::select_overload;
  } // namespace detail

  /*! \brief A type trait to denote symbolic terms (i.e., one that
      is not yet immediately evaluable to a "normal" type)*/
  struct SymbolicOperator {};
}

#include "stator/symbolic/constants.hpp"
#include "stator/orphan/template_config.hpp"

namespace sym {  
  template<class T>
  struct IsSymbolic {
    static constexpr bool value = std::is_base_of<SymbolicOperator, T>::value;
  };

  /*!\brief Symbolic representation of a variable substitution.
  */
  template<class Var, class Arg> struct Relation {
    Relation(const Var var, const Arg& val): _var(var), _val(val) {}

    const Var _var;
    const Arg& _val;
  };

  namespace detail {
    struct vidx_ID;
  };

  template<char idx>
  struct vidx : stator::orphan::value_conf_t<detail::vidx_ID, char, idx> {};

  /*!\brief Symbolic representation of a variable.

    This class is used to denote a variable.
  */
  template<typename ...Args> struct Var : SymbolicOperator {
    static constexpr const auto idx = stator::orphan::get_value<vidx<'x'>, Args...>::value;
    
    template<class Arg>
    Relation<Var<Args...>, Arg> operator=(const Arg& a) const {
      return Relation<Var<Args...>, Arg>(*this, a);
    }
  };

  template<typename Var1, typename Var2>
  struct variable_in {
    static constexpr const bool value = Var1::idx == Var2::idx;
  };

  template<typename Var1, typename Var2>
  struct enable_if_var_in : std::enable_if<variable_in<Var1, Var2>::value> {
  };
  
  template<typename Var1, typename Var2>
  struct enable_if_var_not_in : std::enable_if<!variable_in<Var1, Var2>::value> {
  };

  template<typename Var1, typename Var2>
  struct variable_combine {
    typedef Var<vidx<Var1::idx> > type;
  };
  
  
  namespace detail {
    /*!\brief Type trait to determine if a certain type is a constant.

	This is used to enable the derivative operation to convert
	these types to Null types. It is also to apply a
	specialised functions to these types.
    */
    template<class T>
    struct IsConstant : std::conditional<std::is_arithmetic<T>::value || is_C<T>::value || std::is_base_of<Eigen::EigenBase<T>, T>::value, std::true_type, std::false_type>::type {};

    template<class T>
    struct IsConstant<std::complex<T> > : IsConstant<T> {};
    
  }// namespace detail

  /*! \brief Returns the empty sum of a type.
    
    The empty sum is a term whose additive (and typically its
    subtractive) action is null (can be ignored). This definition
    only applies for selected types and assumes that their default
    constructors create the empty sum.
  */
  template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
  T empty_sum(const T&) { return T(); }

  template<class T, typename = typename std::enable_if<std::is_base_of<Eigen::EigenBase<T>, T>::value>::type>
  auto empty_sum(const T&) -> STATOR_AUTORETURN_BYVALUE(T::Zero());

  template<class T>
  T empty_sum(const std::complex<T>&) { return T(); }
  
  /*! \brief Default implementation of substitution of a symbolic
    expression at a given point.
    
    This implementation only applies if the term is a constant term.

    We deliberately return by const reference as, if this is an
    Eigen expression, the Eigen library may take an internal
    reference to this object to allow delayed evaluation. By
    returning the original object we can try to ensure its lifetime
    is at least longer than the current expression.
  */
  template<class T, class Var, class Arg,
	   typename = typename std::enable_if<detail::IsConstant<T>::value>::type >
  auto sub(const T& f, const Relation<Var, Arg>&) -> STATOR_AUTORETURN_BYVALUE(f);
  
  /*! \brief Evaluates a symbolic Var at a given point.
    
    This is only used if the Var is correct 
    substitution.
  */
  template<typename ...Args1, typename ...Args2, class Arg,
	     typename = typename enable_if_var_in<Var<Args1...>, Var<Args2...> >::type>
  auto sub(const Var<Args1...>& f, const Relation<Var<Args2...>, Arg>& x)
   -> STATOR_AUTORETURN_BYVALUE(x._val);

  /*! \brief Evaluates a symbolic Var at a given point.
    
    This is only used if the Var is not the correct letter for the
    substitution.
  */
  template<class ...Args1, class Arg, class Var2,
	     typename = typename enable_if_var_not_in<Var<Args1...>, Var2>::type>
  Var<Args1...> sub(const Var<Args1...>& f, const Relation<Var2, Arg>& x)
  { return f; }
  
  /*! \brief Output operator for Var types. */
  template<class ...Args>
  inline std::ostream& operator<<(std::ostream& os, const Var<Args...>&) {
    os << Var<Args...>::idx;
    return os;
  }

  /*! \brief Output operator for Relation types. */
  template<class Var, class Arg>
  inline std::ostream& operator<<(std::ostream& os, const Relation<Var, Arg>& sub) {
    os << Var::idx << " <- " << sub._val;
    return os;
  }
  
  /*! \brief Determine the derivative of a symbolic expression.
    
    This default implementation gives all consants
    derivative of zero.
  */
  template<class T, class ...Args,
	     typename = typename std::enable_if<detail::IsConstant<T>::value>::type>
  Null derivative(const T&, Var<Args...>) { return Null(); }

  /*! \brief Determine the derivative of a variable.

    If the variable is the variable in which a derivative is being
    taken, then this overload should be selected to return
    Unity.
  */
  template<class ...Args1, class ...Args2,
	     typename = typename enable_if_var_in<Var<Args1...>, Var<Args2...> >::type>
  Unity derivative(Var<Args1...>, Var<Args2...>) { return Unity(); }

  inline StackVector<double, 0> solve_real_roots(Null f) {
    return StackVector<double, 0>();
  }

  /*! \brief Determine the derivative of a variable by another variable.

    If the variable is NOT the variable in which a derivative is
    being taken, then this overload should be selected to return
    Null.
  */
  template<class ...Args1, class ...Args2,
	     typename = typename enable_if_var_not_in<Var<Args1...>, Var<Args2...> >::type>
  Null derivative(Var<Args1...>, Var<Args2...>) { return Null(); }

  /*! \brief Shift a function forward. It returns \f$g(x)=f(x+a)\f$

    For constant terms, these remain the same so this generic
    implementation does nothing.
  */
  template<class F, class Real,
	     typename = typename std::enable_if<detail::IsConstant<F>::value>::type>
  inline F shift_function(const F& f, const Real t) {
    return f;
  }
  
  /*! \brief Estimate the error in evaluating a function at a given time.
   */
  template<class F, class Real,
	     typename = typename std::enable_if<detail::IsConstant<F>::value>::type>
  inline double precision(const F& f, const Real) {
    return 0.0;
  }

  template<size_t Order, class Real = double, class PolyVar = Var<>> class Polynomial;
}


#include "stator/symbolic/binary_ops.hpp"
#include "stator/symbolic/unary_ops.hpp"
#include "stator/symbolic/simplify.hpp"
#include "stator/symbolic/polynomial.hpp"
#include "stator/symbolic/integrate.hpp"

namespace sym {
  namespace detail {
    template<size_t State, size_t max_Order, class Var>
    struct TaylorSeriesWorker {
	template<class Real>
	static Null eval(const Null& f, const Real& a)
	{ return Null(); }
      
	template<class F, class Real>
	static auto eval(const F& f, const Real& a) 
        -> STATOR_AUTORETURN((typename InvFactorial<State>::value() * sub(f, Var() = a) + (Var() - a) * TaylorSeriesWorker<State+1, max_Order, Var>::eval(derivative(f, Var()), a)))
    };

    template<size_t max_Order, class Var>
    struct TaylorSeriesWorker<max_Order,max_Order, Var> {
	template<class F, class Real>
	static auto eval(const F& f, const Real& a)
        -> STATOR_AUTORETURN((typename InvFactorial<max_Order>::value() * sub(f, Var() = a)));
      
	template<class Real>
	static Null eval(const Null& f, const Real& a)
	{ return Null(); }
    };
  }

  /*! \brief Generate a Taylor series representation of a Symbolic
    expression.
  */
  template<size_t Order, class Var, class F, class Real>
  auto taylor_series(const F& f, Real a, Var)
    -> STATOR_AUTORETURN(try_simplify(detail::TaylorSeriesWorker<0, Order, Var>::eval(f, a)));
} // namespace symbolic

