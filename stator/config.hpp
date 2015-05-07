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

#pragma once
#include <Eigen/Dense>

namespace stator {
  
  /*! \brief A convenience typedef for a non-aligned Eigen Matrix.*/
  template<typename Scalar, size_t D1, size_t D2> using Matrix = Eigen::Matrix<Scalar, D1, D2, Eigen::DontAlign>;
  
  /*! \brief A convenience typedef for a non-aligned Eigen Vector.*/
  template<typename Scalar, size_t D> using Vector = Matrix<Scalar, D, 1>;
} // namespace stator
