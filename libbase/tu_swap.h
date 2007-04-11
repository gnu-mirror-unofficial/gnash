// tu_swap.h	-- Ignacio Casta�o, Thatcher Ulrich 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Basic endian-swapping stuff.


#ifndef TU_SWAP_H
#define TU_SWAP_H


#include "tu_config.h"
#include "tu_types.h"


template<class T>
void	swap(T* a, T* b)
// Convenient swap function.
{
	T	temp(*a);
	*a = *b;
	*b = temp;
}


//
// endian conversions
//

#ifdef swap16
#undef swap16
#endif

inline uint16_t swap16(uint16_t u)
{ 
	return ((u & 0x00FF) << 8) | 
		((u & 0xFF00) >> 8);
}

#ifdef swap32
#undef swap32
#endif
inline uint32_t swap32(uint32_t u)
{ 
	return ((u & 0x000000FF) << 24) | 
		((u & 0x0000FF00) << 8)  |
		((u & 0x00FF0000) >> 8)  |
		((u & 0xFF000000) >> 24);
}

#ifdef swap64
#undef swap64
#endif
inline uint64_t swap64(uint64_t u)
{
#ifdef __GNUC__
	return ((u & 0x00000000000000FFLL) << 56) |
		((u & 0x000000000000FF00LL) << 40)  |
		((u & 0x0000000000FF0000LL) << 24)  |
		((u & 0x00000000FF000000LL) << 8) |
		((u & 0x000000FF00000000LL) >> 8) |
		((u & 0x0000FF0000000000LL) >> 24) |
		((u & 0x00FF000000000000LL) >> 40) |
		((u & 0xFF00000000000000LL) >> 56);
#else
	return ((u & 0x00000000000000FF) << 56) | 
		((u & 0x000000000000FF00) << 40)  |
		((u & 0x0000000000FF0000) << 24)  |
		((u & 0x00000000FF000000) << 8) |
		((u & 0x000000FF00000000) >> 8) |
		((u & 0x0000FF0000000000) >> 24) |
		((u & 0x00FF000000000000) >> 40) |
		((u & 0xFF00000000000000) >> 56);
#endif
}


inline uint64_t	swap_le64(uint64_t le_64)
// Given a 64-bit little-endian piece of data, return it as a 64-bit
// integer in native endian-ness.  I.e., do a swap if we're on a
// big-endian machine.
{
#ifdef _TU_LITTLE_ENDIAN_
	return le_64;
#else	// not _TU_LITTLE_ENDIAN_
	return swap64(le_64);	// convert to big-endian.
#endif	// not _TU_LITTLE_ENDIAN_
}


inline uint32_t	swap_le32(uint32_t le_32)
// Given a 32-bit little-endian piece of data, return it as a 32-bit
// integer in native endian-ness.  I.e. on a little-endian machine,
// this just returns the input; on a big-endian machine, this swaps
// the bytes around first.
{
#ifdef _TU_LITTLE_ENDIAN_
	return le_32;
#else	// not _TU_LITTLE_ENDIAN_
	return swap32(le_32);	// convert to big-endian.
#endif	// not _TU_LITTLE_ENDIAN_
}


inline uint16_t	swap_le16(uint16_t le_16)
// Given a 16-bit little-endian piece of data, return it as a 16-bit
// integer in native endianness.
{
#ifdef _TU_LITTLE_ENDIAN_
	return le_16;
#else	// not _TU_LITTLE_ENDIAN_
	return swap16(le_16);	// convert to big-endian.
#endif	// not _TU_LITTLE_ENDIAN_
}


inline uint32_t	swap_be32(uint32_t le_32)
// Given a 32-bit big-endian piece of data, return it as a 32-bit
// integer in native endian-ness.  I.e. on a little-endian machine,
// this swaps the bytes around; on a big-endian machine, it just
// returns the input.
{
#ifdef _TU_LITTLE_ENDIAN_
	return swap32(le_32);	// convert to little-endian.
#else	// not _TU_LITTLE_ENDIAN_
	return le_32;
#endif	// not _TU_LITTLE_ENDIAN_
}


inline uint16_t	swap_be16(uint16_t le_16)
// Given a 16-bit big-endian piece of data, return it as a 16-bit
// integer in native endianness.
{
#ifdef _TU_LITTLE_ENDIAN_
	return swap16(le_16);	// convert to little-endian.
#else	// not _TU_LITTLE_ENDIAN_
	return le_16;
#endif	// not _TU_LITTLE_ENDIAN_
}



#endif // TU_SWAP_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
