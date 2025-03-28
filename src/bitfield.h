#ifndef BITFIELD_H
#define CONFIG_H

#include <stdint.h>
#include <vector>
#include <string.h>
#ifndef __ICC
#include <unordered_map>
#else
#include <hash_map>
#endif /* __ICC */
#include <assert.h>
#include <string>
#include <limits.h>
using namespace std;

#define BitfieldTypeIs64
#define USE_BITSCAN

#ifdef BitfieldTypeIs64
#ifdef _MSC_VER
#include <immintrin.h>
#  define MyPopcount __popcnt64
#  define MyCTZ _tzcnt_u64
#  define MyCLZ _lzcnt_u64
#  define MyWord unsigned __int64
#else
#  define MyPopcount __builtin_popcountll
#  define MyCTZ __builtin_ctzll
#  define MyCLZ __builtin_clzll
#  define MyWord unsigned long long
#endif
#else //of BitfieldTypeIs64
#ifdef _MSC_VER
#include <immintrin.h>
#  define MyPopcount __popcnt
#  define MyCTZ _tzcnt_u32
#  define MyCLZ _lzcnt_u32
#  define MyWord unsigned int
#else
#  define MyPopcount __builtin_popcount
#  define MyCTZ __builtin_ctz
#  define MyCLZ __builtin_clz
#  define MyWord unsigned int
#endif
#endif //of BitfieldTypeIs64


/**Only working correctly if size is a multiple of of _bitsPerWord!!!*/

/** Representation of a BitField */
template<size_t size> class BitField {
private:
	/** Definition of underlying type for the bitfield*/
	typedef MyWord _word;

	/** Number of bits per word */
	static const size_t _bitsPerWord = CHAR_BIT * sizeof(_word);

	/** Number of words for bitfield<size> */
	static const size_t _words = (size + _bitsPerWord - 1) / _bitsPerWord;

	/** Word offset for a given position */
	static size_t _wordOffset(size_t pos) {
		return pos / _bitsPerWord;
	}

	/** Bit offset offset for a given position */
	static size_t _bitOffset(size_t pos) {
		return pos % _bitsPerWord;
	}

	/** Bitmask for pos in a word */
	static _word _bitmask(size_t pos) {
		return ((_word)1) << _bitOffset(pos);
	}

	/** Count trailing zeroes
	* @remarks: Returns undefined if w == 0
	*/
	static size_t _bitscan(_word w) {
		return MyCTZ(w);
	}

	static size_t _popcount(_word v) {
		return MyPopcount(v);
	}

	/** Underlying word array */
	_word _bits[_words];
public:
	/** Create a new empty bitfield */
	BitField() {
		assert(_bitOffset(size) == 0);
		reset();
	}
	/** Copy create a bitfield from another */
	BitField(const BitField<size>& bf) {
		for (size_t i = 0; i < _words; i++)
			_bits[i] = bf._bits[i];
	}

	/** Clear all bits */
	BitField<size>& reset();
	/** Clear bit at position pos */
	BitField<size>& reset(size_t pos);
	/** Test bit at position pos */
	bool test(size_t pos) const;
	/** Set all bits */
	BitField<size>& set();
	/** Set bit a position pos */
	BitField<size>& set(size_t pos);
	/** Set bit a position pos to value */
	BitField<size>& set(size_t pos, bool value);
	/** Flip all bits */
	BitField<size>& flip();
	/** Flip bit at position pos */
	BitField<size>& flip(size_t pos);
	/** Test if any bit is set */
	bool any() const;
	/** Test if no bits are set */
	bool none() const;
	/** Count the number of high bits */
	size_t count() const;
	/** Get the first set bit, only for BitFields that fit one word */
	size_t singleWordFirst() const;
	/** Get the last set bit, only for BitFields that fit one word */
	size_t singleWordLast() const;

	BitField<size>& operator&= (const BitField<size>& rhs);
	BitField<size>& operator|= (const BitField<size>& rhs);
	BitField<size>& operator^= (const BitField<size>& rhs);

	BitField<size> operator~() const;

	bool operator== (const BitField<size>& rhs) const;
	bool operator!= (const BitField<size>& rhs) const;

	/** Iterator over the high bits in a BitField */
	class BitIterator {
	private:
		const BitField<size>& _bf;
		size_t _pos;
	public:
		BitIterator(const BitField<size>& bf) : _bf(bf) {
			reset();
		}
		bool operator++(int) {
#ifdef USE_BITSCAN
			_pos++;
			if (_pos >= size)
				return false;
			_word w = _bf._bits[_wordOffset(_pos)];
			//Discard bits lower than _pos
			w &= (~(_word)0) << _bitOffset(_pos);
			//Check the first word
			if (w != (_word)0) {
				_pos = _bitscan(w) + _wordOffset(_pos) * _bitsPerWord;
				return true;
			}
			//Check all the following words
			for (size_t i = _wordOffset(_pos) + 1; i < _words; i++) {
				w = _bf._bits[i];
				if (w != 0) {
					_pos = _bitscan(w) + i * _bitsPerWord;
					return true;
				}
			}
			_pos = -1;
			return false;
#else
			do
				_pos++;
			while (_pos < size && !_bf.test(_pos));
			return _pos < size;
#endif /* USE_BITSCAN */
		}
		size_t operator*() {
			return _pos;
		}
		/** Reset the BitIterator */
		void reset() {
			_pos = -1;
		}
		/** Set the BitIterator to some position */
		void set(size_t pos) {
			assert(pos < size);
			_pos = pos;
		}
	};

#ifndef __ICC
	template<typename> friend struct std::hash;
#else
	template<size_t> friend struct BitFieldHasher;
#endif /* __ICC */
};

#ifndef __ICC

/** Specialization of unordered_map for mapping from BitField to value */
template<size_t size, typename value>
struct BitFieldMap : public std::unordered_map<BitField<size>, value, std::hash< BitField<size> > > {};

#else

/** Creates a hash of a bitfield */
template<size_t size>
struct BitFieldHasher {
	size_t operator()(const BitField<size>& bf) const {
		size_t hash = 0;
		for (size_t i = 0; i < BitField<size>::_words; i++)
			hash ^= bf._bits[i];
		return hash;
	}
};
/** Specialization of hash_map for mapping from BitField to value*/
template<size_t size, typename value>
struct BitFieldMap : public __gnu_cxx::hash_map < BitField<size>, value, BitFieldHasher<size> > {};

#endif /* __ICC */


template<size_t size>
inline BitField<size> operator&(const BitField<size>& lhs, const BitField<size>& rhs) {
	BitField<size> retval(lhs);
	retval &= rhs;
	return retval;
}

template<size_t size>
inline BitField<size> operator|(const BitField<size>& lhs, const BitField<size>& rhs) {
	BitField<size> retval(lhs);
	retval |= rhs;
	return retval;
}

template<size_t size>
inline BitField<size> operator^(const BitField<size>& lhs, const BitField<size>& rhs) {
	BitField<size> retval(lhs);
	retval ^= rhs;
	return retval;
}

template<size_t size>
inline BitField<size>& BitField<size>::reset() {
	memset(_bits, 0, sizeof(_word) * _words);
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::reset(size_t pos) {
	assert(pos < size);
	_bits[_wordOffset(pos)] &= ~_bitmask(pos);
	return *this;
}

template<size_t size>
inline bool BitField<size>::test(size_t pos) const {
	assert(pos < size);
	return (_bits[_wordOffset(pos)] & _bitmask(pos)) != (_word)0;
}

//This is only working if size is a multiple of _bitsPerWord; then no sanitizing is necessary
template<size_t size>
inline BitField<size>& BitField<size>::set() {
	memset(_bits, 0xff, sizeof(_word) * _words);
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::set(size_t pos) {
	assert(pos < size);
	_bits[_wordOffset(pos)] |= _bitmask(pos);
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::set(size_t pos, bool value) {
	assert(pos < size);
	if (value)
		set(pos);
	else
		reset(pos);
	return *this;
}

//This is only working if size is a multiple of _bitsPerWord; then no sanitizing is necessary
template<size_t size>
inline BitField<size>& BitField<size>::flip() {
	for (size_t i = 0; i < _words; i++)
		_bits[i] = ~_bits[i];
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::flip(size_t pos) {
	assert(pos < size);
	_bits[_wordOffset(pos)] ^= _bitmask(pos);
	return *this;
}

template<size_t size>
inline bool BitField<size>::any() const {
	for (size_t i = 0; i < _words; i++)
		if (_bits[i] != (_word)0)
			return true;
	return false;
}

template<size_t size>
inline bool BitField<size>::none() const {
	_word tmp = (_word)0;
	for (size_t i = 0; i < _words; i++)
		tmp |= _bits[i];
	return tmp == (_word)0;
}

template<size_t size>
inline size_t BitField<size>::count() const {
	size_t count = 0;
	for (size_t i = 0; i < _words; i++)
		count += _popcount(_bits[i]);
	return count;
}

template<size_t size>
inline size_t BitField<size>::singleWordFirst() const {
	assert(_words <= 1);
	return MyCTZ(_bits[0]);
}

template<size_t size>
inline size_t BitField<size>::singleWordLast() const {
	assert(_words <= 1);
	return _bitsPerWord - MyCLZ(_bits[0]) - 1;
}

template<size_t size>
inline BitField<size>& BitField<size>::operator&=(const BitField<size>& rhs) {
	for (size_t i = 0; i < _words; i++)
		_bits[i] &= rhs._bits[i];
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::operator|=(const BitField<size>& rhs) {
	for (size_t i = 0; i < _words; i++)
		_bits[i] |= rhs._bits[i];
	return *this;
}

template<size_t size>
inline BitField<size>& BitField<size>::operator^=(const BitField<size>& rhs) {
	for (size_t i = 0; i < _words; i++)
		_bits[i] ^= rhs._bits[i];
	return *this;
}

template<size_t size>
inline BitField<size> BitField<size>::operator~() const {
	return BitField<size>(*this).flip();
}

template<size_t size>
bool BitField<size>::operator==(const BitField<size>& rhs) const {
	for (size_t i = 0; i < _words; i++)
		if (_bits[i] != rhs._bits[i])
			return false;
	return true;
}

template<size_t size>
bool BitField<size>::operator!=(const BitField<size>& rhs) const {
	for (size_t i = 0; i < _words; i++)
		if (_bits[i] != rhs._bits[i])
			return true;
	return false;
}

#endif /* BITFIELD_H */