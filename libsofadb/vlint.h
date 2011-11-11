/*****************************************************************************
 * Copyright (c) 2011, Leandro T. C. Melo (ltcmelo@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef VLINT_H
#define VLINT_H

#include <cstddef>
#include <string>
#include <vector>

#include <boost/cstdint.hpp>

class vlint
{
public:
	typedef boost::uint16_t digit_type;

private:
	typedef boost::uint32_t double_digit_type;
	typedef std::vector<digit_type> magnitude_type;
	typedef magnitude_type::size_type size_type;

public:
	vlint();
	vlint(digit_type d, bool negative = false);
	vlint(digit_type d[], std::size_t s, bool negative = false);
	vlint(const std::string &decimal);

	vlint& operator+=(const vlint &other);
	vlint& operator-=(const vlint &other);
	vlint& operator*=(const vlint &other);
	vlint& operator/=(const vlint &other);
	vlint& operator%=(const vlint &other);
	vlint& operator<<=(std::size_t bits);
	vlint& operator>>=(std::size_t bits);
	vlint& operator++();
	vlint  operator++(int);
	vlint& operator--();
	vlint  operator--(int);

	bool less_than(const vlint &v) const;
	bool equal_to(const vlint &v) const;
	bool zero() const;
	bool positive() const;
	bool negative() const;

	std::string to_string() const;

	vlint add(const vlint &other) const;
	vlint subtract(const vlint &other) const;
	vlint multiply(const vlint &other) const;
	vlint divide(const vlint &other) const;
	vlint modulo(const vlint &other) const;
	vlint shift_left(std::size_t bits) const;
	vlint shift_right(std::size_t bits) const;

private:
	enum sign_group {
		negative_values,
		thezero,
		positive_values
	};

	static magnitude_type add_core(const magnitude_type &u, const magnitude_type &v);
	static magnitude_type subtract_core(const magnitude_type &u, const magnitude_type &v);
	static magnitude_type multiply_core(const magnitude_type &u, const magnitude_type &v);
	friend struct division_core;
	static void shift_left_core(magnitude_type &w, std::size_t bits);
	static void shift_right_core(magnitude_type &w, std::size_t bits);

	static void remove_leading_zeros(magnitude_type &w);

	void make_zero();
	void check_zero();
	bool magnitude_one() const;
	bool magnitude_less_than(const vlint &v) const;
	bool magnitude_equal_to(const vlint &v) const;

	static const double_digit_type k_base;
	static const std::size_t k_bits_per_digit;
	static const digit_type k_power_of_10;

	sign_group sign_;
	magnitude_type magnitude_;

#ifdef RUNNING_TESTS
	friend class vlint_test;
#endif
};

std::ostream &operator<<(std::ostream &o, const vlint &l);
vlint operator+(const vlint &a, const vlint &b);
vlint operator-(const vlint &a, const vlint &b);
vlint operator*(const vlint &a, const vlint &b);
vlint operator/(const vlint &a, const vlint &b);
vlint operator%(const vlint &a, const vlint &b);
vlint operator<<(const vlint &a, std::size_t bits);
vlint operator>>(const vlint &a, std::size_t bits);
bool operator<(const vlint& a, const vlint &b);
bool operator==(const vlint &a, const vlint &b);
bool operator!=(const vlint &a, const vlint &b);

#endif // VLINT_H
