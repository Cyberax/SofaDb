#ifndef CONFLICT_H
#define CONFLICT_H

#include "common.h"
#include "database.h"

namespace sofadb {
	typedef std::vector<revision_num_t> revision_list_t;

	class resolver
	{
		json_value rev_log_;
	public:
		resolver(rev_log_ &&rev_log) :
			rev_log_(std::move(rev_log)) {}

		static bool is_left_rev_winning(
			const revision_num_t &left, const revision_num_t &right);

		void merge(const revision_num_t &rev_num_,
				   const sublist_t &current_conflicts);
	};
};

#endif //CONFLICT_H
