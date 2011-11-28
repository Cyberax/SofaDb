#include "conflict.h"

using namespace sofadb;

/** Yes, that's the whole conflict resolution protocol. It's that simple.
  */
bool resolver::is_left_rev_winning(const revision_num_t &left,
	const revision_num_t &right)
{
	assert(!left.empty() && !right.empty());

	if (left.num() != right.num())
		return left.num() > right.num();

	size_t lsz = left.uniq().size();
	size_t rsz = right.uniq().size();
	if (lsz != rsz)
		return lsz < rsz;

	return left.uniq() < right.uniq();
}
