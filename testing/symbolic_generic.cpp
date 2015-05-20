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

//stator
#include <stator/symbolic/symbolic.hpp>

//boost
#define BOOST_TEST_MODULE Symbolic_math_test
#include <boost/test/included/unit_test.hpp>

std::mt19937 RNG;
std::normal_distribution<double> normal_dist(0, 1);
std::uniform_real_distribution<double> angle_dist(0, std::atan(1)*4);

using namespace stator::symbolic;
typedef Eigen::Matrix<double, 3, 1, Eigen::DontAlign> Vector;

Vector random_unit_vec() {
  Vector vec{normal_dist(RNG), normal_dist(RNG), normal_dist(RNG)};
  return vec.normalized();
}

#define CHECK_TYPE(EXPRESSION, TYPE) static_assert(std::is_same<std::decay<decltype(EXPRESSION)>::type, TYPE>::value, "Type is wrong")


template<class T1, class T2>
bool compare_expression(const T1& f, const T2& g, bool output_error=true) {
  std::ostringstream os;
  os << f;
  std::string f_str = os.str();
  os.str(""); os.clear();
  os << g;
  std::string g_str = os.str();
  if (!(f_str == g_str) && output_error)
    std::cerr << f << " != " << g << std::endl;
  return f_str == g_str;
}


BOOST_AUTO_TEST_CASE( symbolic_C )
{
CHECK_TYPE(Unity() * Unity(), Unity);
CHECK_TYPE(Unity() + Unity(), C<2>);
CHECK_TYPE(Unity() + Null(), Unity);
CHECK_TYPE(Null() + Unity(), Unity);
CHECK_TYPE(eval(Unity(), 100), Unity);

CHECK_TYPE(multiply(Unity() , Null(), detail::select_overload{}), Null);

  Variable<'x'> x;
CHECK_TYPE(substitution(C<5>(), x == 2), C<5>);
CHECK_TYPE((C<5>() - C<3>() - C<2>()) * x, Null);

  BOOST_CHECK(compare_expression(pi()*pi()/pi(), "π"));
  BOOST_CHECK(compare_expression(e(), "e"));

CHECK_TYPE(sin(Null()), Null);
CHECK_TYPE(sin(pi()), Null);
CHECK_TYPE(sin(C<12>()*pi()), Null);
CHECK_TYPE(sin(C<5,2>()*pi()), Unity);

CHECK_TYPE(cos(Null()), Unity);
CHECK_TYPE(cos(pi()), Unity);
CHECK_TYPE(cos(C<8>()*pi()), Unity);
CHECK_TYPE(cos(C<5,2>()*pi()), Null);

Variable<'y'> y;
CHECK_TYPE(substitution(Null(), y==100), Null);
CHECK_TYPE(derivative(Null(), x), Null);
CHECK_TYPE(derivative(Unity(),x), Null);
CHECK_TYPE(substitution(Null(), y==100), Null);
//Check some substitutions
CHECK_TYPE(substitution(y*y*y, y == Null()), Null);
CHECK_TYPE(derivative(2, x), Null);
CHECK_TYPE(derivative(3.141, x), Null);

CHECK_TYPE((Vector{1,2,3} * Null()), Null);
BOOST_CHECK(compare_expression(Vector{1,2,3} + Null(), Vector{1,2,3}));
}

BOOST_AUTO_TEST_CASE( Unity_tests )
{
  //Check that Null symbols have zero derivative and value

  BOOST_CHECK(compare_expression(add(Unity(), 1.1, detail::select_overload()), 2.1));


  //Check derivatives of Unity
  BOOST_CHECK(compare_expression(derivative(Unity(), Variable<'x'>()), Null()));

  Variable<'x'> x;
  //Check simplification of multiplication with Unity
  BOOST_CHECK(compare_expression(Unity() * Unity(), Unity()));
  BOOST_CHECK(compare_expression(Unity() * 2, 2));
  BOOST_CHECK(compare_expression(Unity() * x, x));

  BOOST_CHECK(compare_expression(2 * Unity(), 2));
  BOOST_CHECK(compare_expression(x * Unity() * x, x * x));
}

BOOST_AUTO_TEST_CASE( Substitution_of_variables )
{
  Variable<'x'> x;
  Variable<'y'> y;

CHECK_TYPE(substitution(x, x==y), Variable<'y'>);
}

BOOST_AUTO_TEST_CASE( function_basic )
{
Variable<'x'> x;
  //Check basic Function operation
  BOOST_CHECK_CLOSE(eval(stator::symbolic::sin(x), 0.5), std::sin(0.5), 1e-10);
  BOOST_CHECK_CLOSE(eval(stator::symbolic::cos(x), 0.5), std::cos(0.5), 1e-10);

  //Test BinaryOP Addition and subtraction
  BOOST_CHECK_CLOSE(eval(x * sin(x) + x, 0.5), 0.5 * std::sin(0.5) + 0.5, 1e-10);
  BOOST_CHECK_CLOSE(eval(x * sin(x) - x, 0.5), 0.5 * std::sin(0.5) - 0.5, 1e-10);
}

BOOST_AUTO_TEST_CASE( function_multiplication )
{
  Variable<'x'> x;
  //Check function and Polynomial multiplication
  auto poly1 = sin(x + x) * x;
  BOOST_CHECK_CLOSE(eval(poly1, 0.5), std::sin(2 * 0.5) * 0.5, 1e-10);
  auto poly2 = x * sin(x + x);
  BOOST_CHECK_CLOSE(eval(poly2, 0.5), std::sin(2 * 0.5) * 0.5, 1e-10);
}

BOOST_AUTO_TEST_CASE( function_derivatives )
{
  Variable<'x'> x;
  //Check function and Polynomial derivatives
  auto f1 = derivative(x * sin(x), Variable<'x'>());
  BOOST_CHECK_CLOSE(eval(f1, 0.5), std::sin(0.5) + 0.5 * std::cos(0.5), 1e-10);
  auto f2 = derivative(x * cos(x), Variable<'x'>());
  BOOST_CHECK_CLOSE(eval(f2, 0.5), -0.5 * std::sin(0.5) + std::cos(0.5), 1e-10);
}

BOOST_AUTO_TEST_CASE( power_basic )
{
  Variable<'x'> x;
  BOOST_CHECK(pow<3>(3) == 27);
  BOOST_CHECK(pow<2>(Vector{0,1,2}) == 5);

  BOOST_CHECK_CLOSE(substitution(pow<3>(x), Variable<'x'>()==4.0), 4.0*4.0*4.0, 1e-10);
  BOOST_CHECK_CLOSE(eval(pow<3>(x), 0.75), std::pow(0.75, 3), 1e-10);

  //Test PowerOp algebraic operations
  BOOST_CHECK_CLOSE(eval(pow<3>(x) - x, 0.75), std::pow(0.75, 3) - 0.75, 1e-10);
  BOOST_CHECK_CLOSE(eval(pow<3>(x) + x, 0.75), std::pow(0.75, 3) + 0.75, 1e-10);
  BOOST_CHECK_CLOSE(eval(pow<3>(x) * x, 0.75), std::pow(0.75, 3) * 0.75, 1e-10);

  //Check special case derivatives
CHECK_TYPE(derivative(pow<1>(x), Variable<'x'>()), Unity);
}

BOOST_AUTO_TEST_CASE( Var_tests )
{
  Variable<'x'> x;
  Variable<'y'> y;

  BOOST_CHECK(compare_expression(x, "x")); 
  BOOST_CHECK(compare_expression(y, "y"));
  BOOST_CHECK(compare_expression(derivative(x, Variable<'x'>()), Unity()));
  BOOST_CHECK(compare_expression(derivative(y, Variable<'x'>()), Null()));
  BOOST_CHECK(compare_expression(derivative(y, Variable<'y'>()), Unity()));
  BOOST_CHECK(compare_expression(substitution(x, x == 3.14159265), 3.14159265));

  //Check that substitutions in the wrong variable do nothing
  BOOST_CHECK(compare_expression(substitution(y, x == 3.14159265), "y"));

  //Check default substitution is for x
  BOOST_CHECK(compare_expression(eval(y, 3.14159265), "y"));

  //Check that Var derivatives are correct
  BOOST_CHECK(compare_expression(derivative(sin(x), Variable<'x'>()), cos(x)));

  //Check derivatives of Unity
  BOOST_CHECK(compare_expression(derivative(Unity(), Variable<'x'>()), Null()));
  BOOST_CHECK(compare_expression(derivative(x, Variable<'x'>()), Unity()));
  BOOST_CHECK(compare_expression(derivative(x * sin(x), Variable<'x'>()), sin(x) + x * cos(x)));
}

BOOST_AUTO_TEST_CASE( reorder_operations )
{
  Variable<'x'> x;
  //Check the specialised multiply operators are consistently
  //simplifying statements.

  //Again, check that negative matches are correctly determined by
  //compare_expression
  BOOST_CHECK(!compare_expression(x, sin(x), false));

  //Here we're looking for the two Polynomial terms to be reordered 
  BOOST_CHECK(compare_expression((sin(2*x) * x) * x, x * x * sin(2*x)));
  BOOST_CHECK(compare_expression((x * sin(2*x)) * x, x * x * sin(2*x)));
  BOOST_CHECK(compare_expression(x * (sin(2*x) * x), x * x * sin(2*x)));
  BOOST_CHECK(compare_expression(x * (x * sin(2*x)), x * x * sin(2*x)));

  //Here we check that constants (such as 2) will become Null
  //types when the derivative is taken, causing their terms to be
  //eliminated.
  BOOST_CHECK(compare_expression(derivative(2 * cos(x), Variable<'x'>()), -2 * sin(x)));
  BOOST_CHECK(compare_expression(derivative(2 * sin(x), Variable<'x'>()), 2 * cos(x)));
}

BOOST_AUTO_TEST_CASE( Factorial_test )
{
  static_assert(Factorial<0>::value::num == 1, "0! != 1");
  static_assert(Factorial<1>::value::num == 1, "1! != 1");
  static_assert(Factorial<3>::value::num == 6, "3! != 6");
  static_assert(Factorial<3>::value::den == 1, "Base isn't 1!");

  static_assert(InvFactorial<0>::value::den == 1, "0! != 1");
  static_assert(InvFactorial<1>::value::den == 1, "1! != 1");
  static_assert(InvFactorial<3>::value::den == 6, "3! != 6");
  static_assert(InvFactorial<3>::value::num == 1, "Num isn't 1!");
}


BOOST_AUTO_TEST_CASE( vector_symbolic )
{
  static_assert(stator::symbolic::detail::IsConstant<Vector>::value, "Vectors are not considered constant!");
  
  BOOST_CHECK(compare_expression(derivative(Vector{1,2,3}, Variable<'x'>()), Null()));
  BOOST_CHECK(compare_expression(Unity() * Vector{1,2,3}, Vector{1,2,3}));
  BOOST_CHECK(compare_expression(Vector{1,2,3} * Unity(), Vector{1,2,3}));

  Variable<'x'> x;

  const size_t testcount = 100;
  const double errlvl = 1e-10;

  Vector test1 = substitution(Vector{0,1,2} * x, x == 2);
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
      Vector err = end - eval(f, angle);
      
      BOOST_CHECK(std::abs(err[0]) < errlvl);
      BOOST_CHECK(std::abs(err[1]) < errlvl);
      BOOST_CHECK(std::abs(err[2]) < errlvl);
    }

  BOOST_CHECK(toArithmetic(Vector{1,2,3}) == (Vector{1,2,3}));
  BOOST_CHECK(dot(Vector{1,2,3} , Vector{4,5,6}, stator::symbolic::detail::select_overload{}) == 32);
}

BOOST_AUTO_TEST_CASE( symbolic_abs_arbsign )
{
  Variable<'x'> x;

  BOOST_CHECK(compare_expression(abs(x), "|x|"));
  BOOST_CHECK_EQUAL(substitution(abs(x*x - 5*x), x==2), 6);
  BOOST_CHECK_EQUAL(C<6/-2>::num, -3);
  BOOST_CHECK_EQUAL(C<-8/-4>::num, 2);
  BOOST_CHECK(compare_expression(abs(Unity()), Unity()));
  BOOST_CHECK(compare_expression(abs(Null()), Null()));
  BOOST_CHECK(compare_expression(abs(Null()), Null()));
  BOOST_CHECK(compare_expression(derivative(arbsign(x), x), arbsign(Unity())));
  BOOST_CHECK(compare_expression(derivative(arbsign(x), Variable<'y'>()), Null()));

  BOOST_CHECK(compare_expression(simplify(x * arbsign(x)), arbsign(pow<2>(x))));
  BOOST_CHECK(compare_expression(simplify(arbsign(x) * x), arbsign(pow<2>(x))));
  BOOST_CHECK(compare_expression(simplify(arbsign(x) * arbsign(x)), arbsign(pow<2>(x))));

  BOOST_CHECK(compare_expression(simplify(x / arbsign(x)), arbsign(Unity())));
  BOOST_CHECK(compare_expression(simplify(arbsign(x) / x), arbsign(Unity())));
  BOOST_CHECK(compare_expression(simplify(arbsign(x) / arbsign(x)), arbsign(Unity())));
  BOOST_CHECK(compare_expression(simplify(arbsign(arbsign(x))), arbsign(x)));
  BOOST_CHECK(compare_expression(simplify(pow<5>(arbsign(x))), arbsign(pow<5>(x))));
  BOOST_CHECK(compare_expression(simplify(pow<6>(arbsign(x))), pow<6>(x)));
}