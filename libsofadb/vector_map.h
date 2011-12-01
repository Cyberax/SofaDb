#ifndef VECTOR_MAP
#define VECTOR_MAP

#include <vector>

namespace utils {

	template<class K, class V> class vector_map
	{
		typedef typename std::pair<K,V> pair_t;
		typedef typename std::vector<pair_t> vec_t;
		vec_t values_;
	public:
		typedef typename vec_t::const_iterator const_iterator;
		typedef typename vec_t::iterator iterator;

		vector_map() {}
		vector_map(const vector_map &other) : values_(other.values_) {}
		vector_map(vector_map &&other) : values_(std::move(other.values_)) {}
		vector_map& operator = (const vector_map& other)
		{
			values_ = other.values_;
			return *this;
		}
		vector_map& operator = (vector_map&& other)
		{
			if (this == &other) return *this;
			values_ = std::move(other.values_);
			return *this;
		}

		void reserve(size_t sz)
		{
			values_.reserve(sz);
		}

		pair_t& insert(pair_t && pair)
		{
			for(auto iter=begin(), iend=end(); iter!=iend; ++iter)
				if (iter->first == pair.first)
				{
					iter->second = std::move(pair.second);
					return (*iter);
				}
			values_.push_back(std::move(pair));
			return values_.back();
		}
		V& operator [] (const K& key)
		{
			for(auto iter=begin(), iend=end(); iter!=iend; ++iter)
				if (iter->first == key)
					return iter->second;

			values_.push_back(std::make_pair(key, V()));
			return values_.back().second;
		}

		const V& at(const K& key) const
		{
			for(auto iter=begin(), iend=end(); iter!=iend; ++iter)
				if (iter->first == key)
					return iter->second;
			throw std::out_of_range("No key found");
		}

		iterator begin() { return values_.begin(); }
		iterator end() { return values_.end(); }
		const_iterator begin() const { return values_.begin(); }
		const_iterator end() const { return values_.end(); }

	};

}; //namespace utils

#endif //VECTOR_MAP

