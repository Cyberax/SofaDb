#include "conflict.h"
#include <set>
#include <functional>

using namespace sofadb;

typedef std::reference_wrapper<const jstring_t> jstring_ref_t;

namespace std {
	template<> struct less<jstring_ref_t>
	{
		bool operator()(const jstring_ref_t& l, const jstring_ref_t& r) const
		{
			return l.get() < r.get();
		}
	};
}; //namespace std

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


void resolver::merge(const revision_num_t &new_rev,
		   const sublist_t &new_rev_conflicts)
{
	revlog_wrapper wrapper(*rev_log_);
	revision_num_t cur_rev = wrapper.top_rev_id();
	sublist_t& conflicts=wrapper.get_conflicts();

	//Reserve space
	conflicts.reserve(conflicts.size()+new_rev_conflicts.size()+2);

	//We use references to optimize the checking for visited strings.
	//It's safe because 'conflicts' sublist is guaranteed not to be
	//reallocated as we've reserved enough space.
	std::set<jstring_ref_t> visited;
	for(auto i=conflicts.begin(), iend=conflicts.end(); i!=iend; ++i)
		visited.insert(std::cref(i->get_str()));

	//Merge conflict lists
	for(auto i=new_rev_conflicts.begin(), iend=new_rev_conflicts.end();
		i!=iend; ++i)
	{
		const jstring_t &new_conf_str=i->get_str();
		if (visited.find(std::cref(new_conf_str))!=visited.end())
			continue; //The conflict is already there - skip it
		conflicts.push_back(new_conf_str);
		visited.insert(std::cref(conflicts.back().get_str()));
	}

	const std::string &new_rev_str=new_rev.full_string();
	const std::string &cur_rev_str=cur_rev.full_string();

	//First, check if we're losing
	if (is_left_rev_winning(cur_rev, new_rev))
	{
		//The current revision is winning, so just add the new revision
		//and its conflicts to the current list of conflicts

		//Since the current version is winning it can't be in the
		//list of conflicts.
		assert(visited.find(cur_rev_str)==visited.end());

		//Add the new version to the list of conflicts. It can already be
		//there if we're doing a repeated merge.
		if (visited.find(new_rev_str)==visited.end())
			conflicts.push_back(new_rev_str);
	} else
	{
		//The new revision is winning, make the current revision
		//to be conflicting.
		wrapper.add_rev_info(new_rev, true, false);

		//The new version can't be amongst the conflicts!
		assert(visited.find(new_rev_str)==visited.end());

		//Add the new version to the list of conflicts. It can already be
		//there if we're doing a repeated merge.
		if (visited.find(cur_rev_str)==visited.end())
			conflicts.push_back(cur_rev_str);
	}

	//That's all, folks!
}
