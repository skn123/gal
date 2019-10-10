#pragma once

#include <gal/cga.hpp>
#include <gal/engine.hpp>
#include <gal/formatters.hpp>

// SANITY TEST
//
// When the input angles are:
//
//   ang1 = deg2rad(14.0)
//   ang2 = deg2rad(-25.0)
//   ang3 = deg2rad(32.6)
//   ang4 = deg2rad(66.9)
//   ang5 = deg2rad(-42.0)
//
// the result must be:
//
//   R1 = 0.992546 + 0.121869 * e1^e2
//   R2 = 0.976296 + 0.210006 * e1^e3 - 0.0523604 * e2^e3 + 142.804 * e1^ni - 35.6051 * e2^ni - 43.2871 * e3^ni
//   R3 = 0.959806 - 0.272314 * e1^e3 + 0.0678954 * e2^e3 - 404.827 * e1^ni + 100.935 * e2^ni + 161.69 * e3^ni
//   T2 = 1 - 182.475 * e1^ni + 45.4961 * e2^ni + 41.6926 * e3^ni
//   R4 = 0.834423 + 0.296658 * e1^e2 + 0.112228 * e1^e3 + 0.450123 * e2^e3 + 145.475 * e1^ni + 583.469 * e2^ni
//   Rg = 0.933654 + 0.277405 * e1^e2 + 0.0937376 * e1^e3 - 0.206198 * e2^e3 + 112.644 * e1^ni - 763.223 * e2^ni
//        - 174.171 * e3^ni
//   Jg_f = 1351.52 * e1 - 498.052 * e2 + 2132.49 * e3 + 0.99996 * no + 3.31122e+06 * ni
//
// where 'no' is the null point at the origin and 'ni' is the null point at infinity.

namespace gabenchmark
{
using namespace gal::cga;
using point = point<real_t>;
using namespace gal;

// Fourth order exp expansion
template <typename T>
inline auto expp(T arg)
{
    return gal::engine{arg}.compute([](auto arg) {
        auto arg2 = arg * arg;
        auto arg3 = arg2 * arg;
        auto arg4 = arg2 * arg2;
        return component_filter<0b10111>(rational<1>{} + arg + one_half{} * arg2 + rational<1, 6>{} * arg3
                                         + rational<1, 24>{} * arg4);
    }).template reify<real_t>();
}

template <typename T = real_t>
struct point_z
{
    constexpr static size_t size = 0;

    template <size_t ID>
    using type = multivector<void,
                             term<element<0b100>, monomial<one>>,
                             term<element<0b1000>, monomial<one>>,
                             term<element<0b10000>, monomial<one_half>>>;

    GAL_ACCESSORS
};

template <typename T = real_t>
struct point_xz
{
    constexpr static size_t size = 2;

    template <size_t ID>
    using type = multivector<void,
                             term<element<0b1>, monomial<one, generator<tag<ID, 0>>>>,
                             term<element<0b100>, monomial<one, generator<tag<ID, 1>>>>,
                             term<element<0b1000>, monomial<one>>,
                             term<element<0b10000>,
                                  monomial<one_half, generator<tag<ID, 0>, degree<2>>>,
                                  monomial<one_half, generator<tag<ID, 1>, degree<2>>>>>;

    real_t x;
    real_t z;

    GAL_ACCESSORS
};

template <typename Scalar>
constexpr auto
InverseKinematics(const Scalar& ang1, const Scalar& ang2, const Scalar& ang3, const Scalar& ang4, const Scalar& ang5)
{
    real_t d1 = 200.0, d2 = 680.0, d3 = 150.0, d4 = 140.0, d5 = 114.2;
    real_t l12 = 890.0, l23 = 880.0;

    real_t J1_x = d1;
    real_t J1_y = 0.0;
    real_t J1_z = d2;
    real_t J2_x = d1;
    real_t J2_y = 0.0;
    real_t J2_z = d2 + l12;
    real_t J3_x = d1 + l23;
    real_t J3_y = 0.0;
    real_t J3_z = d2 + l12 + d3;
    real_t Jg_x = d1 + l23 + d4 + d5;
    real_t Jg_y = 0.0;
    real_t Jg_z = d2 + l12 + d3;

    // point J1{J1_x, J1_y, J1_z};
    // point J2{J2_x, J2_y, J2_z};
    // point J3{J3_x, J3_y, J3_z};
    // point Jg{Jg_x, Jg_y, Jg_z};
    // The y components of these points is 0 so we just construct xz points to save compilation time. The runtime is not
    // affected with this change
    point_xz<> J1{J1_x, J1_z};
    point_xz<> J2{J2_x, J2_z};
    point_xz<> J3{J3_x, J3_z};
    point_xz<> Jg{Jg_x, Jg_z};

    // Point at (0, 0, 1)
    point_z<> Pz;

    auto Lz = gal::engine{Pz, ang1}
                  .compute([](auto Pz, auto ang1) {
                      return one_half{} * ang1 * ((e_o ^ Pz ^ e_inf) >> cga::pseudoscalar::inverse);
                  })
                  .template reify<real_t>();
    auto R1 = expp(Lz);

    point P2_help{J1_x, J1_y + 1.0, J1_z};
    auto L2 = gal::engine{R1, J1, P2_help, ang2}
                  .compute([](auto R1, auto J1, auto P2_help, auto ang2) {
                      auto L2init = (J1 ^ P2_help ^ e_inf) >> cga::pseudoscalar::inverse;
                      return one_half{} * ang2 * conjugate(R1, L2init);
                  })
                  .template reify<real_t>();
    auto R2 = expp(L2);

    point P3_help{J2_x, J2_y + 1.0, J2_z};

    auto R21 = gal::engine{R1, R2}.compute([](auto R1, auto R2) { return R2 * R1; }).template reify<real_t>();

    auto [J2_f, L3] = gal::engine{R21, J2, P3_help, ang3}
                          .compute([](auto R21, auto J2, auto P3_help, auto ang3) {
                              auto L3init = (J2 ^ P3_help ^ e_inf) >> cga::pseudoscalar::inverse;
                              auto J2_f   = conjugate(R21, J2);
                              return std::make_tuple(J2_f, one_half{} * ang3 * conjugate(R21, L3init));
                          })
                          .template reify<real_t>();

    auto R3 = expp(L3);

    auto [J2_rot1, t2_help] = gal::engine{R1, J2, J2_f}
                                  .compute([](auto R1, auto J2, auto J2_f) {
                                      auto J2_rot1 = conjugate(R1, J2);
                                      auto t2      = gal::component_select<0b1, 0b10, 0b100>(J2_f)
                                                - gal::component_select<0b1, 0b10, 0b100>(J2_rot1);
                                      return std::make_tuple(J2_rot1, minus_one_half{} * t2 ^ e_inf);
                                  })
                                  .template reify<real_t>();

    auto T2 = expp(t2_help);

    auto [L4init, L4weight, R3T2R1] = gal::engine{J3, Jg, R3, T2, R1}
                                          .compute([](auto J3, auto Jg, auto R3, auto T2, auto R1) {
                                              auto L4init   = (J3 ^ Jg ^ e_inf) >> cga::pseudoscalar::inverse;
                                              auto L4weight = L4init >> ~L4init;
                                              return std::make_tuple(L4init, L4weight, R3 * T2 * R1);
                                          })
                                          .template reify<real_t>();

    auto norm = L4weight.data[0] < 0 ? -std::sqrt(-L4weight.data[0]) : std::sqrt(L4weight.data[0]);
    for (auto& component : L4init.data)
    {
        component /= norm;
    }

    auto L4
        = gal::engine{L4init, R3T2R1, ang4}
              .compute([](auto L4init, auto R3T2R1, auto ang4) { return one_half{} * ang4 * conjugate(R3T2R1, L4init); })
              .template reify<real_t>();
    auto R4 = expp(L4);

    point Pg_help{J3_x, J3_y + 1.0, J3_z};
    auto [Lginit, R4R3T2R1] = gal::engine{R4, R3T2R1, J3, Pg_help}
                                  .compute([](auto R4, auto R3T2R1, auto J3, auto Pg_help) {
                                      auto Lginit   = (J3 ^ Pg_help ^ e_inf) >> cga::pseudoscalar::inverse;
                                      auto R4R3T2R1 = R4 * R3T2R1;
                                      return std::make_tuple(Lginit, R4R3T2R1);
                                  })
                                  .template reify<real_t>();
    auto Lg = gal::engine{Lginit, R4R3T2R1, ang5}
                  .compute([](auto Lginit, auto R4R3T2R1, auto ang5) {
                      return one_half{} * ang5 * conjugate(R4R3T2R1, Lginit);
                  })
                  .template reify<real_t>();
    auto Rg = expp(Lg);
    auto Rfinal
        = gal::engine{Rg, R4R3T2R1}.compute([](auto Rg, auto R4R3T2R1) { return Rg * R4R3T2R1; }).template reify<real_t>();

    auto Jg_f = gal::engine{Rfinal, Jg}
                    .compute([](auto Rfinal, auto Jg) { return conjugate(Rfinal, Jg); })
                    .template reify<real_t>();

    return std::make_tuple(R1, R2, R3, T2, R4, Rg, Jg_f);
    // fmt::print("R1: {}\n", R1);
    // fmt::print("R2: {}\n", R2);
    // fmt::print("R3: {}\n", R3);
    // fmt::print("T2: {}\n", T2);
    // fmt::print("R4: {}\n", R4);
    // fmt::print("Rg: {}\n", Rg);
    // fmt::print("Jg_f: {}\n", Jg_f);
}
} // namespace gabenchmark