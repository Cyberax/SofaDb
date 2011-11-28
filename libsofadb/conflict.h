#ifndef CONFLICT_H
#define CONFLICT_H

#include "common.h"
#include "database.h"

namespace sofadb {

	class resolver
	{
		json_value history_;
	public:
		resolver(json_value &&history) :
			history_(std::move(history)) {}
		resolver();

		static bool is_left_rev_winning(
			const revision_num_t &left, const revision_num_t &right);

		void merge(const revision_t &rev_info_);
	};
};

#endif //CONFLICT_H
