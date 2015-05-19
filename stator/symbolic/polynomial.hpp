/*! @file */
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
#include "stator/exception.hpp"
#include "stator/config.hpp"
#include "stator/symbolic/numeric.hpp"
#include "stator/symbolic/operators.hpp"
#include <boost/math/tools/roots.hpp>
#include <stdexcept>
#include <ostream>
#include <array>
#include <tuple>

namespace stator {
  namespace symbolic {
    namespace detail {
      constexpr size_t max_order(size_t N, size_t M)
      { return N > M ? N : M; }
          
      /*! \relates Polynomial 
	
 	\brief Type trait which determines if an operation
 	(multiplication, addition) can be distributed over the
 	coefficients of a polynomial.

 	This general form allows all operations between all constants.
      */
      template<class OpType, class PolyType>
      struct distribute_poly {
 	static const bool value = (detail::IsConstant<OpType>::value && detail::IsConstant<PolyType>::value);
      };

//      /*! \relates Polynomial 
//	
// 	\brief Type trait enabling use of std::complex as a Polynomial
// 	coefficient with arithmetic types.
//      */
//      template<class OpType, class T>
//      struct distribute_poly<OpType, std::complex<T> > {
// 	static const bool value = std::is_arithmetic<OpType>::value;
//      };
//
//      /*! \relates Polynomial 
//	
// 	\brief Type trait enabling use of std::complex as an operation
// 	with Polynomials with arithmetic type coefficients.
//      */
//      template<class T, class PolyType>
//      struct distribute_poly<std::complex<T>, PolyType> {
// 	static const bool value = std::is_arithmetic<PolyType>::value;
//      };
//
//      /*! \relates Polynomial 
//	
// 	\brief Type trait enabling use of std::complex as an operation
// 	with Polynomials.
//      */
//      template<class T>
//      struct distribute_poly<std::complex<T>, std::complex<T> > {
// 	static const bool value = true;
//      };
    }// namespace detail

    namespace { template<size_t T> struct dependent_false: std::false_type {}; }

    template<size_t Order, std::intmax_t num, std::intmax_t denom, char Letter>
    class Polynomial<Order, C<num, denom>, Letter> {
      static_assert(dependent_false<Order>::value,  "Cannot use C types as the coefficients of a polynomial");
    };

    /*! \brief Representation of Polynomial with basic algebra operations.

      This class allows basic computer algebra to be performed with
      Polynomial equations.
    
      For example, the polynomial \f$f(x)=x^2 + 2\,x + 3\f$ can be created like so:
      \code{.cpp}
      Polynomial<1> x{0,1};
      auto f = x*x + 2*x +3;    
      \endcode
      And evaluated at the point \f$x=3\f$ like so:
      \code{.cpp}
      double val = f(3);    
      \endcode
    
      The class also functions with Eigen vector or matrix coefficients.

      \tparam Order The order of the Polynomial.

      \tparam Real The type of the coefficients of the
      Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    class Polynomial : public std::array<Real, Order+1>
    {
      typedef std::array<Real, Order+1> Base;

    public:
      using Base::operator[];
      /*! \brief Default constructor.  

 	This initialises all Polynomial orders to be equivalent to
 	zero.
      */
      Polynomial() { Base::fill(empty_sum(Real())); }

      Polynomial(const PowerOp<Variable<Letter>, Order>&) { 
	Base::fill(empty_sum(Real()));
	Base::operator[](Order) = Real(1);
      }
    
      /*! \brief List initializer for simple Polynomial construction. 
	
 	This allows a polynomial to be constructed using just a list
 	of coefficients. E.g.
 	\code{.cpp}
 	Polynomial<1> f{0.5,1,2};
 	//f = 2 * x*x + x + 0.5;
 	\endcode
	
      */
      Polynomial(std::initializer_list<Real> _list) {
 	if (_list.size() > Order+1)
 	  throw std::length_error("initializer list too long");
    
 	size_t i = 0;
 	for (auto it = _list.begin(); it != _list.end(); ++i, ++it)
 	  Base::operator[](i) = *it;

 	for (; i <= Order; ++i)
 	  Base::operator[](i) = empty_sum(Real());
      }

      template<class InputIt>
      Polynomial(InputIt first, InputIt last) {
	size_t i = 0;
	auto it = first;
 	for (; (it != last) && (i <= Order); ++i, ++it)
 	  Base::operator[](i) = *it;
	
	if (it != last)
	  stator_throw() << "Polynomial type too short to hold this range of coefficients";

 	for (; i <= Order; ++i)
 	  Base::operator[](i) = empty_sum(Real());
      }

      /*! \brief Constructor for constructing higher-order Polynomial
          types from lower order Polynomial types. 
      */
      template<size_t N, class Real2,
	       typename = typename std::enable_if<(N <= Order)>::type >
	       Polynomial(const Polynomial<N, Real2, Letter>& poly) {
 	size_t i(0);
 	for (; i <= N; ++i)
 	  Base::operator[](i) = poly[i];
 	for (; i <= Order; ++i)
 	  Base::operator[](i) = empty_sum(Real());
      }

      /*! \brief Unary negative operator to change the sign of a Polynomial. */
      Polynomial operator-() const {
	Polynomial retval;
	for (size_t i(0); i <= Order; ++i)
	  retval[i] = -Base::operator[](i);
	return retval;
      }
    };
    
    /*! \brief Change the order of a Polynomial.

      This can be dangerous, as if the order of a polynomial is
      lowered high order terms are simply truncated.
    */
    template<size_t NewOrder, size_t Order, class Real, char Letter>
    Polynomial<NewOrder, Real, Letter> change_order(const Polynomial<Order, Real, Letter>& f) {
      //The default constructor blanks Polynomial coefficients to zero
      Polynomial<NewOrder, Real, Letter> retval;
      //Just copy the coefficients which overlap between the new and old polynomial orders.
      std::copy(f.begin(), f.begin() + std::min(Order, NewOrder) + 1, retval.begin());
      return retval;
    }

    /*! \relates Polynomial 
      \name Polynomial set properties
      \{
    */

    /*! \brief Returns the empty sum of a Polynomial.
      
      The empty sum is a term whose multiplicative action is null (can
      be ignored).
    */
    template<size_t Order, class Real, char Letter>
    constexpr Polynomial<Order, Real, Letter> empty_sum(const Polynomial<Order, Real, Letter>&)
    { return Polynomial<Order, Real, Letter>{}; }

    /*! \} */
    
    /*! \relates Polynomial 

      \name Polynomial algebraic operations
      
      For all operations below we do not assume that we have a
      closure. For example, a vector multiplied by a vector is a
      scalar therefore the * operator may change the returned type of
      the polynomial.
      \{
    */

    /*! \brief Optimised Polynomial substitution which performs an
        exchange of the Polynomial Variable.
     */
    template<class Real, size_t Order, char Var1, char Var2>
    Polynomial<Order, Real, Var2> substitution(const Polynomial<Order, Real, Var1>& f, const VariableSubstitution<Var1, Variable<Var2> >& x_container)
    { return Polynomial<Order, Real, Var2>(f.begin(), f.end()); }

    /*! \brief Optimised Polynomial substitution for Null insertions.
     */
    template<size_t Order, class Real, char Letter>
    Real substitution(const Polynomial<Order, Real, Letter>& f, const VariableSubstitution<Letter, Null>&)
    { return f[0]; }

    /*! \brief Numerically Evaluates a Polynomial expression at a given point.

      This function also specially handles the cases where
      \f$x=+\infty\f$ or \f$-\infty\f$ and returns the correct sign of
      infinity (if the polynomial has one non-zero coefficients of
      x). This behaviour is crucial as it is used in the evaluation of
      Sturm chains.
     */
    template<class Real, size_t Order, char Letter, class Real2,
	     typename = typename std::enable_if<std::is_arithmetic<Real2>::value
                                                //Avoid this case if its a Vector expression
                                                && !std::is_base_of<Eigen::EigenBase<Real>, Real>::value
                                                && !std::is_base_of<Eigen::EigenBase<Real2>, Real2>::value
                                                >::type>
    STORETYPE(Real() * Real2()) substitution(const Polynomial<Order, Real, Letter>& f, const VariableSubstitution<Letter, Real2>& x_container)
    {
      //Handle the case where this is actually a constant and not a
      //Polynomial. This is free to evaluate now as Order is a
      //template parameter.
      if (Order == 0)
	return f[0];

      const auto & x = x_container._val;
      //Special cases for infinite values of x
      if (std::isinf(x)) {
	//Look through the Polynomial to find the highest order term
	//with a non-zero coefficient.
	for(size_t i = Order; i > 0; --i)
	  if (f[i] != empty_sum(f[0])) {
	    //Determine if this is an odd or even function of x
	    if (Order % 2)
	      //This is an odd function of x.
	      return (1 - 2 * (std::signbit(f[i]) ^ std::signbit(x))) * std::numeric_limits<Real>::infinity();
	    else
	      //This is an even function of x, the sign of x doesn't
	      //matter!
	      return (1 - 2 * std::signbit(f[i])) * std::numeric_limits<Real>::infinity();
	  };
	//All terms in x have zero as their coefficient
	return f[0];
      }

      typedef STORETYPE(Real() * Real2()) RetType;
      RetType sum = RetType();
      for(size_t i = Order; i > 0; --i)
	sum = sum * x + f[i];
      sum = sum * x + f[0];
      return sum;
    }

    namespace detail {
      /*! \brief Worker class for symbolically evaluating a substitution on
          a Polynomial.
       */
      template<size_t Stage>
      struct PolySubWorker {
	template<size_t Order, char Letter, class Real, class X>
	static auto eval(const Polynomial<Order, Real, Letter>& f, const X& x) -> decltype(f[Order - Stage] + x * PolySubWorker<Stage - 1>::eval(f, x))
	{
	  return f[Order - Stage] + x * PolySubWorker<Stage - 1>::eval(f, x);
	}
      };
      
      /*! \brief Worker class for symbolically evaluating a substitution on
          a Polynomial.
	  
	  This is the closure for the evaluation.
       */
      template<>
      struct PolySubWorker<0> {
	template<size_t Order, char Letter, class Real, class X>
	static auto eval(const Polynomial<Order, Real, Letter>& f, const X& x) -> decltype(f[Order]) {
	  return f[Order];
	}
      };
    }
    /*! \brief Symbolically evaluates a Polynomial expression.

      As the intermediate and final return types of a symbolic
      evaluation are unknown, this must be handled using template
      metaprogramming to unfold the multiplication. This is provided
      by the detail::PolySubWorker classes.

      The method used to evaluate the polynomial is known as Horner's
      method.
     */
    template<class Real, size_t Order, char Letter, class Real2,
	     typename = typename std::enable_if<!std::is_arithmetic<Real2>::value
                                                || (std::is_base_of<Eigen::EigenBase<Real2>, Real2>::value && std::is_arithmetic<Real2>::value)
                                                || (std::is_base_of<Eigen::EigenBase<Real>, Real>::value && std::is_arithmetic<Real2>::value)
                                                >::type>
    auto substitution(const Polynomial<Order, Real, Letter>& f, const VariableSubstitution<Letter, Real2>& x_container) -> decltype(detail::PolySubWorker<Order>::eval(f, x_container._val))
    { return detail::PolySubWorker<Order>::eval(f, x_container._val); }

    /*! \brief Fast evaluation of multiple derivatives of a
        polynomial.
      
      This function is provided to allow derivatives to be evaluated
      without symbolically taking the derivative (causing a copy of
      the coefficients).
    */
    template<size_t D, size_t Order, class Real, char Letter, class Real2>
    std::array<Real, D+1> eval_derivatives(const Polynomial<Order, Real, Letter>& f, const Real2& x)
    {
      std::array<Real, D+1> retval;
      retval.fill(empty_sum(Real()));
      retval[0] = f[Order];
      for (size_t i(Order); i>0; --i) {
	for (size_t j = std::min(D, Order-(i-1)); j>0; --j)
	  retval[j] = retval[j] * x + retval[j-1];
	retval[0] = retval[0] * x + f[i-1];
      }

      Real cnst(1.0);
      for (size_t i(2); i <= D; ++i) {
	cnst *= i;
	retval[i] *= cnst;
      }

      return retval;
    }

    /*! \brief Perform Euclidean division of a polynomial.
      
      Given two polynomials \f$f(x)\f$ and \f$g(x)\f$, the Euclidean
      division is a determination of the quotient polynomial
      \f$q(x)\f$ and the remainder polynomial \f$r(x)\f$, which satisfy
      
      \f[
      f(x) = g(x)\,q(x) + r(x)
      \f]

      If g(x) only consists of factors of f(x), the remainder will be
      zero. The algorithm used here is based on polynomial long
      division.

      If the division is by a monomial \f$g(x)=(x-a)\f$ where \f$a\f$
      is a root of \f$f(x)\f$, then the deflate_polynomial function
      should be used as it is more numerically stable and efficient.

      As g(x) may contain leading order coefficients which are zero,
      we cannot lower the order of the quotient polynomial returned.
    */
    template<size_t Order1, class Real, char Letter, size_t Order2>
    std::tuple<Polynomial<Order1, Real, Letter>, Polynomial<Order2 - 1, Real, Letter> >
    euclidean_division(const Polynomial<Order1, Real, Letter>& f, const Polynomial<Order2, Real, Letter>& g)
    {
      static_assert(Order2 < Order1, "Cannot perform division when the order of the denominator is larger than the numerator using this routine");
      static_assert(Order2 > 0, "Constant division fails with these loops");
      typedef std::tuple<Polynomial<Order1, Real, Letter>, Polynomial<Order2 - 1, Real, Letter> > RetType;
      //If the leading term of g is zero, drop to a lower order
      //euclidean division.
      if (g[Order2] == 0) {
	auto lower_order_op = euclidean_division(f, change_order<Order2 - 1>(g));
	return RetType(std::get<0>(lower_order_op), std::get<1>(lower_order_op));
      }

      //The quotient and remainder.
      Polynomial<Order1, Real, Letter> r(f);
      Polynomial<Order1, Real, Letter> q;

      //Loop from the highest order coefficient of f, down to where we
      //have a polynomial one order lower than g.
      for (size_t k(Order1); k>=Order2; --k) {
	//Calculate the term on the quotient
	q[k-Order2] = r[k] / g[Order2];
	//Subtract this factor of other terms from the remainder
	for (size_t j(0); j <= Order2; j++)
	  r[k+j-Order2] -= q[k-Order2] * g[j];
      }

      return RetType(q, change_order<Order2 - 1>(r));
    }

    /*!  \cond Specializations
      \brief Specialisation for division by a constant.
     */
    template<size_t Order1, class Real, char Letter>
    std::tuple<Polynomial<Order1, Real, Letter>, Polynomial<0, Real, Letter> >
    euclidean_division(const Polynomial<Order1, Real, Letter>& f, const Polynomial<0, Real, Letter>& g)
    {
      typedef std::tuple<Polynomial<Order1, Real, Letter>, Polynomial<0, Real, Letter> > RetType;
      if (g[0] == 0)
	return RetType(Polynomial<Order1, Real, Letter>{std::numeric_limits<Real>::infinity()}, 
		       Polynomial<0, Real, Letter>{empty_sum(Real())});

      return RetType(f * (1.0 / g[0]), Polynomial<0, Real, Letter>{empty_sum(Real())});
    }

    /*! \brief left-handed addition of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator+(const Null& r, const Polynomial<Order, Real, Letter>& poly)
    { return poly; }

    /*! \brief Right-handed addition of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator+(const Polynomial<Order, Real, Letter>& poly, const Null& r)
    { return poly; }

    /*! \brief left-handed subtraction of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    Polynomial<Order, Real, Letter> operator-(const Null& r, const Polynomial<Order, Real, Letter>& poly)
    { return -poly; }

    /*! \brief Right-handed subtraction of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator-(const Polynomial<Order, Real, Letter>& poly, const Null& r)
    { return poly; }

    /*! \brief left-handed multiplication of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    Null operator*(const Null& r, const Polynomial<Order, Real, Letter>& poly)
    { return Null(); }

    /*! \brief Right-handed multiplication of Null on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    Null operator*(const Polynomial<Order, Real, Letter>& poly, const Null& r)
    { return Null(); }

    /*! \brief left-handed multiplication of Unity on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator*(const Unity& r, const Polynomial<Order, Real, Letter>& poly)
    { return poly; }

    /*! \brief Right-handed multiplication of Unity on a Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator*(const Polynomial<Order, Real, Letter>& poly, const Unity& r)
    { return poly; }

    /*! \brief Right-handed division of a Polynomial by Unity.
    */
    template<size_t Order, class Real, char Letter>
    const Polynomial<Order, Real, Letter>& operator/(const Polynomial<Order, Real, Letter>& poly, const Unity& r)
    { return poly; }

    /*! \brief Right-handed addition operation on a Polynomial.
      
      This operator is only enabled if the type of the Polynomial
      coefficients and the type being added is marked as compatitble
      for distribution over the Polnomial coefficients. This is tested
      using detail::distribute_poly.
    */
    template<class Real1, size_t Order, class Real, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real>::value>::type>
    auto operator+(const Real1& r, const Polynomial<Order, Real, Letter>& poly) -> decltype(poly+r)
    { return poly + r; }

    /*!\brief Left-handed addition operator for Polynomials 

      This operator is only enabled if the type of the Polynomial
      coefficients and the type being added is marked as compatitble
      for distribution over the Polnomial coefficients. This is tested
      using detail::distribute_poly.
    */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type>
    auto operator+(const Polynomial<N, Real1, Letter>& poly, const Real2& r) -> Polynomial<N, STORETYPE(poly[0] + r), Letter>
    {
      Polynomial<N, STORETYPE(poly[0] + r), Letter> retval(poly);
      retval[0] = retval[0] + r;
      return retval;
    }

    /*!\brief Addition operator for two Polynomial types. 
     */
    template<size_t M, size_t N, class Real1, class Real2, char Letter>
    auto operator+(const Polynomial<M, Real1, Letter>& poly1, const Polynomial<N, Real2, Letter>& poly2) -> Polynomial<detail::max_order(M, N), STORETYPE(poly1[0] + poly2[0]), Letter>
    {
      Polynomial<detail::max_order(M, N), STORETYPE(poly1[0] + poly2[0]), Letter> retval(poly1);
      for (size_t i(0); i <= N; ++i)
	retval[i] += poly2[i];
      return retval;
    }

    /*! \brief Right-handed subtraction operator for Polynomial types.
     
      This will reorder and convert the operation to a unary negation
      operator with an addition if the left-handed addition form
      exists.
    */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type>
    auto operator-(const Real1& r, const Polynomial<N, Real2, Letter>& poly) -> decltype((-poly) + r)
    { return (-poly) + r; }
  
    /*! \brief Left-handed subtraction from a Polynomial type.

      This will convert the operation to a unary negation operator
      with an addition if the left-handed form exists.
    */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type >
    auto operator-(const Polynomial<N,Real1,Letter>& poly, const Real2& r) -> decltype(poly + (-r))
    { return poly + (-r); }

    /*! \brief Subtraction between two Polynomial types. 
     */
    template<class Real1, class Real2, size_t M, size_t N, char Letter>
    auto operator-(const Polynomial<M,Real1, Letter>& poly1, const Polynomial<N,Real2, Letter>& poly2) -> Polynomial<detail::max_order(M, N),decltype(poly1[0]-poly2[0]), Letter>
    {
      Polynomial<detail::max_order(M, N),decltype(poly1[0]-poly2[0]), Letter> retval(poly1);
      for (size_t i(0); i <= N; ++i)
	retval[i] -= poly2[i];
      return retval;
    }

    /*! \brief Right-handed multiplication operation on a Polynomial.
      
      This operator is only enabled if the type of the Polynomial
      coefficients and the type being added is marked as compatitble
      for distribution over the Polnomial coefficients. This is tested
      using detail::distribute_poly.
    */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type>
    auto operator*(const Real1& r, const Polynomial<N, Real2, Letter>& poly) -> Polynomial<N, decltype(poly[0] * r), Letter>
    { return poly * r; }

    /*! \brief Left-handed multiplication on a Polynomial.

      This operator is only enabled if the type of the Polynomial
      coefficients and the type being added is marked as compatitble
      for distribution over the Polnomial coefficients. This is tested
      using detail::distribute_poly.
    */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type>
    auto operator*(const Polynomial<N, Real1, Letter>& poly, const Real2& r) -> Polynomial<N, decltype(poly[0] * r), Letter>
    {
      Polynomial<N, decltype(Real1() * Real2()), Letter> retval;
      for (size_t i(0); i <= N; ++i)
	retval[i] = poly[i] * r;
      return retval;
    }

    /*! \brief Multiplication between two Polynomial types.
     */
    template<class Real1, class Real2, size_t M, size_t N, char Letter>
    auto operator*(const Polynomial<M, Real1, Letter>& poly1, const Polynomial<N, Real2, Letter>& poly2) -> Polynomial<M + N, STORETYPE(poly1[0] * poly2[0]), Letter>
    {
      Polynomial<M + N, STORETYPE(poly1[0] * poly2[0]), Letter> retval;
      for (size_t i(0); i <= N+M; ++i)
	for (size_t j(i>N?i-N:0); (j <= i) && (j <=M); ++j)
	  retval[i] += poly1[j] * poly2[i-j];
      return retval;
    }

    /*! \brief Specialisation for squares of matrix expressions. */
    template<size_t Power, class Matrix, size_t N, char Letter,
             typename = typename std::enable_if<(Power==2) && std::is_base_of<Eigen::EigenBase<Matrix>, Matrix>::value>::type>
      auto pow(const Polynomial<N, Matrix, Letter>& f) -> Polynomial<2 * N, STORETYPE(f[0].dot(f[0])), Letter>
    { 
      Polynomial<2 * N, STORETYPE(f[0].dot(f[0])), Letter> retval;
      for (size_t i(0); i <= 2 * N; ++i)
	for (size_t j(i>N?i-N:0); (j <= i) && (j <=N); ++j)
	  retval[i] += f[j].dot(f[i-j]);
      return retval;
    }

    /*! \brief Division of a Polynomial by a constant. */
    template<class Real1, class Real2, size_t N, char Letter,
	     typename = typename std::enable_if<detail::distribute_poly<Real1, Real2>::value>::type>
    auto operator/(const Polynomial<N, Real1, Letter>& poly, const Real2& r) -> Polynomial<N, STORETYPE(poly[0] / r), Letter>
    {
      Polynomial<N, STORETYPE(poly[0] / r), Letter> retval;
      for (size_t i(0); i <= N; ++i)
	retval[i] = poly[i] / r;
      return retval;
    }

    /*! \brief Enable reordering of Polynomial types. */
    template<class R1, size_t N1, class R2, size_t N2, char Letter> 
    struct Reorder<Polynomial<N1, R1, Letter>, Polynomial<N2, R2, Letter> > {
      static const bool value = true;
    };

    /*! \brief Enable reordering of Polynomial types with arithmetic types. */
    template<class R1, size_t N1, class R2, char Letter> 
    struct Reorder<Polynomial<N1, R1, Letter>, R2 > {
      static const bool value = std::is_arithmetic<R2>::value;
    };

    /*! \brief Enable reordering of Polynomial types with arithmetic types. */
    template<class R1, size_t N1, class R2, char Letter> 
    struct Reorder<R2,Polynomial<N1, R1, Letter> > {
      static const bool value = std::is_arithmetic<R2>::value;
    };

    /*! \endcond */

    /*! \} */

    /*! \relates Polynomial 
      \name Polynomial calculus operations
      \{
    */

    /*! \brief Derivatives of Polynomial types.
      
      This specialisation is only activated if this is a derivative in
      the correct variable AND the Order of the Polynomial is greater
      than zero.
     */
    template<char Letter, class Real, size_t N, char dLetter,
	     typename = typename std::enable_if<(Letter==dLetter) && (N > 0)>::type>
      inline Polynomial<N-1, Real, Letter> derivative(const Polynomial<N, Real, Letter>& f, Variable<dLetter>) {
      Polynomial<N-1, Real, Letter> retval;
      for (size_t i(0); i < N; ++i) {
	retval[i] = f[i+1] * (i+1);
      }
      return retval;
    }

    /*! \brief Derivatives of Polynomial types.
      
      This specialisation is only activated if this is a derivative in
      the incorrect variable.
     */
    template<char Letter, class Real, size_t N, char dLetter,
	     typename = typename std::enable_if<(Letter!=dLetter) || (N==0)>::type>
      Null derivative(const Polynomial<N, Real, Letter>& f, Variable<dLetter>) 
    { return Null(); }

    /*! \endcond \} */

    namespace detail {
      template<class T>
      typename std::enable_if<!std::is_base_of<Eigen::EigenBase<T>, T>::value>::type print_coeff(std::ostream& os, const T& val) {
	os << val;
      }

      template<class T>
      typename std::enable_if<std::is_base_of<Eigen::EigenBase<T>, T>::value>::type print_coeff(std::ostream& os, const T& val) {
	if ((val.cols() == 1) && (val.rows()==1))
	  os << val;
	else if (val.cols() == 1) {
	  os << "{ ";
	  for (size_t i(0); i < val.rows(); ++i)
	    os << val(i, 0) << " ";
	  os << "}^T";
	} else {
	  os << "{ ";
	  for (size_t i(0); i < val.cols(); ++i) {
	    os << "{ ";
	    for (size_t j(0); j < val.rows(); ++j)
	      os << val(i, j) << " ";
	    os << "} ";
	  }
	  os << "}";
	}
      }
    }// namespace detail

    /*! \relates Polynomial 
      \name Polynomial input/output operations
      \{
    */
    /*! \brief Writes a human-readable representation of the Polynomial to the output stream. */
    template<class Real, size_t N, char Letter>
    inline std::ostream& operator<<(std::ostream& os, const Polynomial<N, Real, Letter>& poly) {
      std::ostringstream oss;
      oss.precision(os.precision());
      size_t terms = 0;
      for (size_t i(N); i != 0; --i) {
	if (poly[i] == empty_sum(poly[i])) continue;
	if (terms != 0)
	  oss << " + ";
	++terms;
        detail::print_coeff(oss, poly[i]);
	oss << " × " << Letter;
	if (i > 1)
	  switch (i) {
	  case 2: oss << "²"; break;
	  case 3: oss << "³"; break;
	  default: oss << "^" << i; break;
	  }
      }
      if ((poly[0] != empty_sum(poly[0])) || (terms == 0)) {
	if (terms != 0)
	  oss << " + ";
	++terms;
	detail::print_coeff(oss, poly[0]);
      }
      os << oss.str();
      return os;
    }
    /*! \} */
    
    /*! \relates Polynomial 
      \name Polynomial transformations
      \{
    */
    /*! \brief Returns a polynomial \f$g(x)=f(x+t)\f$.
      
      Given a polynomial \f$f(x)\f$:
      \f[
      f(x) = \sum_{i=0}^N a_i\,x^i
      \f]
      
      We wish to determine the coefficients of a polynomial
      \f$g(x)=f(x+t)\f$:

      \f[
      g(x) = \sum_{i=0}^N b_i\,x^i
      \f]

      We can define \f$g(x)\f$ by taking a Taylor expansion of
      \f$f(x)\f$ about the point \f$t\f$, we have:
      
      \f[
      g(x) = f(t+x) = \sum_{i=0}^N \frac{f^i(t)}{i!}x^i
      \f]
      
      where \f$f^i(x)\f$ is the \f$i\f$th derivative of \f$f(x)\f$ and
      \f$N\f$ is the order of the polynomial \f$f\f$. Each coefficient
      of \f$g\f$ is then given by:
      
      \f[
      b_i = \frac{f^i(t)}{i!}
      \f]

      Here, we then use a modified version of the \ref
      eval_derivatives function to actually calculate the derivatives
      while avoiding the factorial term.
     */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order, double, Letter> shift_function(const Polynomial<Order, Real, Letter>& f, const double t) 
    {
      //Check for the simple case where t == 0, nothing to be done
      if (t == 0) return f;

      Polynomial<Order, double, Letter> retval;
      retval.fill(empty_sum(Real()));
      retval[0] = f[Order];
      for (size_t i(Order); i>0; i--) {
	for (size_t j = Order-(i-1); j>0; j--)
	  retval[j] = retval[j] * t + retval[j-1];
	retval[0] = retval[0] * t + f[i-1];
      }
      return retval;
    }

    /*! \brief Returns a polynomial \f$g(x)=f(x+1)\f$.

      This is an optimised \ref shift_function operation where the
      shift is unity. See \ref shift_function for more
      implementation details.
     */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order, Real, Letter> shift_function(const Polynomial<Order, Real, Letter>& f, Unity) {
      Polynomial<Order, Real, Letter> retval;
      retval.fill(empty_sum(Real()));
      retval[0] = f[Order];
      for (size_t i(Order); i>0; i--) {
	for (size_t j = Order-(i-1); j>0; j--)
	  retval[j] += retval[j-1];
	retval[0] += f[i-1];
      }
      return retval;
    }

    /*! \brief Returns a polynomial \f$g(x)=(x+1)^{d}
        f\left(\frac{1}{x+1}\right)\f$.

	The creation of \f$p(x)\f$, given by
	\f[
	p(x) = \left(x+1\right)^d\,f\left(\frac{1}{x+1}\right)
	\f]

	where \f$d\f$ is the order of the polynomial \f$f(x)\f$, is
	often carried out while locating roots of a Polynomial. It has
	the useful property of generating a polynomial which has the
	same number of roots of \f$f(x)\f$ in the range
	\f$x\in[0,\,1]\f$, but now in the range
	\f$x\in[\infty,\,0]\f$. Therefore, small sections of a
	polynomial may be inspected for roots using scaling, shifting,
	and this transformation. It is a special case of a Mobius
	transformation of the Polynomial.

	Creation of \f$p(x)\f$ may be carried out in two steps. First,
	the following equation is generated:

	\f[
	p_1(x) = x^d\,f\left(\frac{1}{x}\right)
	\f]
	
	This operation may be performed simply by reversing the order
	of the coefficient array in the Polynomial. Then, a \ref
	shift_function is applied to complete the transformation:

	\f[
	p_2(x) = p_1\left(x+1\right)
	\f]

	This entire operation is performed using an optimised version
	of the \ref shift_polynomial function. Take a look at the
	implementation of \ref shift_polynomial and substitute in a
	shift value of 1 to arrive at this implementation.
     */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order, Real, Letter> invert_taylor_shift(const Polynomial<Order, Real, Letter>& f) {
      Polynomial<Order, Real, Letter> retval;
      retval.fill(empty_sum(Real()));
      retval[0] = f[0];
      for (size_t i(Order); i>0; i--) {
	for (size_t j = Order-(i-1); j>0; j--)
	  retval[j] += retval[j-1];
	retval[0] += f[Order - (i-1)];
      }
      return retval;
    }

    /*! \brief Returns a polynomial \f$g(x)=f\left(-x\right)\f$.
     */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order, Real, Letter> reflect_poly(const Polynomial<Order, Real, Letter>& f) {
      Polynomial<Order, Real, Letter> g(f);
      for (size_t i(1); i <= Order; i+=2)
	g[i] = -g[i];
      return g;
    }

    /*! \brief Returns a polynomial \f$g(x)=f\left(a\,x\right)\f$
        where \f$a\f$ is the scaling factor.
     */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order, Real, Letter> scale_poly(const Polynomial<Order, Real, Letter>& f, const Real& a) {
      Polynomial<Order, Real, Letter> g(f);
      Real factor = 1;
      for (size_t i(1); i <= Order; ++i)
	g[i] *= (factor *= a);
      return g;
    }
    /*! \} */

    /*! \relates Polynomial 
      \name Polynomial roots
      \{
    */
    

    /*! \brief Calculate a maximum error estimate for the evaluation
        of the polynomial at \f$x\f$.
	
	The calculation of this value is outlined in "5.6 Root
	acceptance and refinement" of "A survey of numerical
	mathematics" vol.1. It is useful for setting accuracy limits
	while calculating roots.
     */
    template<class Real, size_t Order, class Real2, char Letter>
    Real precision(const Polynomial<Order, Real, Letter>& f, const Real2& x)
    {
      if (std::isinf(x))
	return 0.0;

      if (Order == 0)
	return 0;

      static_assert(std::is_floating_point<Real>::value, "This has only been implemented for floating point types");
      static_assert(std::numeric_limits<Real>::radix == 2, "This has only been implemented for base-2 floating point types");
      const int mantissa_digits = std::numeric_limits<Real>::digits;
      const Real eps = 1.06 / std::pow(2, mantissa_digits);

      Real sum = f[0];
      Real2 xn = std::abs(x);
      for(size_t i = 1; i <= Order; ++i) {
	sum += (2 * i + 1) * std::abs(f[i]) * xn;
	xn *= std::abs(x);
      }

      return sum * eps;
    }


    /*! \brief Factors out a root of a polynomial and returns a
      lower-order polynomial with the remaining roots.
	
      Given a polynomial, we can rearrange it in factored form like so
      \f[
      \sum_{i=0}^N a_i\,x^i =(x - r_1)\sum_{i=0}^{N-1} b_i\, x^{i}
      \f]
      
      where \f$r_1\f$ is a root of the polynomial. Equating terms on the
      LHS with terms on the RHS with equal powers of \f$x\f$, we have:

      \f[
      b_i=\frac{b_{i-1} - a_i}{r_1}\qquad \textrm{for}\ i\in[1,\,N-1]
      \f]

      This formula, known as backward deflation, can be used to
      calculate all coefficients using the starting point \f$b_0=-a_0
      / r_1\f$. This approach is not always stable (for example if the
      root is zero, or if \f$b_{i-1}\f$ has the same sign as \f$a_i\f$
      we might have catastrophic cancellation).
      
      An alternative "forward" iterative form may be found by
      substituting \f$i\to i+1\f$, which gives:

      \f[
      b_{i} = a_{i+1} + r_1\,b_{i+1} \qquad \textrm{for}\ i\in[0,\,N-2]
      \f]

      Again this approach may be used given the starting point
      \f$b_{N-1}=a_N\f$. However, it might also suffer from
      catastrophic cancellation if \f$a_{i+1}\f$ has the opposite sign
      to \f$r_1\,b_{i+1}\f$.

      It should be noted that Numerical Recipies states that "Forward
      deflation is stable if the largest absolute root is always
      divided out... backward deflation is stable if the smallest
      absolute root is always divided out". Unfortunately we do not
      know a priori the magnitude of the root being divided out.
      
      As both approaches may suffer from catastrophic cancellation, we
      decide to switch between them. If we catch the special
      root-is-zero case, we only must avoid catastrophic
      cancellation. This arises if two non-zero terms are subtracted
      from each other (i.e., for the first approach this happens if
      \f$a_{i+1}\f$ and \f$r_1\,b_{i+1}\f$ are non-zero and have
      opposite sign). We could use this to monitor the bits of
      precision "lost" as we calculate from each end and select a
      point between the two methods where accuracy is highest, but
      this would require a more detailed analysis of the error. A
      simple approach is to solve from both ends of the polynomial at
      the same time and only actually accept whichever has the lowest 
      catastrophic cancellation accuracy in terms of bits.

      \param f The Polynomial to factor a root out of.
      \param root The root to remove.
    */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order-1, Real, Letter> deflate_polynomial(const Polynomial<Order, Real, Letter>& a, const double root) {
      if (root == Real())
	return deflate_polynomial(a, Null());
      
      Polynomial<Order-1, Real, Letter> b;
      //Calculate the highest and lowest order coefficients using
      //these stable approaches
      b[Order-1] = a[Order];
      b[0] = - a[0] / root;

      size_t i_t = Order-2;
      size_t i_b = 1;
      while (i_t >= i_b) {
	const Real d = root * b[i_t + 1];	
	if (subtraction_precision(b[i_b], a[i_b]) > addition_precision(a[i_t+1], d)) {
	  b[i_b] = (b[i_b-1] - a[i_b]) / root;
	  ++i_b;
	} else {
	  b[i_t] = a[i_t+1] + d;
	  --i_t;
	}
      }
      return b;
    }

    /*! \brief A specialisation of deflation where the root is at
        zero. 
	
	Deflating out a root at zero becomes a division by x. As the
	constant term is zero, there is no residual and the division
	becomes a simple shifted copy.
    */
    template<size_t Order, class Real, char Letter>
    inline Polynomial<Order-1, Real, Letter> deflate_polynomial(const Polynomial<Order, Real, Letter>& a, Null) {
      Polynomial<Order-1, Real, Letter> b;
      std::copy(a.begin()+1, a.end(), b.begin());
      return b;
    }
    
    /*!  \cond Specializations
      \brief Specialisation for no roots of a 0th order Polynomial.
      \param f The Polynomial to evaluate.
    */
    template<char Letter>
    inline StackVector<double, 0> solve_real_roots(const Polynomial<0, double, Letter>& f) {
      return StackVector<double, 0>();
    }

    /*! \brief Calculate the single real root (if it exists) of a 1st
        order Polynomial.
      
      \param f The Polynomial to evaluate.
    */
    template<char Letter>
    inline StackVector<double, 1> solve_real_roots(const Polynomial<1, double, Letter>& f) {
      StackVector<double, 1> roots;
      if (f[1] != 0)
	roots.push_back(-f[0] / f[1]);
      return roots;
    }

    /*! \brief Calcualte the real roots of a 2nd order Polynomial
      using radicals.
      
      Roots are always returned sorted lowest-first.
      
      \param f The Polynomial to evaluate.
    */
    template<char Letter>
    inline StackVector<double, 2> solve_real_roots(Polynomial<2, double, Letter> f) {
      //If this is actually a linear polynomial, drop down to that solver.
      if (f[2] == 0) 
	return solve_real_roots(change_order<1>(f));
      
      //Scale the constant of x^2 to 1
      f = f / f[2];

      if (f[0] == 0)
	//There is no constant term, so we actually have x^2 + f[1] * x = 0
	return StackVector<double, 2>{-f[1]};
      
      static const double maxSqrt = std::sqrt(std::numeric_limits<double>::max());
      if ((f[1] > maxSqrt) || (f[1] < -maxSqrt)) {
	//arg contains f[1]*f[1], so it will overflow. In this case we
	//can approximate the equation as x^2 + a x = 0 to solve for
	//one root, and use root2 = f[0]/root1 to find the second
	//root. This should work even with large constant values
	StackVector<double, 2> retval{-f[1], -f[0] / f[1]};
	//Sort them into order
	if (retval[0] > retval[1])
	  std::swap(retval[0], retval[1]);
	return retval;
      }

      const double arg = f[1] * f[1] - 4 * f[0];

      //Test if there are no real roots
      if (arg < 0)
	return StackVector<double, 2>();

      //Test if there is a double root
      if (arg == 0)
	return StackVector<double, 2>{-f[1] * 0.5};

      //Return both roots
      const double root1 = -(f[1] + std::copysign(std::sqrt(arg), f[1])) * 0.5;
      const double root2 = f[0] / root1;
      StackVector<double, 2> retval{root1, root2};
      if (root1 > root2)
	std::swap(retval[0], retval[1]);
      return retval;
    }

    namespace {
      /*! \brief Deflate a Polynomial and solves for the remaining
	roots.

	This routine also polishes the root to try to improve accuracy;
	however, do not assume this function will behave well with
	inaccurate roots.
      */
      template<size_t Order, class Real, char Letter>
      inline StackVector<Real, Order> deflate_and_solve_polynomial(const Polynomial<Order, Real, Letter>& f, Real root) {
	StackVector<Real, Order> roots = solve_real_roots(deflate_polynomial(f, root));
	//The roots obtained through deflation are not the roots of
	//the original equation.  They will need polishing using the
	//original function:
	for (auto& root : roots)
	  //We don't test if the polishing fails. Without established
	  //bounds polishing is hard to do for multiple roots, so we
	  //just accept a polished root if it is available
          stator::numeric::halleys_method([&](double x){ return eval_derivatives<2>(f, x); }, root);
	
	roots.push_back(root);
	std::sort(roots.begin(), roots.end());
	return roots;
      }
    }
    
    /*! \brief Calculate the real roots of a 3rd order Polynomial
        using radicals.
      
	Roots are always returned sorted lowest-first.

      \param f The Polynomial to evaluate.
    */
    template<char Letter>
    inline StackVector<double, 3> solve_real_roots(const Polynomial<3, double, Letter>& f_original) {
      //Ensure this is actually a third order polynomial
      if (f_original[3] == 0)
	return solve_real_roots(change_order<2>(f_original));
      
      if (f_original[0] == 0)
	//If the constant is zero, one root is x=0.  We can divide
	//by x and solve the remaining quadratic
	return deflate_and_solve_polynomial(f_original, 0.0);

      //Convert to a cubic with a unity high-order coefficient
      auto f = f_original / f_original[3];
      
      if ((f[2] == 0) && (f[1] == 0))
	//Special case where f(x) = x^3 + f[0]
	return StackVector<double, 3>{std::cbrt(-f[0])};

      static const double maxSqrt = std::sqrt(std::numeric_limits<double>::max());
      
      if ((f[2] > maxSqrt) || (f[2] < -maxSqrt))
	//The equation is limiting to x^3 + f[2] * x^2 == 0. Use
	//this to estimate the location of one root, polish it up,
	//then deflate the polynomial and solve the quadratic.
	return deflate_and_solve_polynomial(f, -f[2]);

      //NOT SURE THESE RANGE TESTS ARE BENEFICIAL
      if (f[1] > maxSqrt)
	//Special case, if f[1] is large (and f[2] is not) the root is
	//near -f[0] / f[1], the x^3 term is negligble, and all other terms
	//cancel.
	return deflate_and_solve_polynomial(f, -f[0] / f[1]);

      if (f[1] < -maxSqrt)
	//Special case, equation is approximated as x^3 + q x == 0
	return deflate_and_solve_polynomial(f, -std::sqrt(-f[1]));

      if ((f[0] > maxSqrt) || (f[0] < -maxSqrt))
	//Another special case where equation is approximated as f(x)= x^3 +f[0]
	return deflate_and_solve_polynomial(f, -std::cbrt(f[0]));

      const double v = f[0] + (2.0 * f[2] * f[2] / 9.0 - f[1]) * (f[2] / 3.0);

      if ((v > maxSqrt) || (v < -maxSqrt))
	return deflate_and_solve_polynomial(f, -f[2]);
      
      const double uo3 = f[1] / 3.0 - f[2] * f[2] / 9.0;
      const double u2o3 = uo3 + uo3;
      
      if ((u2o3 > maxSqrt) || (u2o3 < -maxSqrt))
	{
	  if (f[2]==0)
	    {
	      if (f[1] > 0)
		return deflate_and_solve_polynomial(f, -f[0] / f[1]);
	      
	      if (f[1] < 0)
		return deflate_and_solve_polynomial(f, -std::sqrt(-f[1]));
		
	      return deflate_and_solve_polynomial(f, 0.0);
	    }

	  return deflate_and_solve_polynomial(f, -f[1] / f[2]);
	}

      const double uo3sq4 = u2o3 * u2o3;
      if (uo3sq4 > maxSqrt)
	{
	  if (f[2] == 0)
	    {
	      if (f[1] > 0)
		return deflate_and_solve_polynomial(f, -f[0] / f[1]);

	      if (f[1] < 0)
		return deflate_and_solve_polynomial(f, -std::sqrt(-f[1]));

	      return deflate_and_solve_polynomial(f, 0.0);
	    }

	  return deflate_and_solve_polynomial(f, -f[1] / f[2]);
	}

      const double j = (uo3sq4 * uo3) + v * v;

      if (j > 0) 
	{//Only one root (but this test can be wrong due to a
	  //catastrophic cancellation in j) (i.e. (uo3sq4 * uo3) == v
	  //* v) so we solve for this root then solve the deflated
	  //quadratic.

	  const double w = std::sqrt(j);
	  double root;
	  if (v < 0)
	    root = std::cbrt(0.5*(w-v)) - (uo3) * std::cbrt(2.0 / (w-v)) - f[2] / 3.0;
	  else
	    root = uo3 * std::cbrt(2.0 / (w+v)) - std::cbrt(0.5*(w+v)) - f[2] / 3.0;
	  
	  //We don't test if the polishing fails. Without established
	  //bounds polishing is hard to do for multiple roots, so we
	  //just accept a polished root if it is available
	  stator::numeric::halleys_method([&](double x){ return eval_derivatives<2>(f, x); }, root);

	  return deflate_and_solve_polynomial(f, root);
	}
  
      if (uo3 >= 0)
	//Triple root detected
	return StackVector<double, 3>{std::cbrt(v) - f[2] / 3.0};

      const double muo3 = - uo3;
      double s = 0;
      if (muo3 > 0)
	{
	  s = std::sqrt(muo3);
	  if (f[2] > 0) s = -s;
	}
      
      const double scube = s * muo3;
      if (scube == 0)
	return StackVector<double, 3>{ -f[2] / 3.0 };
      
      double t = - v / (scube + scube);
      //Clamp t, as in certain edge cases it might numerically fall
      //outside of the valid range of acos
      t = std::min(std::max(t, -1.0), +1.0);
      const double k = std::acos(t) / 3.0;
      const double cosk = std::cos(k);
      
      StackVector<double, 3> roots{ (s + s) * cosk - f[2] / 3.0 };
      
      const double sinsqk = 1.0 - cosk * cosk;
      if (sinsqk < 0)
	return roots;

      double rt3sink = std::sqrt(3.0) * std::sqrt(sinsqk);
      roots.push_back(s * (-cosk + rt3sink) - f[2] / 3.0);
      roots.push_back(s * (-cosk - rt3sink) - f[2] / 3.0);

      //We don't test if the polishing fails. Without established
      //bounds polishing is "hard" with multiple real roots, so we
      //just accept a polished root if it is available
      stator::numeric::halleys_method([&](double x){ return eval_derivatives<2>(f, x); }, roots[0]);
      stator::numeric::halleys_method([&](double x){ return eval_derivatives<2>(f, x); }, roots[1]);
      stator::numeric::halleys_method([&](double x){ return eval_derivatives<2>(f, x); }, roots[2]);
      
      std::sort(roots.begin(), roots.end());
      return roots;
    }
    /*! \endcond */

    namespace detail {
      /*! \brief Calculates the negative of the remainder of the
          division of \f$f(x)\f$ by \f$g(x)\f$.
       */
      template<size_t Order, class Real, char Letter>
      Polynomial<Order-2, Real, Letter> 
	mrem(const Polynomial<Order, Real, Letter>& f, const Polynomial<Order-1, Real, Letter>&g)
      {
	Polynomial<Order-2, Real, Letter> rem;
	std::tie(std::ignore, rem) = euclidean_division(f, g);
	return -rem;
      }

      /*! \brief A collection of Polynomials which form a Sturm chain. 
       */
      template<size_t Order, class Real, char Letter>
      struct SturmChain {
	/*! \brief Constructor if is the first Polynomial in the
            chain. 
	*/
	SturmChain(const Polynomial<Order, Real, Letter>& p_n):
	  _p_n(p_n), _p_nminus1(p_n, derivative(p_n, Variable<Letter>()))
	{}

	/*! \brief Constructor if is an intermediate Polynomial in the
            chain.
	*/
	SturmChain(const Polynomial<Order+1, Real, Letter>& p_nplus1, const Polynomial<Order, Real, Letter>& p_n):
	  _p_n(p_n), _p_nminus1(p_n, mrem(p_nplus1, p_n))
	{}

	Polynomial<Order, Real, Letter> _p_n;
	SturmChain<Order-1, Real, Letter> _p_nminus1;
	
	/*! Accessor function for the ith Polynomial in the Sturm
            chain.

	    This promotes the order of the Sturm chain polynomial to
	    the original order of the Polynomial as this is done at
	    runtime.
	*/
	Polynomial<Order, Real, Letter> get(size_t i) const {
	  if (i == 0)
	    return _p_n;
	  else
	    return _p_nminus1.get(i-1); 
	}
	
	/*! Count the number of sign changes in the Sturm chain
	  evaluated at \f$x\f$.
	  
	  This actually uses a helper function sign_change_helper to
	  carry out the calculation.
	*/
	template<class Real2>
	size_t sign_changes(const Real2& x) const {
	  return sign_change_helper(0, x);
	}

	template<class Real2>
	size_t roots(const Real2& a, const Real2& b) const {
	  return std::abs(int(sign_changes(a)) - int(sign_changes(b)));
	}
	
	
	/*! \brief Helper function for calculating the sign changes in
            the Sturm chain.

	    These functions use -1, 0, and +1 to denote the sign of an
	    evaluation of a polynomial. The sign of the previous
	    Polynomial in the Strum chain is given as last_sign. If
	    this is zero, then there has been no sign so far (all
	    previous polynomials were zero or this is the first
	    polynomial in the chain).
	 */
	template<class Real2>
	size_t sign_change_helper(const int last_sign, const Real2& x) const {
	  const Real currentx = eval(_p_n, Variable<Letter>() == x);
	  const int current_sign = (currentx != 0) * (1 - 2 * std::signbit(currentx));
	  const bool sign_change = (current_sign != 0) && (last_sign != 0) && (current_sign != last_sign);

	  const int next_sign = (current_sign != 0) ? current_sign : last_sign;
	  return _p_nminus1.sign_change_helper(next_sign, x) + sign_change;
	}

	void output_helper(std::ostream& os, const size_t max_order) const {
	  os << ",\n           p_" <<  max_order - Order << "=" << _p_n;
	  _p_nminus1.output_helper(os, max_order);
	}
      };

      /*! \brief Specialisation for a container holding the last Sturm
          chain Polynomial.
      */
      template<class Real, char Letter>
      struct SturmChain<0, Real, Letter> {
	/*! \brief Constructor  is the first and last Polynomial in
            the chain.
	*/
	SturmChain(const Polynomial<0, Real, Letter>& p_n):
	  _p_n(p_n)
	{}

	/*! \brief Constructor if this is the last Polynomial in the
            chain.
	*/
	SturmChain(const Polynomial<1, Real, Letter>& p_nplus1, const Polynomial<0, Real, Letter>& p_n):
	  _p_n(p_n) {}

	Polynomial<0, Real, Letter> get(size_t i) const {
	  if (i == 0)
	    return _p_n;
	  return Polynomial<0, Real, Letter>{};
	}

	template<class Real2>
	size_t sign_changes(const Real2& x) const {
	  return 0;
	}

	Polynomial<0, Real, Letter> _p_n;

	template<class Real2>
	size_t sign_change_helper(const int last_sign, const Real2& x) const {
	  const Real currentx = eval(_p_n, Variable<Letter>() == x);
	  const int current_sign = (currentx != 0) * (1 - 2 * std::signbit(currentx));
	  const bool sign_change = (current_sign != 0) && (last_sign != 0) && (current_sign != last_sign);
	  return sign_change;
	}
	
	void output_helper(std::ostream& os, const size_t max_order) const {
	  os << ",\n           p_" <<  max_order << "=" << _p_n;
	}
      };
      
      template<size_t Order, class Real, char Letter>
      std::ostream& operator<<(std::ostream& os, const SturmChain<Order, Real, Letter>& c) {
	os << "SturmChain{p_0=" << c._p_n;
	c._p_nminus1.output_helper(os, Order);
	os << "}";
	return os;
      }
    }

    /*! \brief Helper function to generate a SturmChain from a
        Polynomial.
      
      The actual calculation, storage, and evaluation of the Sturm
      chain is done by the detail::SturmChain type.

      The Sturm chain is a sequence of polynomials \f$p_0(x)\f$,
      \f$p_1(x)\f$, \f$p_2(x)\f$, \f$\ldots\f$, \f$p_n(x)\f$ generated
      from a single polynomial \f$f(x)\f$ of order \f$n\f$. The first
      two Sturm chain polynomials are given as
      
      \f{eqnarray*}{
      p_0(x) &=& f(x)\\
      p_1(x) &=& f'(x)
      \f}

      All higher polynomials are evaluated like so:

      \f{eqnarray*}{
      p_n(x) &=& -\mathrm{rem}(p_{n+2}, p_{n+1})
      \f}
      
      where \f$\mathrm{rem}(p_{n+2},\,p_{n+1})\f$ returns the
      remainder polynomial from a \ref euclidean_division of
      \f$p_{n+2}\f$ over \f$p_{n+1}\f$. This sequence terminates at
      \f$p_N\f$, where \f$N\f$ is the order of the original
      Polynomial, \f$f(x)\f$.
      
      The interesting property of this chain is that it allows a
      calculation of the count of distinct real roots of \f$f(x)\f$
      within a certain range. If we evaluate the Sturm chain of
      \f$f(x)\f$ at a point \f$\xi\f$ and count the number of changes
      in sign (ignoring zeros) in the sequence:
      
      \f[
      p_0(\xi),\,p_1(\xi),\,\ldots p_n(\xi)
      \f]
      
      and define this as \f$\sigma(\xi)\f$. Then, given two real
      numbers \f$a<b\f$, the number of distinct roots of \f$f(x)\f$ in
      \f$(a,b]\f$ is given by \f$\sigma(a)-\sigma(b)\f$.

      This gives a method to calculate the exact number of real and
      distinct roots in a region, which can then be used in a
      bisection routine to bound individual distinct roots. The Sturm
      sequence can also be easily evaluated at infinite bounds
      \f$(-\infty,+\infty)\f$ to determine the total number of real
      roots.

      Although this method allows the construction of an
      arbitrary-order Polynomial root finder through bisection,
      inexact methods for computing the number of roots in a region
      (such as \ref descartes_rule_of_signs, used in \ref
      VAS_real_root_bounds) are preferred as they are more
      computationally efficient.
    */
    template<size_t Order, class Real, char Letter>
    detail::SturmChain<Order, Real, Letter> sturm_chain(const Polynomial<Order, Real, Letter>& f) {
      return detail::SturmChain<Order, Real, Letter>(f);
    }

    /*! \brief Calculates an upper bound estimate for the number of
      positive real roots of a Polynomial (including multiples).
      
      Descarte's rule of signs states that the number of positive real
      roots for a single-variable real-coefficient Polynomial is less
      than or equal to the number of sign changes between consecutive
      non-zero coefficients in the Polynomial. When the actual root
      count is less, it is less by an even number. Therefore, the
      values 0 or 1 are exact.
     */
    template<size_t Order, class Real, char Letter>
    size_t descartes_rule_of_signs(const Polynomial<Order, Real, Letter>& f) {
      //Count the sign changes
      size_t sign_changes(0);
      int last_sign = 0;      
      for (size_t i(0); i <= Order; ++i) {
	const int current_sign = (f[i] != 0) * (1 - 2 * std::signbit(f[i]));
	sign_changes += (current_sign != 0) && (last_sign != 0) && (current_sign != last_sign);
	last_sign = (current_sign != 0) ? current_sign : last_sign;
      }
      return sign_changes;
    }

    /*! \brief Budan's upper bound estimate for the number of real
        roots in a Polynomial over the range \f$(0,\,1)\f$.

	Budan's test is actually just Descarte's test, but on a
	transformed Polynomial \f$p(x)\f$, which is related to
	\f$f(x)\f$ as follows:
	
	\f[
	p(x) = \left(x+1\right)^d\,f\left(\frac{1}{x+1}\right)
	\f]

	where \f$d\f$ is the order of the polynomial \f$f(x)\f$. The
	roots of \f$f(x)\f$ in the range \f$[0,1]\f$ are mapped over
	the range \f$[0,\infty]\f$ of \f$p(x)\f$. This allows
	Descarte's rule of signs to be applied to a limited range of
	the original polynomial.

	The actual transformation to \f$p(x)\f$ is carried out using
	the \ref invert_taylor_shift function, before this is passed
	to \ref descartes_rule_of_signs.

	\return An upper bound on the number of real roots in the
	interval \f$(0,\,1)\f$.
     */
    template<size_t Order, class Real, char Letter>
    size_t budan_01_test(const Polynomial<Order, Real, Letter>& f) {
      return descartes_rule_of_signs(invert_taylor_shift(f));
    }

    /*! \brief Alesina-Galuzzi upper bound estimate for the numer of
        real roots in a Polynomial over a specified range
        \f$(a,\,b)\f$.

	This a generalisation of Budan's 01 test (see \ref
	budan_01_test) and is implemented that way. The polynomial is
	shifted so that \f$0\f$ corresponds to \f$a\f$, then scaled so
	that \f$x=1\f$ corresponds to \f$b\f$, before Budan's test is
	called on the transformed Polynomial.
    */
    template<size_t Order, class Real, char Letter>
    size_t alesina_galuzzi_test(const Polynomial<Order, Real, Letter>& f, const Real& a, const Real& b) {
      return budan_01_test(scale_poly(shift_function(f, a), b - a));
    }
    
    /*! \brief Local-max Quadratic upper bound estimate for the value
        of the real roots of a Polynomial.

	This function is adapted from the thesis "Upper bounds on the
	values of the positive roots of polynomials" by Panagiotis
	S. Vigklas (2010). The main change is to generalise to
	arbitrary sign on the highest order coefficient, and to allow
	high-order coefficients with zero values.
     */
    template<class Real, size_t Order, char Letter>
    Real LMQ_upper_bound(const Polynomial<Order, Real, Letter>& f) {
      std::array<size_t, Order+1> times_used;
      times_used.fill(1);
      Real ub = Real();

      size_t real_order = Order;
      while ((real_order > 0) && (f[real_order] == 0))
	--real_order;

      for (int m(real_order-1); m >= 0; --m)
	if (std::signbit(f[m]) != std::signbit(f[real_order])) {
	  Real tempub = std::numeric_limits<Real>::infinity();
	  for (int k(real_order); k > m; --k)
	    if (std::signbit(f[k]) != std::signbit(f[m]))
	      {
		Real temp = std::pow(-(1 << times_used[k]) * f[m] / f[k], 1.0 / (k - m));
		++times_used[k];
		tempub = std::min(temp, tempub);
	      }
	  ub = std::max(tempub, ub);
	}
      return ub;
    }

    /*! \brief Local-max Quadratic lower bound estimate for the real
      roots of a Polynomial.
      
      Given a Polynomial \f$f(x)\f$, this function performs the
      transformation:
      
      \f[
      g(x) = x^n\,f(\frac{1}{x})
      \f]
      
      Now the upper bound on the real roots of \f$g(x)\f$ are the
      inverse of the lower bound on the real roots of \f$f(x)\f$. The
      transformation is computationally equivalent to reversing the
      coefficient array of the polynomial \f$f(x)\f$. This function is
      adapted from the thesis "Upper bounds on the values of the
      positive roots of polynomials" by Panagiotis S. Vigklas (2010).
    */
    template<class Real, size_t Order, char Letter>
    Real LMQ_lower_bound(const Polynomial<Order, Real, Letter>& f) {
      return 1.0 / LMQ_upper_bound(Polynomial<Order, Real, Letter>(f.rbegin(), f.rend()));
    }

    /*! \cond Specializations

      \brief Specialisation of Local-max Quadratic upper-bound
      estimation for real roots of a Polynomial, where the Polynomial
      is a constant.
    */
    template<class Real, char Letter>
    Real LMQ_upper_bound(const Polynomial<0, Real, Letter>& f) {
      return 0;
    }

    /*!
      \brief Specialisation of Local-max Quadratic
      lower-bound estimation for real roots of a Polynomial, where the
      Polynomial is a constant.
    */
    template<class Real, char Letter>
    Real LMQ_lower_bound(const Polynomial<0, Real, Letter>& f) {
      return HUGE_VAL;
    }

    /*! \brief Calculate interval bounds on all of the positive real
      roots between \f$(0,1)\f$ of a squarefree Polynomial.
      
      This function uses the VCA algorithm to bound the roots. It
      assumes that the polynomial has a non-zero constant term and
      leading order coefficient term.
    */
    template<size_t Order, class Real, char Letter>
    StackVector<std::pair<Real,Real>, Order> 
    VCA_real_root_bounds_worker(const Polynomial<Order, Real, Letter>& f) {
      //Test how many roots are in the range (0,1)
      switch (budan_01_test(f)) {
      case 0:
	//No roots, return empty
	return StackVector<std::pair<Real,Real>, Order>();
      case 1:
	//One root! the bound is (0,1)
	return StackVector<std::pair<Real,Real>, Order>{std::make_pair(Real(0), Real(1))};
      default:
	//Possibly multiple roots, so divide the range [0,1] in half
	//and recursively explore it.

	//We first scale the polynomial so that the original range
	//x=[0,1] is scaled to the range x=[0,2]. To do this, we
	//perform the subsitution x->x/2. Rather than use division
	//(even though a division by 2 usually cheap), we actually
	//generate this function \f$p1(x) = 2^{Order}\,f(x/2)\f$. This
	//transforms this to a multiplication op which is always cheap.
	Polynomial<Order, Real, Letter> p1(f);
	for (size_t i(0); i <= Order; ++i)
	  p1[i] *= (1 << (Order-i)); //This gives (2^Order) / (2^i)

	//Perform a Taylor shift p2(x) = p1(x+1). This gives 
	//
	//p2(x) = 2^Order f(x/2 + 0.5) 
	//
	//in terms of the original function, f(x).
	const Polynomial<Order, Real, Letter> p2 = shift_function(p1, Unity());

	//Now that we have two scaled and shifted polynomials where
	//the roots in f in the range x=[0,0.5] are in p1 over the
	//range \f$x=[0,1]\f$, and the roots of f in the range
	//x=[0.5,1.0] are in p2 over the range \f$x=[0,1]\f$. Search
	//them both and combine the results.

	auto retval = VCA_real_root_bounds_worker(p1);
	//Scale these roots back to the original mapping
	for (auto& root_bound : retval) {
	  root_bound.first /= 2;
	  root_bound.second /= 2;
	}

	auto second_range = VCA_real_root_bounds_worker(p2);
	//Scale these roots back to the original mapping
	for (auto& root_bound : second_range)
	  retval.push_back(std::make_pair(root_bound.first / 2 + 0.5, root_bound.second / 2 + 0.5));
	
	return retval;
      }
    }

    /*! \endcond */

    /*! \brief Calculate bounds on all of the positive real roots of a
        Polynomial.

	This function uses the VCA algorithm to bound the roots and
	assumes the polynomial has non-zero constant and leading order
	coefficients. This function enforces these requirements and
	then simply scales the Polynomial so that all roots lie in the
	range (0,1) before passing it to \ref VCA_real_root_bounds_worker.
     */
    template<size_t Order, class Real, char Letter>
    StackVector<std::pair<Real,Real>, Order> 
    VCA_real_root_bounds(const Polynomial<Order, Real, Letter>& f) {
      //Calculate the upper bound on the polynomial real roots, and
      //return if no roots are detected
      const Real upper_bound = LMQ_upper_bound(f);
      if (upper_bound == 0)
	return StackVector<std::pair<Real,Real>, Order>();
      
      //Scale the polynomial so that all roots lie in the region
      //(0,1), then solve for its roots
      auto bounds = VCA_real_root_bounds_worker(scale_poly(f, upper_bound) );

      //finally, we must undo the scaling on the roots found
      for (auto& bound : bounds) {
	bound.first *= upper_bound;
	bound.second *= upper_bound;
      }
      
      return bounds;
    }

    /*! \brief Class representing the current Mobius transformation
        applied to a Polynomial.
	
	This class is used in the implementation of the
	VAS_real_root_bounds_worker function. A mobius transformation is
	the following function:
	
	\$
	M(x) = \frac{a\,x+b}{c\,x+d}
	\$

	This allows a polynomial to be transformed and (provided the
	Mobius transformation has the same operations applied to it)
	we can map between the original polynomial x and the
	transformed x.
     */
    template<class Real>
    struct MobiusTransform: public std::array<Real, 4> {
      typedef std::array<Real, 4> Base;
      /*! \brief Constructor for the Mobius transformation.
       */
      MobiusTransform(Real a, Real b, Real c, Real d):
	Base{{a,b,c,d}}
      {}
      
      /*! \brief Evaluate the Mobius transformation at the transformed
          \f$x\f$.
       */
      Real eval(Real x) {
	//Catch the special case that there is no x term
	if (((*this)[0] == 0) && ((*this)[2] == 0))
	  return (*this)[1] / (*this)[3];

	//Special case of inf/inf
	if (std::isinf(x) && ((*this)[0] != 0) && ((*this)[2] != 0))
	  return (*this)[0] / (*this)[2];

	//We have to be careful not to do 0 * inf.
	Real numerator = (*this)[1];
	if ((*this)[0] != 0)
	  numerator += x * (*this)[0];
	
	Real denominator = (*this)[3];
	if ((*this)[2] != 0)
	  denominator += x * (*this)[2];
	
	return numerator / denominator;
      }

      /*!\brief Add the effect of a \ref shift_poly operation
         to the Mobius transform.
       */
      void shift(Real x) {
	(*this)[1] += (*this)[0] * x;
	(*this)[3] += (*this)[2] * x;
      }

      /*!\brief Add the effect of a \ref scale_poly operation to the
         Mobius transform.
       */
      void scale(Real x) {
	(*this)[0] *= x;
	(*this)[2] *= x;
      }
      
      /*!\brief Add the effect of a \ref scale_poly operation to the
         Mobius transform.
       */
      void invert_taylor_shift() {
	Base::operator=(std::array<Real, 4>{{(*this)[1], (*this)[0] + (*this)[1], (*this)[3], (*this)[2] + (*this)[3]}});
      }
    };

    /*! \brief Calculate bounds on all of the positive real roots in
      the range of a Polynomial.

      This function uses the VAS algorithm to bound the roots and
      assumes the polynomial has non-zero constant and leading order
      coefficients.
      
      The parameter M is an array of Mobius transformation
      coefficients {a,b,c,d}. By default it is a direct mapping to the
      original Polynomial.
     */
    template<size_t Order, class Real, char Letter>
    StackVector<std::pair<Real,Real>, Order> 
    VAS_real_root_bounds_worker(Polynomial<Order, Real, Letter> f, MobiusTransform<Real> M) {
      //This while loop is only used to allow restarting the method
      //without recursion, as this may recurse a large number of
      //times.
      while (true) {
	//Test how many positive roots exist
	const size_t sign_changes = descartes_rule_of_signs(f);
	
	if (sign_changes == 0)
	  //No roots, return empty
	  return StackVector<std::pair<Real,Real>, Order>();
	
	if (sign_changes == 1)
	  //One root! the bounds are M(0) and M(\infty)
	  return StackVector<std::pair<Real,Real>, Order>{std::make_pair(M.eval(0), M.eval(HUGE_VAL))};
	
	auto lb = LMQ_lower_bound(f);
	
	//Attempt to divide the polynomial range from [0, \infty] up
	//into [0, 1] and [1, \infty].
	
	//If there's a large jump in the lower bound, then this will
	//take too long to converge. Try rescaling the polynomial. The
	//factor 16 is empirically selected.
	if (lb >= 16) {
	  f = scale_poly(f, lb);
	  M.scale(lb);
	  lb = 1;
	}

	//Check if the lower bound suggests that this split is a waste
	//of time (no roots in [0,1]), if so, shift the polynomial and
	//try again. This also occurs if there was a large jump in the
	//lower bound as detected above.
	if (lb >= 1) {
	  f = shift_function(f, lb);
	  M.shift(lb);
	  continue; //Start again
	}

	if (std::abs(eval(f, Variable<Letter>() == 1.0)) <= (100 * precision(f, 1.0))) {
	  //There is probably a root near x=1.0 as its approached zero
	  //closely (compared to the precision of the polynomial
	  //evaluation). Rather than trying to divide it out or do
	  //anything too smart, just scale the polynomial so that next
	  //time it falls at x=0.5.
	  //
	  //This is needed as we are using finite precision math
	  //(floating point)
	  const Real scale = 2.0;
	  f = scale_poly(f, scale);
	  M.scale(scale);
	  continue; //Start again
	}

	//Check for a root at x=0. If there is one, divide it out!
	StackVector<std::pair<Real,Real>, Order> retval;
	if (f[0] == 0) {
	  retval.push_back(std::make_pair(M.eval(0), M.eval(0)));
	  f = deflate_polynomial(f, Null());
	}

	//Create and solve the polynomial for [0, 1]
	Polynomial<Order, Real, Letter> p01 = invert_taylor_shift(f);
	auto M01 = M;
	M01.invert_taylor_shift();
	auto first_range = VAS_real_root_bounds_worker(p01, M01);
	for (const auto& bound: first_range)
	  retval.push_back(bound);
	
	//Create and solve the polynomial for [1, \infty]
	Polynomial<Order, Real, Letter> p1inf = shift_function(f, Unity());
	auto M1inf = M;
	M1inf.shift(1);
	auto second_range = VAS_real_root_bounds_worker(p1inf, M1inf);
	for (const auto& bound: second_range)
	  retval.push_back(bound);

	return retval;
      }
    }
    
    template<class Real, char Letter>
    StackVector<std::pair<Real,Real>, 1> 
    VAS_real_root_bounds_worker(const Polynomial<0, Real, Letter>& f, MobiusTransform<Real> M) {
      return StackVector<std::pair<Real,Real>, 1>();
    }

    /*! \endcond */
    
    /*! \brief Calculate bounds on all of the positive real roots of a
        Polynomial.
    
    	This function uses the VAS algorithm to bound the roots and
    	assumes the polynomial has non-zero constant and leading order
    	coefficients. This function enforces these conditions before
    	passing it to \ref VAS_real_root_bounds_worker.
     */
    template<size_t Order, class Real, char Letter>
    StackVector<std::pair<Real,Real>, Order> 
    VAS_real_root_bounds(const Polynomial<Order, Real, Letter>& f) {
      //Calculate the upper bound on the polynomial real roots, and
      //return if no roots are detected
      const Real upper_bound = LMQ_upper_bound(f);
      if (upper_bound == 0)
    	return StackVector<std::pair<Real,Real>, Order>();
      
      auto bounds = VAS_real_root_bounds_worker(f, MobiusTransform<Real>(1,0,0,1));
      for (auto& bound: bounds) {
	//Sort the bound values correctly
	if (bound.first > bound.second)
	  std::swap(bound.first, bound.second);
	//Replace infinite bounds with the upper bound estimate
	if (std::isinf(bound.second))
	  bound.second = upper_bound;
      }

      return bounds;
    }
    
    /*! \brief Enumeration of the types of root bounding methods we
        have for \ref solve_real_roots. 
    */
    enum class  PolyRootBounder {
      VCA, VAS, STURM
    };

    /*! \brief Enumeration of the types of bisection routines we have
        for \ref solve_real_roots. */
    enum class  PolyRootBisector {
      BISECTION, TOMS748
    };

    /*! \brief Solve for a quadratic factor of a Polynomial using the LinBairstow method.

      This function is part of a generic polynomial root finding
      technique. It returns a quadratic factor of the passed
      polynomial. The tolerance is the convergence criterion on the
      coefficients of the factor.
     */
    template<size_t Order, class Real, char Letter>
    Polynomial<2, Real, Letter>
    LinBairstowSolve(Polynomial<Order, Real, Letter> f, Real tolerance) {
      if (f[Order] == Real()) 
	return LinBairstowSolve(change_order<Order-1>(f), tolerance);

      size_t nIterations = 0;
      Eigen::Matrix<Real, 2, 1> dFactor{0, 0};

      //We're trying to find the quadratic factor that divides the
      //polynomial and leaves no remainder.:
      Polynomial<2, Real, Letter> factor{0,0,1};

      //Now loop solving for the quadratic factor
      do {
	++nIterations;
	  
	Polynomial<2, Real, Letter> rem1, rem2;
	Polynomial<Order, Real, Letter> p1;
	//Determine the error (the actual remainder)
	std::tie(p1, rem1) = euclidean_division(f, factor);

	//Also determine the gradient of the error with respect to the
	//lowest order coefficients of the factor. These are related
	//to the remainder of a second division
	std::tie(std::ignore, rem2) = euclidean_division(p1, factor);
	  
	//Perform a Newton-Raphson step in the remainder, while varying the factor
	Eigen::Matrix<Real, 2, 1> F{rem1[0], rem1[1]}; //{S, R}	
	Eigen::Matrix<Real, 2, 2> J{-rem2[0], factor[0] * rem2[1], -rem2[1], factor[1]*rem2[1] - rem2[0]}; //{dS/dC, dS/dB, dR/dC, dR/dB}
	  
	//New factor is calculated
	dFactor = J.inverse() * (-F);
	factor[0] = factor[0] + dFactor[0];
	factor[1] = factor[1] + dFactor[1];
      } while (dFactor.nrm2() > tolerance*tolerance);
      
      return factor;
    }

    template<class Real, char Letter>
    Polynomial<2, Real, Letter>
    LinBairstowSolve(Polynomial<2, Real, Letter> f, Real tolerance) {
      return f;
    }

    template<class Real, char Letter>
    Polynomial<1, Real, Letter>
    LinBairstowSolve(Polynomial<1, Real, Letter> f, Real tolerance) {
      return f;
    }

    /*! \brief Determine the positive real roots of a polynomial using
        bisection and Sturm chains.
	
     */
    template<class Real, size_t Order, char Letter>
    StackVector<Real, Order>
    solve_real_positive_roots_poly_sturm(const Polynomial<Order, Real, Letter>& f, const size_t tol_bits=56) {
      //Establish bounds on the positive roots
      const Real max = LMQ_upper_bound(f);
      const Real min = LMQ_lower_bound(f);
      
      //If there are no roots, then return
      if (min > max) return StackVector<Real, Order>();
      
      //Construct the Sturm chain and bisect it
      auto chain = sturm_chain(f);
      
      StackVector<std::tuple<Real,Real,size_t>, Order> regions{std::make_tuple(min, max, chain.roots(min, max))};
      StackVector<Real, Order> retval;
      
      const Real eps = std::max(Real(std::ldexp(1.0, 1-tol_bits)), Real(2*std::numeric_limits<Real>::epsilon()));

      bool try_toms748 = true;
      while(!regions.empty()) {
	auto range = regions.pop_back();
	const Real xmin = std::get<0>(range); 
	const Real xmax = std::get<1>(range); 
	const size_t roots = std::get<2>(range);

	Real xmid = (xmin + xmax) / 2;

	//Check if we have reached the tolerance of the calculations via Sturm bisection
	if (std::abs(xmin - xmax) <= (eps * std::min(std::abs(xmin), std::abs(xmax)))) {
	  for (size_t i(0); i < roots; ++i)
	    retval.push_back(xmid);
	  try_toms748 = true;
	  continue;
	}
	
	size_t rootsa = chain.roots(xmin, xmid);
	size_t rootsb = chain.roots(xmid, xmax);

	if ((rootsa + rootsb) != roots) {
	  //A precision error of the calculations has caused us to
	  //lose track of the roots. This may be caused by us dropping
	  //xmid exactly on a root.

	  //Try shifting where we bisected the range
	  xmid = (xmid + xmax) / 2;
	  rootsa = chain.roots(xmin, xmid);
	  rootsb = chain.roots(xmid, xmax);
	  if ((rootsa + rootsb) != roots) {
	    //That didn't work. Rather than abort, assume this is a
	    //precision error and there are roots somewhere in
	    //xmin/xmax. Return this as a best estimate
	    retval.push_back((xmin + xmax) / 2);
	    continue;
	  }
	}

	if (rootsa > 1)
	  regions.push_back(std::make_tuple(xmin, xmid, rootsa));
	else if (rootsa == 1) {
	  if (try_toms748){
	    try {
	      boost::uintmax_t iter = 100;
	      auto root = boost::math::tools::toms748_solve([&](Real x) { return eval(f, Variable<Letter>() == x); }, xmin, xmid, boost::math::tools::eps_tolerance<Real>(100), iter);
	      retval.push_back((root.first + root.second) / 2);
	    } catch(...) {
	      regions.push_back(std::make_tuple(xmin, xmid, rootsa));
	      try_toms748 = false;
	    }
	  } else {
	    regions.push_back(std::make_tuple(xmin, xmid, rootsa));	    
	  }
	}
	if (rootsb > 1)
	  regions.push_back(std::make_tuple(xmid, xmax, rootsb));
	else if (rootsb == 1) {
	  if (try_toms748){
	    try {
	      boost::uintmax_t iter = 100;
	      auto root = boost::math::tools::toms748_solve([&](Real x) { return eval(f, Variable<Letter>() == x); }, xmid, xmax, boost::math::tools::eps_tolerance<Real>(100), iter);
	      retval.push_back((root.first + root.second) / 2);
	    } catch(...) {
	      regions.push_back(std::make_tuple(xmid, xmax, rootsb));
	      try_toms748 = false;
	    }
	  } else {
	    regions.push_back(std::make_tuple(xmid, xmax, rootsb));
	  }
	}
      }
	
      return retval;
    }

    /*! \brief Iterative solver for the real roots of a square-free
        Polynomial.

	This function assumes that the polynomial has non-zero high
	and constant coefficients.
     */
    template<PolyRootBounder BoundMode, PolyRootBisector BisectionMode, size_t Order, class Real, char Letter>
    StackVector<Real, Order>
    solve_real_positive_roots_poly(const Polynomial<Order, Real, Letter>& f) {
      StackVector<std::pair<Real,Real>, Order> bounds;

      switch (BoundMode) {
      case PolyRootBounder::STURM: return solve_real_positive_roots_poly_sturm(f); break;
      case PolyRootBounder::VCA: bounds = VCA_real_root_bounds(f); break;
      case PolyRootBounder::VAS: bounds = VAS_real_root_bounds(f); break;
      }
            
      //Now bisect to calculate the roots to full precision
      StackVector<Real, Order> retval;
      
      for (const auto& bound : bounds) {
	const Real& a = bound.first;
	const Real& b = bound.second;
	switch(BisectionMode) {
	case PolyRootBisector::BISECTION: 
	  {
	    boost::uintmax_t iter = 100;
	    auto root = boost::math::tools::bisect([&](Real x) { return eval(f, Variable<Letter>() == x); }, a, b, boost::math::tools::eps_tolerance<Real>(100), iter);
	    retval.push_back((root.first + root.second) / 2);
	    break;
	  }
	case PolyRootBisector::TOMS748: 
	  {
	    boost::uintmax_t iter = 100;
	    auto root = boost::math::tools::toms748_solve([&](Real x) { return eval(f, Variable<Letter>() == x); }, a, b, boost::math::tools::eps_tolerance<Real>(100), iter);
	    retval.push_back((root.first + root.second) / 2);
	    break;
	  }
	}
      }

      std::sort(retval.begin(), retval.end());
      return retval;
    }
    
    /*\brief Solve for the distinct real roots of a Polynomial.

      This is a general implementation of the polynomial real root
      solver. It defaults to using the VAS algorithm, and polishing up
      the roots using the TOMS748 algorithm. It will recurse and call
      the default root solver if the polynomial has a zero
      leading-order coefficient!

      Roots are always returned sorted lowest-first.
     */
    template<PolyRootBounder BoundMode = PolyRootBounder::STURM, PolyRootBisector BisectionMode = PolyRootBisector::TOMS748, size_t Order, class Real, char Letter>
    StackVector<Real, Order>
    solve_real_roots(const Polynomial<Order, Real, Letter>& f) {
      //Handle special cases 
      //The constant coefficient is zero: deflate the polynomial
      if (f[0] == Real())
	return solve_real_roots(deflate_polynomial(f, Null()));
      
      //The highest order coefficient is zero: drop to lower order
      //solvers
      if (f[Order] == Real())
	return solve_real_roots(change_order<Order-1>(f));

      auto roots = solve_real_positive_roots_poly<BoundMode, BisectionMode, Order, Real, Letter>(f);
      auto neg_roots = solve_real_positive_roots_poly<BoundMode, BisectionMode, Order, Real, Letter>(reflect_poly(f));

      //We need to flip the sign on the negative roots
      for (const auto& root: neg_roots)
	roots.push_back(-root);
      
      std::sort(roots.begin(), roots.end());
      return roots;
    }
    /*! \endcond \} */

  } // namespace symbolic
} // namespace stator
