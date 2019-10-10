#pragma once

#include "finite_algebra.hpp"
#include "ga.hpp"

#include <cmath>

// Projective Geometric Algebra for representing Euclidean 2-space

namespace gal
{
namespace pga2
{
    // NOTE: the inner product of e0 can be set to +1 or -1 without any change in the algebra's geometric
    // interpretation. Here, we opt to define e0^2 := 1 by convention
    using metric = gal::metric<2, 0, 1>;

    using algebra = ga::algebra<metric>;

    GAL_OPERATORS(algebra);

    using e    = multivector<void, term<element<0>, monomial<one>>>;
    using e0   = multivector<void, term<element<0b1>, monomial<one>>>;
    using e1   = multivector<void, term<element<0b10>, monomial<one>>>;
    using e2   = multivector<void, term<element<0b100>, monomial<one>>>;
    using e01  = multivector<void, term<element<0b11>, monomial<one>>>;
    using e02  = multivector<void, term<element<0b101>, monomial<one>>>;
    using e12  = multivector<void, term<element<0b110>, monomial<one>>>;
    using e012 = multivector<void, term<element<0b111>, monomial<one>>>;

    // Point at (x, y)
    template <int X, int Y>
    using point_t = multivector<void,
                                term<element<0b11>, monomial<rational<Y>>>,
                                term<element<0b101>, monomial<rational<-X>>>,
                                term<element<0b110>, monomial<one>>>;

    template <typename T = float>
    struct point
    {
        using value_t = T;
        constexpr static size_t size = 2;

        template <size_t ID>
        using type = multivector<void,
                                 term<element<0b11>, monomial<one, generator<tag<ID, 1>>>>,        // y
                                 term<element<0b101>, monomial<minus_one, generator<tag<ID, 0>>>>, // -x
                                 term<element<0b110>, monomial<one>>>;

        union
        {
            T x;
            T u;
            T s;
        };

        union
        {
            T y;
            T v;
            T t;
        };

        GAL_ACCESSORS

        template <typename Engine, typename... I>
        [[nodiscard]] constexpr static point<T> convert(const Engine& engine, multivector<void, I...> mv) noexcept
        {
            auto x_e = extract<0b101>(mv);
            auto y_e = extract<0b11>(mv);
            auto c_e = extract<0b110>(mv);

            T x = -engine.template evaluate<T>(x_e);
            T y = engine.template evaluate<T>(y_e);
            T c = engine.template evaluate<T>(c_e);

            return {x / c, y / c};
        }
    };

    // Line with equation ax + by + c = 0
    template <int A, int B, int C>
    using line_t = multivector<void,
                               term<element<0b1>, monomial<rational<C>>>,
                               term<element<0b10>, monomial<rational<A>>>,
                               term<element<0b100>, monomial<rational<B>>>>;

    template <typename T = float>
    struct line
    {
        using value_t = T;
        constexpr static size_t size = 3;

        template <size_t ID>
        using type  = multivector<void,
                                   term<element<0b1>, monomial<one, generator<tag<ID, 2>>>>,
                                   term<element<0b10>, monomial<one, generator<tag<ID, 0>>>>,
                                   term<element<0b100>, monomial<one, generator<tag<ID, 1>>>>>;
        T a;
        T b;
        T c;

        GAL_ACCESSORS

        template <typename Engine, typename... I>
        [[nodiscard]] constexpr static line<T> convert(const Engine& engine, multivector<void, I...> mv) noexcept
        {
            auto a_e = extract<0b10>(mv);
            auto b_e = extract<0b100>(mv);
            auto c_e = extract<0b1>(mv);

            T a = engine.template evaluate<T>(a_e);
            T b = engine.template evaluate<T>(b_e);
            T c = engine.template evaluate<T>(c_e);

            return {a, b, c};
        }
    };

    // Direction pointing toward (x, y) aka an ideal point
    template <int X, int Y>
    using direction_t
        = multivector<void, term<element<0b11>, monomial<rational<Y>>>, term<element<0b101>, monomial<rational<-X>>>>;

    template <typename T>
    struct alignas(16) direction
    {
        using value_t = T;
        constexpr static size_t size = 2;

        template <size_t ID>
        using type = multivector<void,
                                 term<element<0b11>, monomial<minus_one, generator<tag<ID, 1>>>>, // y
                                 term<element<0b101>, monomial<one, generator<tag<ID, 0>>>>>;     // -x

        T x;
        T y;

        GAL_ACCESSORS

        template <typename Engine, typename... I>
        [[nodiscard]] constexpr static direction<T> convert(Engine& engine, multivector<void, I...> mv) noexcept
        {
            auto x_e = -extract<0b101>(mv);
            auto y_e = extract<0b11>(mv);

            auto&& [x, y] = engine.template evaluate_terms<T>(x_e, y_e);

            return {x, y};
        }
    };

    // Additional helper functions for plane geometry and result verification
    template <typename... T>
    [[nodiscard]] constexpr auto line_slope(multivector<void, T...> line) noexcept
    {
        constexpr auto x = extract<0b10>(line);
        constexpr auto y = extract<0b100>(line);
        // For now this only works on compile time multivectors over a finite field
        static_assert(decltype(x)::size == 1);
        static_assert(decltype(y)::size == 1);
        return -typename decltype(x)::first_t::rational_t{} / typename decltype(y)::first_t::rational_t{};
    }

    template <typename... T>
    [[nodiscard]] constexpr auto cartesian_point(multivector<void, T...> point) noexcept
    {
        constexpr auto x    = -extract<0b101>(point);
        constexpr auto y    = extract<0b11>(point);
        constexpr auto sign = extract<0b110>(point);

        // For now this only works on compile time multivectors over a finite field
        static_assert(decltype(x)::size == 1);
        static_assert(decltype(y)::size == 1);
        static_assert(decltype(sign)::size == 1);

        return std::make_pair(
            typename decltype(x)::first_t::rational_t{} / typename decltype(sign)::first_t::rational_t{},
            typename decltype(y)::first_t::rational_t{} / typename decltype(sign)::first_t::rational_t{});
    }
} // namespace pga2
} // namespace gal