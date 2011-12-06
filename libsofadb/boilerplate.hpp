//boilerplate.hpp
#ifndef BOILERPLATE_HPP
#define BOILERPLATE_HPP

#include <boost/preprocessor.hpp>

#define UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR_BASE(pR, pData, pBase) pBase(std::move(pOther))
#define UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT_BASE(pR, pData, pBase) pBase::operator=(std::move(pOther));

#define UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR(pR, pData, pMember) pMember(std::move(pOther.pMember))
#define UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT(pR, pData, pMember) pMember = std::move(pOther.pMember);

#define UTILITY_MOVE_DEFAULT_DETAIL(pT, pBases, pMembers) \
	pT(pT&& pOther) : \
	BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
		UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR_BASE, BOOST_PP_EMPTY, pBases)), \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
			UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR, BOOST_PP_EMPTY, pMembers)) \
			{} \
	pT& operator=(pT&& pOther) \
	{ \
		if (this == &pOther) return *this; \
		BOOST_PP_SEQ_FOR_EACH(UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT_BASE, BOOST_PP_EMPTY, pBases) \
		BOOST_PP_SEQ_FOR_EACH(UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT, BOOST_PP_EMPTY, pMembers) \
		return *this; \
	}

#define UTILITY_MOVE_DEFAULT_BASES_DETAIL(pT, pBases) \
	pT(pT&& pOther) : \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
		UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR_BASE, BOOST_PP_EMPTY, pBases)) \
	{} \
	pT& operator=(pT&& pOther) \
	{ \
		if (this == &pOther) return *this; \
		BOOST_PP_SEQ_FOR_EACH(UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT_BASE, BOOST_PP_EMPTY, pBases) \
		return *this; \
	}

#define UTILITY_MOVE_DEFAULT_MEMBERS_DETAIL(pT, pMembers) \
	pT(pT&& pOther) : \
		BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
			UTILITY_MOVE_DEFAULT_DETAIL_CONSTRUCTOR, BOOST_PP_EMPTY, pMembers)) \
	{} \
	pT& operator=(pT&& pOther) \
	{ \
		if (this == &pOther) return *this; \
		BOOST_PP_SEQ_FOR_EACH(UTILITY_MOVE_DEFAULT_DETAIL_ASSIGNMENT, BOOST_PP_EMPTY, pMembers) \
		return *this; \
	}

// move bases and members
#define UTILITY_MOVE_DEFAULT(pT, pBases, pMembers) UTILITY_MOVE_DEFAULT_DETAIL(pT, pBases, pMembers)
// base only version
#define UTILITY_MOVE_DEFAULT_BASES(pT, pBases) UTILITY_MOVE_DEFAULT_BASES_DETAIL(pT, pBases)
// member only version
#define UTILITY_MOVE_DEFAULT_MEMBERS(pT, pMembers) UTILITY_MOVE_DEFAULT_MEMBERS_DETAIL(pT, pMembers)

#endif //BOILERPLATE_HPP
