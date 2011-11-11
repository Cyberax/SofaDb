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

#include "vlint.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <list>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

static vlint one(1);

const vlint::double_digit_type vlint::k_base(0x10000);
const std::size_t vlint::k_bits_per_digit(16);
const vlint::digit_type vlint::k_power_of_10(10000);

vlint::vlint()
	: sign_(thezero)
	, magnitude_()
{}

vlint::vlint(digit_type d, bool negative)
	: sign_(negative ? negative_values : positive_values)
	, magnitude_(1, d)
{
	check_zero();
}

vlint::vlint (digit_type d[], std::size_t s, bool negative)
	: sign_(negative ? negative_values : positive_values)
	, magnitude_(d, d + s)
{
	check_zero();
}

vlint::vlint(const std::string &decimal)
	: sign_(thezero)
{
	if (!decimal.empty()) {
		sign_ = positive_values;
		std::string current = decimal;
		if (decimal[0] == '+' || decimal[0] == '-') {
			if (decimal[0] == '-')
				sign_ = negative_values;
			current = decimal.substr(1);
		}

		const std::size_t max = std::log10(k_power_of_10);
		std::list<std::string> parts;
		while (current.length() > max) {
			parts.push_front(current.substr(current.length() - max));
			current = current.erase(current.length() - max);
		}
		parts.push_front(current);

		static const magnitude_type power_of_ten(1, k_power_of_10);
		std::list<std::string>::const_iterator it = parts.begin();
		while (it != parts.end()) {
			std::istringstream iss(*it);
			digit_type digit;
			iss >> digit;
			magnitude_ = add_core(magnitude_, magnitude_type(1, digit));
			++it;
			if (it != parts.end())
				magnitude_ = multiply_core(magnitude_, power_of_ten);
		}
		check_zero();
	}
}

vlint::magnitude_type vlint::add_core(const magnitude_type &u, const magnitude_type &v)
{
	digit_type k = 0;
	const size_type un = u.size();
	const size_type vn = v.size();
	const size_type n = std::max(un, vn);
	magnitude_type w(n);
	for (size_type j = 0; j < n; ++j) {
		const digit_type uj = j >= un ? 0 : u[j];
		const digit_type vj = j >= vn ? 0 : v[j];
		const double_digit_type current = uj + vj + k;
		w[j] = current % k_base;
		k = current/k_base;
	}
	assert(k == 0 || k == 1);

	if (k > 0)
		w.push_back(k);

	return w;
}

vlint::magnitude_type vlint::subtract_core(const magnitude_type &u, const magnitude_type &v)
{
	digit_type k = 1;
	const size_type vn = v.size();
	const size_type un = u.size();
	magnitude_type w(un);
	for (size_type j = 0; j < un; ++j) {
		const digit_type uj = u[j];
		const digit_type vj = j >= vn ? 0 : v[j];
		const double_digit_type current = uj - vj + k + k_base - 1;
		w[j] = current % k_base;
		k = current/k_base;
	}
	assert(k == 1);
	remove_leading_zeros(w);

	return w;
}

vlint::magnitude_type vlint::multiply_core(const magnitude_type &u, const magnitude_type &v)
{
	const size_type m = u.size();
	const size_type n = v.size();
	magnitude_type w(m + n);
	for (size_type j = 0; j < n; ++j) {
		if (v[j] == 0)
			continue;
		digit_type k = 0;
		for (size_type i = 0; i < m; ++i) {
			const double_digit_type current = u[i] * v[j] + w[i + j] + k;
			w[i + j] = current % k_base;
			k = current/k_base ;
		}
		w[j + m] = k;
	}
	remove_leading_zeros(w);

	return w;
}

struct division_core
{
	typedef vlint::digit_type digit_type;
	typedef vlint::double_digit_type double_digit_type;
	typedef vlint::magnitude_type magnitude_type;
	typedef vlint::size_type size_type;

	division_core(const magnitude_type &u, const magnitude_type &v)
		: q_(0), du_(u), dv_(v), shifts_(0)
	{}

	void divide();

	magnitude_type q_;
	magnitude_type du_;
	magnitude_type dv_;
	std::size_t shifts_;
};

void division_core::divide()
{
	const int n = dv_.size();
	const int m = du_.size() - n;
	q_.resize(m + 1);

	if (n == 1) {
		const digit_type divisor = dv_[0];

		size_type j = du_.size() - 1;
		if (du_[j] >= divisor) {
			const digit_type digit = du_[j];
			q_[j] = digit/divisor;
			du_[j] = digit % divisor;
		}

		for (; j > 0; --j) {
			const digit_type high_digit = du_[j];
			const digit_type low_digit = du_[j - 1];
			const double_digit_type divident = low_digit + vlint::k_base * high_digit;
			q_[j - 1] = divident/divisor;
			du_[j - 1] = divident % divisor;
			du_[j] = 0;
		}
		vlint::remove_leading_zeros(du_);
		vlint::remove_leading_zeros(q_);

		return;
	}

	const digit_type half_base = vlint::k_base >> 1;
	while (dv_.back() < half_base) {
		vlint::shift_left_core(dv_, 1);
		++shifts_;
	}
	if (shifts_)
		vlint::shift_left_core(du_, shifts_);

	for (int j = m; j >= 0; --j) {
		const double_digit_type divident = du_[j + n] * vlint::k_base + du_[j + n - 1];
		const double_digit_type divisor = dv_[n - 1];
		double_digit_type qhat = divident/divisor;
		double_digit_type rhat = divident % divisor;

		// Test qhat (errata value).
		if (qhat >= vlint::k_base
				|| (qhat * dv_[n - 2] > vlint::k_base * rhat + du_[j + n - 2])) {
			--qhat;
			rhat += dv_[n - 1];
			if (rhat < vlint::k_base
					&& (qhat >= vlint::k_base
						|| (qhat * dv_[n - 2] > vlint::k_base * rhat + du_[j + n - 2])))
				--qhat;
		}

		bool add_back = false;
		digit_type ks = 1;
		digit_type km = 0;
		magnitude_type replacement(n + 1);
		for (int i = 0; i <= n; ++i) {
			const double_digit_type qvi = i == n ? 0 : qhat * dv_[i];
			const double_digit_type current = du_[j + i]
					- (qvi % vlint::k_base)
					+ ks
					+ vlint::k_base
					- 1
					- km;
			replacement[i] = current % vlint::k_base;
			ks = current/vlint::k_base;
			km = qvi/vlint::k_base;
		}
		assert(km == 0);
		if (ks == 0)
			add_back = true;

		std::copy(replacement.begin(), replacement.end(), du_.begin() + j);

		q_[j] = qhat;
		if (add_back) {
			--q_[j];
			digit_type k = 0;
			for (int i = 0; i <= n; ++i) {
				const digit_type vi = i == n ? 0 : dv_[i];
				const double_digit_type current = vi + du_[i + j] + k;
				du_[i + j] = current % vlint::k_base;
				k = current/vlint::k_base;
			}
		}
	}
	vlint::remove_leading_zeros(du_);
	vlint::remove_leading_zeros(q_);
}

void vlint::shift_left_core(magnitude_type &w, std::size_t bits)
{
	if (bits >= k_bits_per_digit) {
		const int digits = bits/k_bits_per_digit;
		w.resize(w.size() + digits);

		int current = w.size() - 1;
		while (current >= digits) {
			w[current] = w[current - digits];
			w[current - digits] = 0;
			--current;
		}
		bits = bits % k_bits_per_digit;
	}

	if (!bits)
		return;

	digit_type mask = ~0;
	mask >>= bits;
	mask = ~mask;

	digit_type k = 0;
	for (size_type i = 0; i < w.size(); ++i) {
		digit_type current = w[i];
		digit_type following_k = current & mask;
		following_k >>= k_bits_per_digit - bits;
		current <<= bits;
		current |= k;
		w[i] = current;
		k = following_k;
	}
	if (k)
		w.push_back(k);
}

void vlint::shift_right_core(magnitude_type &w, std::size_t bits)
{
	if (bits >= k_bits_per_digit) {
		const size_type digits = bits/k_bits_per_digit;
		if  (digits >= w.size()) {
			w.clear();
			return;
		}

		size_type current = 0;
		while (current < w.size()) {
			if (current + digits < w.size())
				w[current] = w[current + digits];
			else
				w[current] = 0;
			++current;
		}
		remove_leading_zeros(w);
		bits = bits % k_bits_per_digit;
	}

	if (!bits)
		return;

	digit_type mask = ~0;
	mask <<= bits;
	mask = ~mask;

	digit_type k = 0;
	for (int i = w.size() - 1; i >= 0; --i) {
		digit_type current = w[i];
		digit_type following_k = current & mask;
		following_k <<= k_bits_per_digit - bits;
		current >>= bits;
		current |= k;
		w[i] = current;
		k = following_k;
	}
	remove_leading_zeros(w);
}

void vlint::remove_leading_zeros(magnitude_type &w)
{
	size_type i = w.size();
	for (; i > 0; --i)
		if (w[i - 1] != 0)
			break;
	if (i != w.size())
		w.resize(i);
}

void vlint::make_zero()
{
	sign_ = thezero;
	magnitude_.clear();
}

void vlint::check_zero()
{
	size_type i = 0;
	while (i < magnitude_.size() && magnitude_[i] == 0)
		++i;
	if (i == magnitude_.size())
		make_zero();
}

bool vlint::magnitude_one() const
{
	if (magnitude_.size() == 1 && magnitude_[0] == 1)
		return true;
	return false;
}

bool vlint::magnitude_less_than(const vlint &v) const
{
	if (magnitude_.size() < v.magnitude_.size())
		return true;
	else if (magnitude_.size() > v.magnitude_.size())
		return false;
	else {
		for (int i = magnitude_.size() - 1; i >= 0; --i) {
			if (magnitude_[i] < v.magnitude_[i])
				return true;
			else if (magnitude_[i] > v.magnitude_[i])
				return false;
		}
	}
	return false;
}

bool vlint::magnitude_equal_to(const vlint &v) const
{
	return magnitude_ == v.magnitude_;
}

vlint& vlint::operator+=(const vlint &other)
{
	if (other.zero())
		return *this;

	if (zero()) {
		magnitude_ = other.magnitude_;
		sign_ = other.sign_;
		return *this;
	}

	if (sign_ != other.sign_) {
		if (magnitude_equal_to(other)) {
			make_zero();
		} else {
			if (magnitude_less_than(other)) {
				magnitude_ = subtract_core(other.magnitude_, magnitude_);
				sign_ = other.sign_;
			} else {
				magnitude_ = subtract_core(magnitude_, other.magnitude_);
			}
			check_zero();
		}
	} else {
		magnitude_ = add_core(magnitude_, other.magnitude_);
	}

	return *this;
}

vlint& vlint::operator-=(const vlint &other)
{
	if (other.zero())
		return *this;

	if (zero()) {
		sign_ = other.sign_ == negative_values ? positive_values : negative_values;
		magnitude_ = other.magnitude_;
		return *this;
	}

	if (sign_ == other.sign_) {
		if (magnitude_equal_to(other)) {
			make_zero();
		} else {
			if (magnitude_less_than(other)) {
				magnitude_ = subtract_core(other.magnitude_, magnitude_);
				sign_ = other.sign_ == negative_values ? positive_values : negative_values;
			} else {
				magnitude_ = subtract_core(magnitude_, other.magnitude_);
			}
			check_zero();
		}
	} else {
		magnitude_ = add_core(magnitude_, other.magnitude_);
	}

	return *this;
}

vlint& vlint::operator*=(const vlint &other)
{
	if (zero() || other.zero()) {
		make_zero();
		return *this;
	}

	if (sign_ != other.sign_)
		sign_ = negative_values;
	else
		sign_ = positive_values;

	if (magnitude_one() || other.magnitude_one()) {
		if (magnitude_one())
			magnitude_ = other.magnitude_;
	} else {
		magnitude_ = multiply_core(magnitude_, other.magnitude_);
		check_zero();
	}

	return *this;
}

vlint& vlint::operator/=(const vlint &other)
{
	if (other.zero())
		throw std::invalid_argument("division by zero");

	if (zero() || magnitude_less_than(other)) {
		make_zero();
		return *this;
	}

	if (sign_ != other.sign_)
		sign_ = negative_values;
	else
		sign_ = positive_values;

	if (magnitude_equal_to(other)) {
		magnitude_.clear();
		magnitude_.push_back(1);
		return *this;
	}

	division_core helper(magnitude_, other.magnitude_);
	helper.divide();
	magnitude_ = helper.q_;
	check_zero();
	assert(!zero());

	return *this;
}

vlint& vlint::operator %=(const vlint &other)
{
	if (other.zero())
		throw std::invalid_argument("division by zero");

	if (magnitude_less_than(other))
		return *this;

	if (zero() || magnitude_equal_to(other)) {
		make_zero();
		return *this;
	}

	division_core helper(magnitude_, other.magnitude_);
	helper.divide();
	magnitude_ = helper.du_;
	shift_right_core(magnitude_, helper.shifts_);
	check_zero();

	return *this;
}

vlint& vlint::operator<<=(std::size_t bits)
{
	if (!zero())
		shift_left_core(magnitude_, bits);

	return *this;
}

vlint& vlint::operator>>=(std::size_t bits)
{
	if (!zero()) {
		shift_right_core(magnitude_, bits);
		check_zero();
	}

	return *this;
}

bool vlint::equal_to(const vlint &v) const
{
	if (sign_ == thezero)
		return v.sign_ == thezero;

	if (sign_ != v.sign_)
		return false;

	return magnitude_equal_to(v);
}

bool vlint::less_than(const vlint &v) const
{
	if (sign_ != v.sign_) {
		if (sign_ == negative_values)
			return true;
		return false;
	}

	if (sign_ == thezero)
		return false;

	if (sign_ == negative_values)
		return !magnitude_less_than(v);
	return magnitude_less_than(v);
}

bool vlint::zero() const
{
	return sign_ == thezero;
}

bool vlint::positive() const
{
	return sign_ == positive_values;
}

bool vlint::negative() const
{
	return sign_ == negative_values;
}

std::string vlint::to_string() const
{
	if (zero())
		return "0";

	std::ostringstream oss;
	if (sign_ == negative_values)
		oss << "-";

	if (magnitude_.size() == 1) {
		oss << magnitude_[0];
		return oss.str();
	}

	vlint divisor(k_power_of_10);
	vlint current = *this;
	std::list<std::string> decimal;
	while (!current.zero()) {
		std::ostringstream part;
		part.width(4);
		part.fill('0');
		vlint remainer = current % divisor;
		if (!remainer.zero())
			part << remainer.magnitude_[0];
		else
			part << 0;
		decimal.push_back(part.str());
		current = current/divisor;
	}
	std::reverse(decimal.begin(), decimal.end());
	assert(!decimal.empty());

	std::string &front = decimal.front();
	const std::size_t non_zero = front.find_first_not_of('0');
	if (non_zero > 0 && non_zero != std::string::npos)
		front.erase(front.begin(), front.begin() + non_zero);

	std::copy(decimal.begin(), decimal.end(), std::ostream_iterator<std::string>(oss));

	return oss.str();
}

vlint vlint::add(const vlint &other) const
{
	return *this + other;
}

vlint vlint::subtract(const vlint &other) const
{
	return *this - other;
}

vlint vlint::multiply(const vlint &other) const
{
	return *this * other;
}

vlint vlint::divide(const vlint &other) const
{
	return *this / other;
}

vlint vlint::modulo(const vlint &other) const
{
	return *this % other;
}

vlint vlint::shift_left(std::size_t bits) const
{
	return *this << bits;
}

vlint vlint::shift_right(std::size_t bits) const
{
	return *this >> bits;
}

std::ostream &operator<<(std::ostream &output, const vlint &num)
{
	output << num.to_string();
	return output;
}

vlint operator+(const vlint &a, const vlint &b)
{
	vlint t(a);
	t += b;
	return t;
}

vlint operator-(const vlint &a, const vlint &b)
{
	vlint t(a);
	t -= b;
	return t;
}

vlint operator*(const vlint &a, const vlint &b)
{
	vlint t(a);
	t *= b;
	return t;
}

vlint operator/(const vlint &a, const vlint &b)
{
	vlint t(a);
	t /= b;
	return t;
}

vlint operator%(const vlint &a, const vlint &b)
{
	vlint t(a);
	t %= b;
	return t;
}

vlint operator<<(const vlint &a, std::size_t bits)
{
	vlint t(a);
	t <<= bits;
	return t;
}

vlint operator>>(const vlint &a, std::size_t bits)
{
	vlint t(a);
	t >>= bits;
	return t;
}

vlint& vlint::operator++()
{
	operator +=(one);
	return *this;
}

vlint vlint::operator++(int)
{
	vlint t(*this);
	operator +=(one);
	return t;
}

vlint& vlint::operator--()
{
	operator -=(one);
	return *this;
}

vlint vlint::operator--(int)
{
	vlint t(*this);
	operator -=(one);
	return t;
}

bool operator<(const vlint& a, const vlint &b)
{
	return a.less_than(b);
}

bool operator==(const vlint &a, const vlint &b)
{
	return a.equal_to(b);
}

bool operator!=(const vlint &a, const vlint &b)
{
	return !(a == b);
}

