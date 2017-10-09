var searchData=
[
  ['dd_5fvisit',['dd_visit',['../structsym_1_1detail_1_1SimplifyRT.html#ab5a5e984924f09bd5552e1b101b5dab1',1,'sym::detail::SimplifyRT::dd_visit(const T1 &amp;l, const T2 &amp;r, Op)'],['../structsym_1_1detail_1_1SimplifyRT.html#a394e524229a0004f6671cfc895cd89e5',1,'sym::detail::SimplifyRT::dd_visit(const double &amp;l, const T2 &amp;r, detail::Subtract)'],['../structsym_1_1detail_1_1SimplifyRT.html#a45cfb31d128d216fbe2ff43c9e7b1c73',1,'sym::detail::SimplifyRT::dd_visit(const double &amp;l, const BinaryOp&lt; Expr, detail::Multiply, Expr &gt; &amp;r, detail::Multiply)'],['../structsym_1_1detail_1_1SimplifyRT.html#aa9d5559b4c9853a7420688009773cff9',1,'sym::detail::SimplifyRT::dd_visit(const BinaryOp&lt; Expr, detail::Multiply, Expr &gt; &amp;r, const double &amp;l, detail::Multiply)']]],
  ['deflate_5fpolynomial',['deflate_polynomial',['../classsym_1_1Polynomial.html#abceed2e1f15d2a0f1a50943806c4b853',1,'sym::Polynomial::deflate_polynomial(const Polynomial&lt; Order, Coeff_t, PolyVar &gt; &amp;a, const double root)'],['../classsym_1_1Polynomial.html#a5463d42e10f39ce3d00504eaf2eafb8f',1,'sym::Polynomial::deflate_polynomial(const Polynomial&lt; Order, Coeff_t, PolyVar &gt; &amp;a, Null)']]],
  ['derivative',['derivative',['../classsym_1_1Polynomial.html#ae351d04a9d403044ed0c5cf07b33e970',1,'sym::Polynomial::derivative(const Polynomial&lt; N, Coeff_t, PolyVar &gt; &amp;f, Var&lt; VarArgs... &gt;)'],['../classsym_1_1Polynomial.html#a3ae408df9721e673cc67275d67c87680',1,'sym::Polynomial::derivative(const Polynomial&lt; N, Coeff_t, PolyVar &gt; &amp;f, Var&lt; VarArgs... &gt;)'],['../namespacesym.html#afabeb933868220c3c142c87e27d2a237',1,'sym::derivative(const Expression &amp;)'],['../namespacesym.html#a836f6b5692ad97ec7a4a53f63fa2358d',1,'sym::derivative(const AddOp&lt; LHS, RHS &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN(derivative(f._l, v)+derivative(f._r, v)) template&lt; class Var, class LHS, class RHS &gt; auto derivative(const SubtractOp&lt; LHS, RHS &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN(derivative(f._l, v) - derivative(f._r, v)) template&lt; class Var, class LHS, class RHS &gt; auto derivative(const MultiplyOp&lt; LHS, RHS &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN(derivative(f._l, v) *f._r+f._l *derivative(f._r, v)) template&lt; class Var, class LHS, class RHS &gt; auto derivative(const DivideOp&lt; LHS, RHS &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN((derivative(f._l, v) *f._r - f._l *derivative(f._r, v))/pow(f._r, C&lt; 2 &gt;())) template&lt; class Var, class Arg, std::intmax_t num, std::intmax_t den &gt; auto derivative(const PowerOp&lt; Arg, C&lt; num, den &gt; &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN((C&lt; num, den &gt;() *derivative(f._l, v) *pow(f._l, C&lt; num, den &gt;() -C&lt; 1 &gt;())))'],['../namespacesym.html#a50db1740e10bb8f61c6dcf6d03248804',1,'sym::derivative(const PowerOp&lt; Arg, Power &gt; &amp;f, Var v) -&gt; STATOR_AUTORETURN(f._r *derivative(f._l, v) *pow(f._l, f._r - C&lt; 1 &gt;())+derivative(f._r, v) *log(f._l) *f)'],['../namespacesym.html#a483e4189ceb542f1302170763468062c',1,'sym::derivative(Var&lt; Args1... &gt; v1, Var&lt; Args2... &gt; v2)'],['../namespacesym.html#afbb637ace588ae027451c029bee9f81f',1,'sym::derivative(const Expr &amp;f, const Var v)'],['../namespacesym.html#a93fab33c9c2637783ee91334a83a4ca5',1,'sym::derivative(const T &amp;, Var&lt; Args... &gt;)'],['../namespacesym.html#a09371363e1f8b44731583c11d3ac7c0f',1,'sym::derivative(Var&lt; Args1... &gt;, Var&lt; Args2... &gt;) -&gt; typename std::enable_if&lt; Var&lt; Args1... &gt;::idx==Var&lt; Args2... &gt;::idx, Unity &gt;::type'],['../namespacesym.html#a0e10223c3944b1ddcda0bff54ab1587d',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Sine &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(derivative(f._arg, x) *sym::cos(f._arg))'],['../namespacesym.html#a5ff0308b252c36ee5d051ae0e5484ec7',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Cosine &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(-derivative(f._arg, x) *sym::sin(f._arg))'],['../namespacesym.html#a3e0ec07cf75a4729ccf957bf8e114ace',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Exp &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(derivative(f._arg, x) *f)'],['../namespacesym.html#ada4e03665585338d813cdc13be4fe319',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Log &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(derivative(f._arg, x)/f._arg)'],['../namespacesym.html#ac087e553867877145939fccb7bd158a0',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Absolute &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(derivative(f._arg, x) *sym::abs(f._arg)/f._arg)'],['../namespacesym.html#a30160f389d70a6fdbc1b1e5d4e1619fd',1,'sym::derivative(const UnaryOp&lt; Arg, detail::Arbsign &gt; &amp;f, Var x) -&gt; STATOR_AUTORETURN(derivative(f._arg, x) *sym::arbsign(Unity()))']]],
  ['derivativert',['DerivativeRT',['../structsym_1_1detail_1_1DerivativeRT.html#ac43d0a8e975722bfc53ede865984c98d',1,'sym::detail::DerivativeRT']]],
  ['descartes_5frule_5fof_5fsigns',['descartes_rule_of_signs',['../classsym_1_1Polynomial.html#aeee847c21ce675b9feae0b56d1472a49',1,'sym::Polynomial']]],
  ['dimensions',['dimensions',['../classstator_1_1geometry_1_1AABox.html#a75b2da95fe62fab32d7a76af56ad7729',1,'stator::geometry::AABox']]],
  ['doubledispatch1',['DoubleDispatch1',['../structsym_1_1detail_1_1DoubleDispatch1.html#afe80af056e90aef2b4b49202b137ae44',1,'sym::detail::DoubleDispatch1']]],
  ['doubledispatch2',['DoubleDispatch2',['../structsym_1_1detail_1_1DoubleDispatch2.html#a1cb36d9527f51977a80e05d72b59585b',1,'sym::detail::DoubleDispatch2']]]
];
