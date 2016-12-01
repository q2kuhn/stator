/*
  Copyright (C) 2015 Marcus Bannerman <m.bannerman@gmail.com>

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


//C++
#include <iostream>
#include <vector>
#include <cmath>
#include <complex>

//stator
#include <stator/symbolic/symbolic.hpp>

//boost
#define BOOST_TEST_MODULE Polynomial_test
#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/assert.hpp>

#include <random>

std::mt19937 RNG;
std::normal_distribution<double> normal_dist(0, 1);
std::uniform_real_distribution<double> angle_dist(0, std::atan(1)*4);

typedef Eigen::Matrix<double, 3, 1, Eigen::DontAlign> Vector;

Vector random_unit_vec() {
  Vector vec{normal_dist(RNG), normal_dist(RNG), normal_dist(RNG)};
  return vec.normalized();
}

bool err(double val, double expected)
{
  return std::abs(val / expected - 1) < 0.0001;
}

template<class T1, class T2>
bool compare_expression(const T1& f, const T2& g) {
  using namespace sym;
  std::ostringstream os;
  os << f;
  std::string f_str = os.str();
  os.str(""); os.clear();
  os << g;
  std::string g_str = os.str();
  if (!(f_str == g_str))
    std::cerr << f << " != " << g << std::endl;
  return f_str == g_str;
}

#define CHECK_TYPE(EXPRESSION, TYPE) \
  BOOST_MPL_ASSERT_MSG((std::is_same<decltype(EXPRESSION), TYPE>::value), TYPE_COMPARE, (decltype(EXPRESSION), TYPE));

struct RootData {
  RootData() : multiplicity(0) {}
  std::vector<double> matched_roots;
  size_t multiplicity;
};

template<class Real1, class Real2, size_t N, class PolyVar>
sym::Polynomial<N, Real1, PolyVar> convert_poly(const sym::Polynomial<N, Real2, PolyVar>& p) {
  sym::Polynomial<N, Real1, PolyVar> f;
  for (size_t i(0); i <= N; ++i)
    f[i] = Real1(p[i]);
  return f;
}

template<class T1, class T2, class Func>
void compare_roots(T1 roots, T2 actual_roots, Func f){
  std::sort(roots.begin(), roots.end());
  std::sort(actual_roots.begin(), actual_roots.end());

  //Push all real roots on to a map
  std::map<double, RootData> root_data;
  for (auto root: actual_roots)
    root_data[root].multiplicity += 1;

  bool extra_root = false;
  for (auto root: roots) {
    bool found = false;
    for (auto& test_root: root_data) {
      const double root_error = std::abs((root - test_root.first) / (test_root.first + (test_root.first == 0)));
      if (root_error < 0.00124) {
	found = true;
	test_root.second.matched_roots.push_back(root);
	break;
      }
    }
    if (!found) extra_root = true;
  }

  //Check all roots of odd multiplicity have been detected
  bool missed_root = false;
  for (auto test_root: root_data)
    if (test_root.second.multiplicity % 2)
      if (test_root.second.matched_roots.empty()) {
	missed_root = true;
	break;
      }

  if (extra_root || missed_root) {
    std::ostringstream os;
    os.precision(10);
    os << "\n";
    if (extra_root)
      os << "Unmatched root, ";
    if (missed_root)
      os << "Missing root";

    os << "\n f     = " << f;
    os << "\n actual_roots = " << actual_roots;
    os << "\n detected_roots = " << roots;
    os << "\n Matching data:";
    for (auto test_root: root_data)
      os << "\n   " << test_root.first << ", multiplicity=" << test_root.second.multiplicity << ", matches=" <<  test_root.second.matched_roots.size();
    BOOST_ERROR(os.str());
  }
}


BOOST_AUTO_TEST_CASE( poly_variables )
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  Polynomial<1,double, Var<vidx<'y'> > > y{0, 1};
  BOOST_CHECK(compare_expression(substitution(y, Var<vidx<'y'> >()==Var<vidx<'x'> >()), "P(1 × x)"));

  BOOST_CHECK(compare_expression(expand(x * x * x), "P(1 × x³)"));
  BOOST_CHECK(compare_expression(expand(y * y * y), "P(1 × y³)"));
  BOOST_CHECK(compare_expression(substitution(expand(y * y * y), Var<vidx<'y'> >()==Var<vidx<'x'> >()), "P(1 × x³)"));
}

BOOST_AUTO_TEST_CASE( poly_addition )
{
  using namespace sym;
  Polynomial<1> x{0, 2.5};
  Polynomial<0> C{0.3};
  auto poly1 = expand(x+C);
  BOOST_CHECK_EQUAL(poly1[0], 0.3);
  BOOST_CHECK_EQUAL(poly1[1], 2.5);

  poly1 = expand(C+x);
  BOOST_CHECK_EQUAL(poly1[0], 0.3);
  BOOST_CHECK_EQUAL(poly1[1], 2.5);

  auto poly2 = expand(x + 0.3);
  BOOST_CHECK_EQUAL(poly2[0], 0.3);
  BOOST_CHECK_EQUAL(poly2[1], 2.5);

  poly2 = expand(0.3 + x);
  BOOST_CHECK_EQUAL(poly2[0], 0.3);
  BOOST_CHECK_EQUAL(poly2[1], 2.5);
}

BOOST_AUTO_TEST_CASE( poly_multiplication )
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  auto poly1 = -2.0;
  auto poly2 = 2.0 - x + x * x;
  auto poly3 = expand(poly2 * poly1);
  BOOST_CHECK_EQUAL(poly3[0], -4);
  BOOST_CHECK_EQUAL(poly3[1], +2);
  BOOST_CHECK_EQUAL(poly3[2], -2);

  CHECK_TYPE((Null() * Polynomial<2>{1,2,3}), Null);
  CHECK_TYPE((Polynomial<2>{1,2,3}) * Null(), Null);

  static_assert(std::is_same<decltype(Null() * Polynomial<2>{1,2,3}), Null>::value, "Null multiply is not cancelling the polynomial");
  static_assert(std::is_same<decltype(Polynomial<2>{1,2,3} * Null()), Null>::value, "Null multiply is not cancelling the polynomial");
  
  CHECK_TYPE(simplify(Unity() * Polynomial<2>{1,2,3}), Polynomial<2>);
  CHECK_TYPE(simplify(Polynomial<2>{1,2,3} * Unity()), Polynomial<2>);
}

BOOST_AUTO_TEST_CASE( poly_division )
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  auto poly1 = 2.0 - x + x * x;
  auto poly2 = expand(poly1 / 0.5);
  BOOST_CHECK_EQUAL(poly2[0], 4);
  BOOST_CHECK_EQUAL(poly2[1], -2);
  BOOST_CHECK_EQUAL(poly2[2], 2);

  static_assert(std::is_same<decltype(simplify(Polynomial<2>{1,2,3} / Unity())), Polynomial<2> >::value, "Unity division is not returning the polynomial");
}

BOOST_AUTO_TEST_CASE( poly_vector )
{
  using namespace sym;
  Polynomial<1, Vector> x{Vector{0,0,0}, Vector{1,2,3}};
  Polynomial<0, Vector> C{Vector{3,2,1}};
  auto poly1 = expand(x+C);
  BOOST_CHECK_EQUAL(poly1[0], (Vector{3,2,1}));
  BOOST_CHECK_EQUAL(poly1[1], (Vector{1,2,3}));

  auto poly2 = expand(dot(poly1, poly1));
  BOOST_CHECK_EQUAL(poly2[0], 14);
  BOOST_CHECK_EQUAL(poly2[1], 20);
  BOOST_CHECK_EQUAL(poly2[2], 14);
}

BOOST_AUTO_TEST_CASE( poly_lower_order )
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  Polynomial<2> poly2 = expand(2.0 - x + x * x);
  Polynomial<3> poly3 = expand(poly2 + 0 * x * x * x);
  //Try to cast down one level as the highest order coefficient is zero
  BOOST_CHECK(poly3[3] == 0);
  Polynomial<2> poly4(change_order<2>(poly3));

  BOOST_CHECK_EQUAL(poly4[0], 2);
  BOOST_CHECK_EQUAL(poly4[1], -1);
  BOOST_CHECK_EQUAL(poly4[2], 1);
  BOOST_CHECK_EQUAL(substitution(poly3, Var<vidx<'x'> >()==123), substitution(poly4, Var<vidx<'x'> >()==123));
}

BOOST_AUTO_TEST_CASE( poly_simplify )
{
  using namespace sym;
  Var<vidx<'x'> > x;
  //Test that simplify creates polynomials from Vars
  auto poly1 = expand(2 * x * x);
  BOOST_CHECK_EQUAL(poly1[0], 0);
  BOOST_CHECK_EQUAL(poly1[1], 0);
  BOOST_CHECK_EQUAL(poly1[2], 2);

  Var<vidx<'y'> > y;
  //Check that Polynomial simplifications exist for these functions
  expand(y+y);
  expand(y+y+y);
  expand(y*y*2);
  expand(y*y*2+1+2*x+2-12*x*x);

  //Check expansion
  {
    Polynomial<1> x{0,1};
    BOOST_CHECK(compare_expression(expand(pow<3>(x+2)), expand((x+2) * (x+2) * (x+2))));;
  }
}

BOOST_AUTO_TEST_CASE( poly_eval_limits )
{
  using namespace sym;
  Polynomial<1> x{0, 1};

  {//Check even positive polynomials
    auto f = expand(x * x - x + 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==0), 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==+HUGE_VAL), +HUGE_VAL);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==-HUGE_VAL), +HUGE_VAL);
  }

  {//Check even negative polynomials
    auto f = expand(-x * x + x + 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==0), 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==+HUGE_VAL), -HUGE_VAL);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==-HUGE_VAL), -HUGE_VAL);
  }

  {//Check odd positive polynomials
    auto f = expand(x * x * x + x + 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==+HUGE_VAL), +HUGE_VAL);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==-HUGE_VAL), -HUGE_VAL);
  }

  {//Check odd negative polynomials
    auto f = expand(-x * x * x + x + 3);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==+HUGE_VAL), -HUGE_VAL);
    BOOST_CHECK_EQUAL(substitution(f, Var<vidx<'x'> >()==-HUGE_VAL), +HUGE_VAL);
  }
}

BOOST_AUTO_TEST_CASE( poly_derivative )
{
  using namespace sym;
  Polynomial<1> x{0, 1};

  auto poly1 = expand(x + x*x + x*x*x + x*x*x*x);
  auto poly2 = derivative(poly1, Var<>());
  BOOST_CHECK_EQUAL(poly2[0], 1);
  BOOST_CHECK_EQUAL(poly2[1], 2);
  BOOST_CHECK_EQUAL(poly2[2], 3);  
  BOOST_CHECK_EQUAL(poly2[3], 4);  

  BOOST_CHECK_CLOSE(substitution(poly2, Var<>()==3.14159), eval_derivatives<1>(poly1, 3.14159)[1], 1e-10);

  auto poly3 = expand(2.0 - x + 2 * x * x);
  auto poly4 = derivative(poly3, Var<>());
  BOOST_CHECK_EQUAL(poly4[0], -1);
  BOOST_CHECK_EQUAL(poly4[1], 4);
  BOOST_CHECK_EQUAL(substitution(poly4, Var<>()==0), -1);
  BOOST_CHECK_EQUAL(substitution(poly4, Var<>()==1), 3);

  C<2>() * derivative(x, Var<>()) * PowerOp<decltype(x), 1>(x);
  //derivative(pow<2>(x), Var<vidx<'x'> >());
  
  BOOST_CHECK(compare_expression(simplify(derivative(pow<2>(x), Var<>())), C<2>()* Polynomial<1>{0,1}));
}

BOOST_AUTO_TEST_CASE( poly_zero_derivative)
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  const auto poly1 = derivative(x, Var<>());
  BOOST_CHECK_EQUAL(poly1[0], 1);

  const auto poly2 = derivative(poly1, Var<>());
  BOOST_CHECK(compare_expression(poly2, Null()));
}

BOOST_AUTO_TEST_CASE( poly_deflation)
{
  using namespace sym;
  Polynomial<1> x{0, 1};

  const double roots[] = {-1e3, 4e3, 0, 3.14159265, -3.14159265};
  for (double root1: roots)
    for (double root2: roots)
      for (double root3: roots) {
	auto poly = expand((x - root1) * (x - root2) * (x-root3));
	
	auto deflated = deflate_polynomial(poly, root1);
	auto exact = expand((x - root2) * (x-root3));
	for (size_t i(0); i < 3; ++i)
	  if (exact[i] != 0)
	    BOOST_CHECK_CLOSE(deflated[i], exact[i], 1e-10);
	  else
	    BOOST_CHECK_SMALL(deflated[i], 1e-10);
	
	deflated = deflate_polynomial(poly, root2);
	exact = expand((x - root1) * (x-root3));
	for (size_t i(0); i < 3; ++i)
	  if (exact[i] != 0)
	    BOOST_CHECK_CLOSE(deflated[i], exact[i], 1e-10);
	  else
	    BOOST_CHECK_SMALL(deflated[i], 1e-10);

	deflated = deflate_polynomial(poly, root3);
	exact = expand((x - root1) * (x-root2));
	for (size_t i(0); i < 3; ++i)
	  if (exact[i] != 0)
	    BOOST_CHECK_CLOSE(deflated[i], exact[i], 1e-10);
	  else
	    BOOST_CHECK_SMALL(deflated[i], 1e-10);
      }
}

BOOST_AUTO_TEST_CASE( poly_shift)
{
  using namespace sym;
  Polynomial<1> x{0, 1};

  const double roots[] = {-1e3, 4e3, 0, 3.14159265, -3.14159265};
  for (double root1: roots)
    for (double root2: roots)
      for (double root3: roots) {
	auto f = expand((x - root1) * (x - root2) * (x-root3));

	for (double shift : {-1.0, 2.0, 1e3, 3.14159265, -1e5}) {
	  auto g = shift_function(f, shift);
	  
	  BOOST_CHECK_CLOSE(substitution(g, Var<vidx<'x'> >() == 0), substitution(f, Var<vidx<'x'> >() == shift), 1e-10);
	  BOOST_CHECK_CLOSE(substitution(g, Var<vidx<'x'> >() == 1e3), substitution(f, Var<vidx<'x'> >() == 1e3 + shift), 1e-10);
	}
      }
}

BOOST_AUTO_TEST_CASE(poly_quadratic_roots_simple)
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  
  {//Quadratic with no roots
    auto poly = expand(x * x - 3 * x + 4);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 0);
  }
  
  {//Quadratic with one root
    auto poly = expand(-4 * x * x + 12 * x - 9);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 1);
    if (roots.size() == 1)
      BOOST_CHECK_CLOSE(roots[0], 1.5, 1e-10);
  }
  
  {//quadratic but linear function with one root
    auto poly =  expand(0 * x * x + 12 * x - 9);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 1);
    if (roots.size() == 1)
      BOOST_CHECK_CLOSE(roots[0], 0.75, 1e-10);
  }

  {//constant function, with no roots
    auto poly =  expand(0 * x * x + 0 * x - 9);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 0);
  }
}

BOOST_AUTO_TEST_CASE( poly_quadratic_special_cases)
{
  using namespace sym;
  Polynomial<1> x{0, 1};
  
  {//Quadratic with catastrophic cancellation of error
    auto poly = expand(x * x + 712345.12 * x + 1.25);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 2);
    BOOST_CHECK_CLOSE(roots[0], -712345.1199985961, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], -1.754767408250742e-6, 1e-10);
  }

  const double maxsqrt = std::sqrt(std::numeric_limits<double>::max());
  const double largeterm = maxsqrt * 100;
  {//Large linear coefficient
    auto poly = expand(x * x + largeterm * x + 1.25);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 2);
    //Mathematica value
    BOOST_CHECK_CLOSE(roots[0], -1.3407807929942599e156, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], -9.322925914000258e-157, 1e-10);
  }

  {//Large (+ve) constant coefficient
    auto poly = expand(x * x + x + largeterm);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 0);
  }
  {//Large (-ve) constant coefficient
    auto poly = expand(x * x + x - largeterm);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 2);

    //Mathematica value
    BOOST_CHECK_CLOSE(roots[0], -1.157920892373162e78, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], 1.157920892373162e78, 1e-10);
  }
}

double cubic_rootvals[] = {-1e6, -1e3, -100, -1, 0, 1, +100, 1e3, 1e6};

BOOST_AUTO_TEST_CASE( poly_linear_roots_full )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  for (double root1 : cubic_rootvals)
    for (double factor : cubic_rootvals)
	{
          if (factor == 0) continue;
	  auto f = expand(factor * (x - root1));
	  auto roots = solve_real_roots(f);
	  decltype(roots) actual_roots = {root1};
	  compare_roots(roots, actual_roots, f);
	}
}

BOOST_AUTO_TEST_CASE( poly_quadratic_roots_full )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  for (double root1 : cubic_rootvals)
    for (double root2 : cubic_rootvals)
      //for (double factor : cubic_rootvals)
	{
          //if (factor == 0) continue;
	  auto f = expand((x - root1) * (x - root2));
	  auto roots = solve_real_roots(f);
	  decltype(roots) actual_roots = {root1,root2};
	  compare_roots(roots, actual_roots, f);
	}
}

BOOST_AUTO_TEST_CASE( poly_cubic_triple_roots )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  for (double root1 : cubic_rootvals)
    for (double root2 : cubic_rootvals)
      for (double root3 : cubic_rootvals)
	{
	  auto f = expand((x - root1) * (x - root2) * (x - root3));
	  auto roots = solve_real_roots(f);
	  decltype(roots) actual_roots = {root1,root2, root3};
	  compare_roots(roots, actual_roots, f);
	}
}

BOOST_AUTO_TEST_CASE( poly_cubic_single_roots )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  for (double root1 : cubic_rootvals)
    for (double root2real : cubic_rootvals)
      for (double root2im : cubic_rootvals)
	{
	  //Skip real second root cases, and the symmetric cases
	  if (root2im <= 0) continue;
	  
	  std::complex<double>
	    root1val(root1, 0),
	    root2val(root2real, root2im),
	    root3val(root2real, -root2im);

	  auto poly_c = expand((x - root1val) * (x - root2val) * (x - root3val));
	  Polynomial<3, double> f = {poly_c[0].real(), poly_c[1].real(), poly_c[2].real(), poly_c[3].real()};

	  auto roots = solve_real_roots(f);
	  decltype(roots) actual_roots = {root1};
	  compare_roots(roots, actual_roots, f);	  
	}
}

BOOST_AUTO_TEST_CASE( poly_cubic_special_cases )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  {//Zero constant term with three roots
    auto poly = expand((x * x + 712345.12 * x + 1.25) * x);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 3);
    BOOST_CHECK_CLOSE(roots[0], -712345.1199985961, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], -1.754767408250742e-6, 1e-10);
    BOOST_CHECK_CLOSE(roots[2], 0, 1e-10);
  }

  {//Zero constant term with one root
    auto poly = expand((x * x - 3 * x + 4) * x);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 1);
    BOOST_CHECK_CLOSE(roots[0], 0, 1e-10);
  }

  {//Special case where f(x) = a * x^3 + d
    auto poly = expand(x * x * x + 1e3);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 1);
    BOOST_CHECK_CLOSE(roots[0], -10, 1e-10);

    poly = expand(x * x * x - 1e3);
    roots = solve_real_roots(poly);
    BOOST_CHECK(roots.size() == 1);
    BOOST_CHECK_CLOSE(roots[0], 10, 1e-10);
  }

  const double maxsqrt = std::sqrt(std::numeric_limits<double>::max());
  const double largeterm = maxsqrt * 100;

  {//Large x^2 term
    auto poly = expand(x * x * x - largeterm * x * x + 1.25);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK_EQUAL(roots.size(), 3);
    BOOST_CHECK_CLOSE(roots[0], -9.655529977168658e-79, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], +9.655529977168658e-79, 1e-10);
    BOOST_CHECK_CLOSE(roots[2], 1.3407807929942599e156, 1e-10);
  }

  {//Large x term
    auto poly = expand(x * x * x - x * x - largeterm * x + 1.25);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK_EQUAL(roots.size(), 3);
    BOOST_CHECK_CLOSE(roots[0], -1.1579208923731622e78, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], 9.322925914000258e-157, 1e-10);
    BOOST_CHECK_CLOSE(roots[2], 1.1579208923731622e78, 1e-10);
  }

  const double smallerterm = maxsqrt * 1e-1;
  {
    //Large v term
    auto poly = expand(x * x * x  - smallerterm * x * x - smallerterm * x + 2);
    auto roots = solve_real_roots(poly);
    BOOST_CHECK_EQUAL(roots.size(), 3);
    BOOST_CHECK_CLOSE(roots[0], -1.0, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], 1.491668146240041472864517142264024641421371730393e-153, 1e-10);
    BOOST_CHECK_CLOSE(roots[2], 1.340780792994259598314974448015366224371799690462e153, 1e-10);
  }
}

BOOST_AUTO_TEST_CASE( poly_root_tests)
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  {//Check cubic
    auto f1 = expand(4 * (x*x*x) - x * x - 2 * x + 12);
    
    auto roots = solve_real_roots(f1);
    BOOST_CHECK(roots.size() == 1);
    BOOST_CHECK_CLOSE(roots[0], -1.472711896724616002268033950475380144341, 1e-10);
    
    auto droots = solve_real_roots(derivative(f1, Var<vidx<'x'> >()));
    BOOST_CHECK(droots.size() == 2);
    BOOST_CHECK_CLOSE(droots[0], -1.0/3, 1e-10);
    BOOST_CHECK_CLOSE(droots[1], 0.5, 1e-10);
  }

  {//Check quartic
    auto f1 = expand(10 * (x*x*x*x) + x*x*x - 30 * x * x -23);

    auto roots = solve_real_roots<>(f1);
    BOOST_CHECK_EQUAL(roots.size(), 2);
    BOOST_CHECK_CLOSE(roots[0], -1.949403904489790210996459054473124835057, 1e-10);
    BOOST_CHECK_CLOSE(roots[1], +1.864235880634589025006445510389799368569, 1e-10);

    auto droots = solve_real_roots(derivative(f1, Var<vidx<'x'> >()));
    BOOST_CHECK_EQUAL(droots.size(),3);
    BOOST_CHECK_CLOSE(droots[0], -1.262818836058599076329128653113014315066, 1e-10);
    BOOST_CHECK_CLOSE(droots[1], 0, 1e-10);
    BOOST_CHECK_CLOSE(droots[2], +1.187818836058599076329128653113014315066, 1e-10);
  }

  {//PowerOp quartic
    auto f1 = expand(pow<2>(30 * x * x + x - 23));

    auto roots = solve_real_roots(f1);
    //FIX THIS UNIT TEST!
    BOOST_CHECK(roots.size() == 2);
    ////NOTE THESE ROOTS ARE DOUBLE ROOTS (roots.size() may equal 2,3, or 4)
    BOOST_CHECK_CLOSE(roots[0], -0.8924203103613100773375343963347855860436, 1e-7);
    BOOST_CHECK_CLOSE(roots[1], -0.8924203103613100773375343963347855860436, 1e-7);

    auto droots = solve_real_roots(derivative(f1, Var<vidx<'x'> >()));
    BOOST_CHECK(droots.size() == 3);
    BOOST_CHECK_CLOSE(droots[0], -0.8924203103613100773375343963347855860436, 1e-10);
    BOOST_CHECK_CLOSE(droots[1], -0.01666666666666666666666666666666666666667, 1e-10);
    BOOST_CHECK_CLOSE(droots[2], +0.8590869770279767440042010630014522527103, 1e-10);
  }
}

BOOST_AUTO_TEST_CASE( poly_gcd )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  { //Standard division with remainder
    auto q = expand(x * x * x + 3 * x - 2);
    auto g = expand(x * x - 2 * x);
    auto r = expand(4 * x - 2);
    auto f = expand(q * g + r);
    auto euclid = gcd(f, g);  
    BOOST_CHECK(compare_expression(q, std::get<0>(euclid)));
    BOOST_CHECK(compare_expression(r, std::get<1>(euclid)));
  }
  {//Standard division without remainder
    auto q = expand(x * x * x + 3 * x - 2);
    auto g = expand(x * x - 2 * x);
    auto r = Polynomial<0>{0};
    auto f = simplify(q * g + r);
    auto euclid = gcd(f, g);
    
    BOOST_CHECK(compare_expression(q, std::get<0>(euclid)));
    BOOST_CHECK(compare_expression(r, std::get<1>(euclid)));
  }

  {//Division with a zero leading order coefficient
    auto q = expand(x * x * x + 3 * x - 2);
    auto g = expand(0 * x*x*x + x*x - 2 * x);
    auto r = Polynomial<0>{0};
    auto f = expand(q * g + r);
    auto euclid = gcd(f, g);
    
    BOOST_CHECK(compare_expression(q, std::get<0>(euclid)));
    BOOST_CHECK(compare_expression(r, std::get<1>(euclid)));
  }

  {//Division by a constant
    auto q = expand(x * x * x + 3 * x - 2);
    auto g = Polynomial<0>{0.5};
    auto r = Polynomial<0>{0};
    auto f = expand(q * g + r);
    auto euclid = gcd(f, g);
    
    BOOST_CHECK(compare_expression(q, std::get<0>(euclid)));
    BOOST_CHECK(compare_expression(r, std::get<1>(euclid)));
  }

  {//Division by a high-order Polynomial which is actually a constant
    auto q = expand(x * x * x + 3 * x - 2);
    auto g = Polynomial<3>{0.25};
    auto r = Polynomial<0>{0};
    auto f = expand(q * g + r);
    auto euclid = gcd(f, g);
    
    BOOST_CHECK(compare_expression(q, std::get<0>(euclid)));
    BOOST_CHECK(compare_expression(r, std::get<1>(euclid)));
  }
}

BOOST_AUTO_TEST_CASE( poly_Sturm_chains )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  { //Example from wikipedia (x^4+x^3-x-1)
    auto f = expand(x*x*x*x + x*x*x - x - 1);
    auto chain = sturm_chain(f);

    BOOST_CHECK(compare_expression(chain.get(0), f));
    BOOST_CHECK(compare_expression(chain.get(1), expand(4*x*x*x + 3*x*x - 1)));
    BOOST_CHECK(compare_expression(chain.get(2), expand((3.0/16) * x*x + (3.0/4)*x + (15.0/16))));
    BOOST_CHECK(compare_expression(chain.get(3), expand(-32*x -64)));
    BOOST_CHECK(compare_expression(chain.get(4), Polynomial<0>{-3.0/16}));
    BOOST_CHECK(compare_expression(chain.get(5), Polynomial<0>{0}));
    BOOST_CHECK(compare_expression(chain.get(6), Polynomial<0>{0}));
    
    //This polynomial has roots at -1 and +1
    BOOST_CHECK_EQUAL(chain.sign_changes(-HUGE_VAL), 3);
    BOOST_CHECK_EQUAL(chain.sign_changes(0), 2);
    BOOST_CHECK_EQUAL(chain.sign_changes(HUGE_VAL), 1);
    
    BOOST_CHECK_EQUAL(chain.roots(0.5, 3.0), 1);
    BOOST_CHECK_EQUAL(chain.roots(-2.141, -0.314159265), 1);
    BOOST_CHECK_EQUAL(chain.roots(-HUGE_VAL, HUGE_VAL), 2);
 }
}

BOOST_AUTO_TEST_CASE( descartes_sturm_and_budan_01_alesina_rootcount_test )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  
  //The values 0.5, and 2.0 are strong tests of the algorithms, as
  //these are the division points for VCA and VAS algorithms.
  const double roots[] = {-1e5, -0.14159265, -0.0001,0.1, 0.3333, 0.5, 0.8, 1.001, 2.0, 3.14159265, 1e7};
  
  for (const double root1: roots)
    for (const double root2: roots)
      if (root1 != root2)
	for (const double root3: roots)
	  if (!std::set<double>{root1, root2}.count(root3))
	    for (const double root4: roots)
	      if (!std::set<double>{root1, root2, root3}.count(root4))
		for (const double root5: roots)
		  if (!std::set<double>{root1, root2, root3, root4}.count(root5))
		    for (int sign : {-1, +1}) {
		      //Test where all 5 roots of a 5th order
		      //Polynomial are real
		      auto f1 = expand(sign * (x - root1) * (x - root2) * (x - root3) * (x - root4) * (x - root5));
		      
		      //Test with the same roots, but an additional 2
		      //imaginary roots
		      auto f2 = expand(f1 * (x * x - 3 * x + 4));

		      auto roots_in_range = [&](double a, double b) {
			return size_t
			(((root1 > a) && (root1 < b))
			+((root2 > a) && (root2 < b))
			+((root3 > a) && (root3 < b))
			+((root4 > a) && (root4 < b))
			 +((root5 > a) && (root5 < b)))
			; };

		      size_t roots_in_01 = roots_in_range(0, 1);

		      auto chain1 = sturm_chain(f1);
		      auto chain2 = sturm_chain(f2);

		      //Test interval [0,1]
		      switch (roots_in_01) {
		      case 0: 
		      case 1: 
			BOOST_CHECK_EQUAL(budan_01_test(f1), roots_in_01);
			BOOST_CHECK_EQUAL(budan_01_test(f2), roots_in_01);
			BOOST_CHECK_EQUAL(alesina_galuzzi_test(f1, 0.0, 1.0), roots_in_01);
			BOOST_CHECK_EQUAL(alesina_galuzzi_test(f2, 0.0, 1.0), roots_in_01);
			break;
		      default: 
			BOOST_CHECK(budan_01_test(f1) >= roots_in_01); 
			BOOST_CHECK(budan_01_test(f2) >= roots_in_01); 
			BOOST_CHECK(alesina_galuzzi_test(f1, 0.0, 1.0) >= roots_in_01); 
			BOOST_CHECK(alesina_galuzzi_test(f2, 0.0, 1.0) >= roots_in_01); 
			break;
		      }
		      BOOST_CHECK_EQUAL(chain1.roots(0.0, 1.0), roots_in_01); 
		      BOOST_CHECK_EQUAL(chain2.roots(0.0, 1.0), roots_in_01); 
		      
		      //Test interval [0, \infty]
		      size_t positive_roots = roots_in_range(0, HUGE_VAL);
		      switch (positive_roots) {
		      case 0: 
		      case 1:
			BOOST_CHECK_EQUAL(descartes_rule_of_signs(f1), positive_roots); 
			BOOST_CHECK_EQUAL(descartes_rule_of_signs(f2), positive_roots);
			break;
		      default: 
			BOOST_CHECK(descartes_rule_of_signs(f1) >= positive_roots); 
			break;
		      }
		      BOOST_CHECK_EQUAL(chain1.roots(0.0, HUGE_VAL), positive_roots); 
		      BOOST_CHECK_EQUAL(chain2.roots(0.0, HUGE_VAL), positive_roots); 

		      //Try some others
		      BOOST_CHECK_EQUAL(chain1.roots(-HUGE_VAL, HUGE_VAL), 5);
		      BOOST_CHECK_EQUAL(chain2.roots(-HUGE_VAL, HUGE_VAL), 5); 
		      BOOST_CHECK(alesina_galuzzi_test(f1,-1.0, 30.0) >= roots_in_range(-1, 30));
		      BOOST_CHECK(alesina_galuzzi_test(f1,-0.01, 5.0) >= roots_in_range(-0.01, 5));
		    }
}

BOOST_AUTO_TEST_CASE( LMQ_upper_bound_test )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  const double roots[] = {-1e5, -0.14159265, 3.14159265, -0.0001,0.1, 0.3333, 0.6, 1.001, 2.0, 3.14159265, 1e7};

  //Test simple expressions
  for (const double root1: roots)
    for (const double root2: roots)
      for (const double root3: roots)
	for (const double root4: roots)
	  for (int sign : {-1, +1})
	    {
	      auto f = expand(sign * (x - root1) * (x - root2) * (x - root3) * (x - root4));

	      double max_root = root1;
	      max_root = std::max(max_root, root2);
	      max_root = std::max(max_root, root3);
	      max_root = std::max(max_root, root4);
	    
	      double bound = LMQ_upper_bound(f);
	      if (max_root < 0)
		BOOST_CHECK_EQUAL(bound, 0);
	      else
		BOOST_CHECK(bound >= max_root);
	    }

  //Test expressions with zero coefficients
  for (const double root1: roots)
    for (const double root2: roots)
      for (int sign : {-1, +1})
	{
	  auto f = expand(sign * (x - root1) * (x - root2) + 0 * x*x*x*x*x);
	  double max_root = std::max(root1, root2);
	  
	  double bound = LMQ_upper_bound(f);
	  if (max_root < 0)
	    BOOST_CHECK_EQUAL(bound, 0);
	  else
	    BOOST_CHECK(bound >= max_root);
	}

  //Test constant coefficients 
  BOOST_CHECK_EQUAL(LMQ_upper_bound(expand(1 + 0 * x*x*x*x*x)), 0);
}

BOOST_AUTO_TEST_CASE( LMQ_lower_bound_test )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};

  const double roots[] = {-1e5, -0.14159265, 3.14159265, -0.0001,0.1, 0.3333, 0.6, 1.001, 2.0, 3.14159265, 1e7};

  for (const double root1: roots)
    for (const double root2: roots)
      for (const double root3: roots)
	for (const double root4: roots)
	  for (int sign : {-1, +1})
	    {
	      auto f = expand(sign * (x - root1) * (x - root2) * (x - root3) * (x - root4));

	      double min_pos_root = HUGE_VAL;
	      if (root1 >= 0)
		min_pos_root = std::min(min_pos_root, root1);
	      if (root2 >= 0)
		min_pos_root = std::min(min_pos_root, root2);
	      if (root3 >= 0)
		min_pos_root = std::min(min_pos_root, root3);
	      if (root4 >= 0)
		min_pos_root = std::min(min_pos_root, root4);
	    
	      double bound = LMQ_lower_bound(f);
	      if (min_pos_root == HUGE_VAL)
		BOOST_CHECK_EQUAL(bound, HUGE_VAL);
	      else
		BOOST_CHECK(bound <= min_pos_root);
	    }

  //Test expressions with zero coefficients
  for (const double root1: roots)
    for (const double root2: roots)
      for (int sign : {-1, +1})
	{
	  auto f = expand(sign * (x - root1) * (x - root2) + 0 * x*x*x*x*x);

	  double min_pos_root = HUGE_VAL;
	  if (root1 >= 0)
	    min_pos_root = std::min(min_pos_root, root1);
	  if (root2 >= 0)
	    min_pos_root = std::min(min_pos_root, root2);
	  
	  double bound = LMQ_lower_bound(f);
	  if (min_pos_root == HUGE_VAL)
	    BOOST_CHECK_EQUAL(bound, HUGE_VAL);
	  else
	    BOOST_CHECK(bound <= min_pos_root);
	}

  //Test constant coefficients 
  BOOST_CHECK_EQUAL(LMQ_lower_bound(expand(1 + 0 * x*x*x*x*x)), HUGE_VAL);
}

BOOST_AUTO_TEST_CASE( generic_solve_real_roots )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  const Polynomial<1, long double> x_hp{0, 1};

  std::vector<double> roots{-0.14159265, -3.14159265, -0.0001,0.1, 0.3333, 0.6, 1.001, 2.0, 3.14159265};

  //MSVC (long double) == (double), thus there is not the precision required to test some of these roots on MSVC!
  if ((std::numeric_limits<double>::digits == std::numeric_limits<long double>::digits))
    return;
  
  for (auto it1 = roots.begin(); it1 != roots.end(); ++it1)
    for (auto it2 = it1; it2 != roots.end(); ++it2)
      for (auto it3 = it2; it3 != roots.end(); ++it3)
	for (auto it4 = it3; it4 != roots.end(); ++it4)
	  for (auto it5 = it4; it5 != roots.end(); ++it5)
	    for (int sign : {-1, +1})
	      {
		StackVector<double, 5> test_roots{*it1, *it2, *it3, *it4, *it5};

		//Test where all 5 roots of a 5th order Polynomial are real
		auto f1_hp = expand(sign * (x_hp - *it1) * (x_hp - *it2) * (x_hp - *it3) * (x_hp - *it4) * (x_hp - *it5));
		//Test where 5 roots of a 7th order Polynomial are real
		auto f2_hp = expand(f1_hp * (x_hp * x_hp - 3 * x_hp + 4));
		//Test where 5 roots of a 9th order Polynomial are real
		auto f3_hp = expand(f1_hp * (x_hp * x_hp - 3 * x_hp + 4) * (x_hp * x_hp - 3 * x_hp + 30));

		//long double precision is used to calculate the
		//coefficients of the polynomial, as loss of precision
		//is significant here.
		Polynomial<5> f1 = convert_poly<double>(f1_hp);
		Polynomial<7> f2 = convert_poly<double>(f2_hp);
		Polynomial<9> f3 = convert_poly<double>(f3_hp);
		
		//Test the default implementation
		compare_roots(solve_real_roots(f1), test_roots, f1);
		compare_roots(solve_real_roots(f2), test_roots, f2);
		compare_roots(solve_real_roots(f3), test_roots, f3);

		//These root finding methods only work for
		//squarefree polynomials, so we only test against those
		if (std::set<double>{*it1, *it2, *it3, *it4, *it5}.size() == 5) {
		  //Test VCA + TOMS748
		  compare_roots(solve_real_roots<PolyRootBounder::VCA, PolyRootBisector::TOMS748>(f1), test_roots, f1);
		  compare_roots(solve_real_roots<PolyRootBounder::VCA, PolyRootBisector::TOMS748>(f2), test_roots, f2);
			
		  //Test VAS + BISECTION
		  compare_roots(solve_real_roots<PolyRootBounder::VAS, PolyRootBisector::BISECTION>(f1), test_roots, f1);
		  compare_roots(solve_real_roots<PolyRootBounder::VAS, PolyRootBisector::BISECTION>(f2), test_roots, f2);
		}
	      }
}

BOOST_AUTO_TEST_CASE( polynomials_derivative_subtraction )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  //Test Polynomial derivatives on subtraction Operation types
  auto poly1 = expand(derivative(2*x*x - x, Var<vidx<'x'> >()));
  //derivative will automatically combine polynomials
  BOOST_CHECK_EQUAL(poly1[0], -1);
  BOOST_CHECK_EQUAL(poly1[1], 4);
}

BOOST_AUTO_TEST_CASE( polynomials_multiply_expansion )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  //Test Polynomial simplification on multiplication Operation types
  auto poly1 = expand((x + 1)*(x + 3));
  BOOST_CHECK_EQUAL(poly1[0], 3);
  BOOST_CHECK_EQUAL(poly1[1], 4);
  BOOST_CHECK_EQUAL(poly1[2], 1);
}

BOOST_AUTO_TEST_CASE( function_poly_derivatives_special )
{ 
  using namespace sym;

  //Check special case derivatives of Functions with constant
  //arguments.
  BOOST_CHECK(compare_expression(derivative(sin(Polynomial<0>{1}), Var<vidx<'x'> >()), Null()));
  BOOST_CHECK(compare_expression(derivative(cos(Polynomial<0>{1}), Var<vidx<'x'> >()), Null()));
}

template<class T> auto d(T a) -> STATOR_AUTORETURN(sym::derivative(a, sym::Var<sym::vidx<'y'> >()));

BOOST_AUTO_TEST_CASE( poly_taylor )
{ 
  using namespace sym;
  Var<vidx<'y'> > y;

  BOOST_CHECK(compare_expression(taylor_series<3>(y*y*y, Null(), Var<vidx<'x'> >()), simplify(y*y*y)));
  
  BOOST_CHECK(compare_expression(taylor_series<3>(y*y*y, Null(), y), simplify(y*y*y)));

  //Test truncation of PowerOp expressions when the original order is too high
  CHECK_TYPE(taylor_series<2>(y*y*y, Null(), y), Null);
  
  //Test simple Taylor expansion of sine 
  BOOST_CHECK(compare_expression(taylor_series<6>(sin(y), Null(), y), y * (C<1>()+(pow<2>(y)*(C<-1,6>()+(pow<2>(y)*C<1,120>()))))));

  //MSVC only supports limited symbol names lengths, thus deep
  //templating is not possible, and these tests fail on compilation
#if !(defined(_MSC_VER) && !defined(__INTEL_COMPILER))
  BOOST_CHECK(compare_expression(taylor_series<8>(sin(y*y), Null(), y), pow<2>(y) * (C<1>()+(pow<4>(y)*C<-1,6>()))));
  
  //Test Taylor expansion of a complex expression at zero
  Var<vidx<'x'> > x;
  auto f = sin(cos(x)+2*x*x - x + 3);
  auto ffinal = simplify((3.0 * std::sin(4.0)/2.0 + (std::cos(4.0)/6.0)) * x*x*x + (3*std::cos(4.0)/2.0 - std::sin(4.0)/2.0) * x*x - std::cos(4.0) * x + std::sin(4.0));

  //We compare the expressions as Polynomials using the expand
  auto ffinal_expanded = expand(ffinal);
  BOOST_CHECK(compare_expression(expand(taylor_series<3>(f, Null(), x)), ffinal_expanded));

  //Test Taylor expansion again at a non-zero location
  BOOST_CHECK(compare_expression(expand(taylor_series<3>(sin(cos(x)+2*x*x - x + 3), 3.0, x)), expand(82.77908670866608 * x*x*x - 688.8330378984795 * x*x + 1895.079543801394 * x - 1721.740734454172)));

  //Partially truncate a Polynomial through expansion
  BOOST_CHECK(compare_expression(expand(taylor_series<2>(Polynomial<3,int,Var<vidx<'y'> > >{1,2,3,4}, Null(), y)), Polynomial<2,int,Var<vidx<'y'> > >{1,2,3}));
  
  //Keep the order the same
  BOOST_CHECK(compare_expression(expand(taylor_series<3>(Polynomial<3,int,Var<vidx<'y'> > >{1,2,3,4}, Null(), y)), Polynomial<3,int,Var<vidx<'y'> > >{1,2,3,4}));
  
  //Taylor simplify at a higher order
  BOOST_CHECK(compare_expression(expand(taylor_series<4>(Polynomial<3,int,Var<vidx<'y'> > >{1,2,3,4}, Null(), y)), Polynomial<3,int,Var<vidx<'y'> > >{1,2,3,4})); 
#endif
}

BOOST_AUTO_TEST_CASE( Poly_Vector_symbolic )
{
  using namespace sym;

  static_assert(sym::detail::IsConstant<Vector>::value, "Vectors are not considered constant!");
  
  BOOST_CHECK(compare_expression(derivative(Vector{1,2,3}, Var<vidx<'x'> >()), Null()));
  BOOST_CHECK(compare_expression(Unity() * Vector{1,2,3}, Vector{1,2,3}));
  BOOST_CHECK(compare_expression(Vector{1,2,3} * Unity(), Vector{1,2,3}));

  const Polynomial<1> x{0, 1};

  const size_t testcount = 100;
  const double errlvl = 1e-10;

  Vector test1 = substitution(Vector{0,1,2} * Var<vidx<'x'> >(), Var<vidx<'x'> >() == 2);
  BOOST_CHECK(test1[0] == 0);
  BOOST_CHECK(test1[1] == 2);
  BOOST_CHECK(test1[2] == 4);

  //A tough test is to implement the Rodriugues formula symbolically.
  RNG.seed();
  for (size_t i(0); i < testcount; ++i)
    {
      double angle = angle_dist(RNG);
      Vector axis = random_unit_vec().normalized();
      Vector start = random_unit_vec();
      Vector end = Eigen::AngleAxis<double>(angle, axis) * start;
      
      Vector r = axis * axis.dot(start);
      auto f = (start - r) * cos(x) + axis.cross(start) * sin(x) + r;
      Vector err = end - substitution(f, Var<vidx<'x'> >()==angle);
      
      BOOST_CHECK(std::abs(err[0]) < errlvl);
      BOOST_CHECK(std::abs(err[1]) < errlvl);
      BOOST_CHECK(std::abs(err[2]) < errlvl);
    }

  BOOST_CHECK((toArithmetic(Vector{1,2,3}) == Vector{1,2,3}));
  BOOST_CHECK(dot(Vector{1,2,3} , Vector{4,5,6}) == 32);
}

BOOST_AUTO_TEST_CASE( generic_solve_real_roots_2 )
{
  using namespace sym;
  const Polynomial<1> x{0, 1};
  std::cout.precision(20);
  //Roots are, -1+-i, 0.5*(1+-i\sqrt{11})
  //std::cout << LinBairstowSolve(Polynomial<4>{6,4,3,1,1}, 1e-14) << std::endl;
}
