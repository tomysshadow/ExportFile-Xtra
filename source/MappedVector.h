#pragma once
#include <stdexcept>
#include <map>
#include <vector>

/*
this is a pattern I often use in my tools
such as Old CPU Simulator, Aft3rMARK etc.
and finally made into its own class
sometimes I want a vector where all items are unique
"just use std::set," you say! But what if
I want the order of the items to be preserved?
adding items {1, 3, 2, 4} to a std::set will
reorganize them to {1, 2, 3, 4}, and
std::unordered_set is only even more unorderly!
using only a regular std::vector would mean we'd have to
loop the entire thing every time to check for duplicates
so, we could pair together a std::unordered_set and std::vector
items are added to both, if it's in the set, it's in the vector
but then, to get the position of the item in the vector, we
have to look it up in both seperately
so the proper solution is a std::map and a std::vector
the former redundantly stores each value as its keys
and the corresponding indices in the vector as its values
usage of this class is similar (but not identical) to
typical STL containers
push_back is just push. There is no way to push to the front
it would break the indexing, and generally I don't need it anyway
push returns true or false based on whether the value was added to
the end or was already in the mapped vector. You can also optionally
get the index of the value you just added
pop_back is just pop. It BOTH gets the value and erases the element
(as God intended)
you can get elements with [] operator. But you
can't erase them - only pop, and you can't set them - use replace instead
*/
template <typename ValueType, typename Comparer = std::less<ValueType>> class MappedVector {
	private:
	using MAPPED_VECTOR = MappedVector<ValueType, Comparer>;
	using INITIALIZER_LIST = std::initializer_list<ValueType>;

	public:
	using VECTOR = std::vector<ValueType>;
	using SIZE_TYPE = typename VECTOR::size_type;
	using CONST_ITERATOR = typename VECTOR::const_iterator;

	static const SIZE_TYPE NPOS = -1;

	private:
	using MAP = std::map<ValueType, SIZE_TYPE, Comparer>;

	MAP _map = {};
	VECTOR _vector = {};

	public:
	SIZE_TYPE findIndex(const ValueType &value) const {
		typename MAP::const_iterator mapIterator = _map.find(value);

		if (mapIterator == _map.cend()) {
			return NPOS;
		}
		return mapIterator->second;
	}

	CONST_ITERATOR findIterator(const ValueType &value) const {
		SIZE_TYPE index = findIndex(value);
		
		if (index != NPOS) {
			return _vector.cbegin() + index;
		}
		return _vector.cend();
	}

	bool find(const ValueType &value) const {
		return findIndex(value) != NPOS;
	}

	bool front(ValueType &value) const {
		if (_vector.empty()) {
			value = {};
			return false;
		}

		value = _vector.front();
		return true;
	}

	bool back(ValueType &value) const {
		if (_vector.empty()) {
			value = {};
			return false;
		}

		value = _vector.back();
		return true;
	}

	// allow getting const iterators only
	// to modify the values, the other more specific methods must be used instead
	CONST_ITERATOR cbegin() const {
		return _vector.cbegin();
	}

	CONST_ITERATOR cend() const {
		return _vector.cend();
	}

	bool empty() const {
		return _vector.empty();
	}

	SIZE_TYPE size() const {
		return _vector.size();
	}

	void clear() {
		_map.clear();
		_vector.clear();
	}

	// the return value here indicates whether the value was
	// added to the vector (true) or already in it (false)
	// it is not an error indication and shouldn't be replaced with an exception
	// the intention is that you can still get the index/iterator if it was
	// already in the vector, saving you a call to find if you
	// still wanted that information in that scenario
	bool push(const ValueType &value, SIZE_TYPE &index) {
		index = findIndex(value);

		if (index != NPOS) {
			return false;
		}

		index = _vector.size();
		_map[value] = index;
		_vector.push_back(value);
		return true;
	}

	bool push(const ValueType &value, CONST_ITERATOR &iterator) {
		iterator = findIterator(value);

		if (iterator != _vector.cend()) {
			return false;
		}

		_map[value] = _vector.size();
		_vector.push_back(value);
		iterator = _vector.back();
		return true;
	}

	bool push(const ValueType &value) {
		SIZE_TYPE index = NPOS;
		return push(value, index);
	}

	bool pop(ValueType &value) {
		if (_vector.empty()) {
			value = {};
			return false;
		}

		value = _vector.back();
		_map.erase(value);
		_vector.pop_back();
		return true;
	}

	// this is implemented using an index
	// because we need to insert one into the map anyway
	// (the map needs to use indices to avoid iterator invalidation on vector erase)
	bool replace(const ValueType &value, SIZE_TYPE index) {
		if (index >= _vector.size()) {
			throw std::invalid_argument("index must not be greater or equal to Size");
		}

		// don't allow a duplicate value to get inserted at a different index
		if (find(value)) {
			return false;
		}

		_map.erase(_vector[index]);
		_map[value] = index;
		_vector[index] = value;
		return true;
	}

	bool replace(const ValueType &value, CONST_ITERATOR iterator) {
		return replace(value, iterator - _vector.cbegin());
	}

	// this is implemented using an iterator
	// because it's the only way to know the next element after the one we erased
	// and we need that information in order to correct the map
	// these methods have iterator and index in the name to differentiate them from erase
	// (what if ValueType is an iterator or index?)
	CONST_ITERATOR eraseIterator(CONST_ITERATOR &firstIterator, CONST_ITERATOR &lastIterator) {
		// having an iterator outside of the vector is undefined behaviour
		// we don't check for that, since the UB has already been "committed" in that case
		// so, we shouldn't try and make sense of that
		// however the compiler has no way of knowing I expect the firstIterator to be
		// before the lastIterator here, so I do check this to avoid smashing the stack
		if (lastIterator < firstIterator) {
			throw std::invalid_argument("lastIterator must not be less than firstIterator");
		}

		for (CONST_ITERATOR vectorIterator = firstIterator; vectorIterator != lastIterator; vectorIterator++) {
			_map.erase(*vectorIterator);
		}

		// first will be last...
		SIZE_T index = lastIterator - firstIterator;
		CONST_ITERATOR iterator = _vector.erase(firstIterator, lastIterator);

		// may be zero if first and last are the same (which is valid)
		if (index) {
			for (CONST_ITERATOR vectorIterator = iterator; vectorIterator != _vector.cend(); vectorIterator++) {
				_map[*vectorIterator] -= index;
			}
		}
		return iterator;
	}

	CONST_ITERATOR eraseIterator(CONST_ITERATOR &iterator) {
		// would cause undefined behaviour if allowed to continue
		if (iterator == _vector.cend()) {
			throw std::invalid_argument("iterator must not be equal to End Iterator");
		}
		return eraseIterator(iterator, iterator + 1);
	}

	MappedVector &eraseIndex(SIZE_TYPE firstIndex, SIZE_TYPE lastIndex) {
		if (lastIndex < firstIndex) {
			throw std::invalid_argument("lastIndex must not be less than firstIndex");
		}

		if (lastIndex >= _vector.size()) {
			throw std::invalid_argument("lastIndex must not be greater or equal to Size");
		}

		CONST_ITERATOR beginIterator = _vector.cbegin();
		eraseIterator(beginIterator + firstIndex, beginIterator + lastIndex);
		return *this;
	}

	MappedVector &eraseIndex(SIZE_TYPE index) {
		if (index >= _vector.size()) {
			throw std::invalid_argument("index must not be greater or equal to Size");
		}

		eraseIterator(_vector.cbegin() + index);
		return *this;
	}

	// find and erase a value in the mapped vector
	// this can be better optimized when written as its own implementation
	// so this doesn't use the other overloads
	// once again this returns true if the value was erased, false otherwise
	bool erase(const ValueType &value) {
		typename MAP::const_iterator mapIterator = _map.find(value);

		if (mapIterator == _map.cend()) {
			return false;
		}

		for (CONST_ITERATOR vectorIterator = _vector.erase(_vector.cbegin() + mapIterator->second); vectorIterator != _vector.cend(); vectorIterator++) {
			_map[*vectorIterator]--;
		}

		_map.erase(mapIterator);
		return true;
	}

	// these return the MappedVector reference to allow use with a temporary
	MappedVector &concat(const VECTOR &concatVector) {
		for (CONST_ITERATOR concatVectorIterator = concatVector.cbegin(); concatVectorIterator != concatVector.cend(); concatVectorIterator++) {
			push(*concatVectorIterator);
		}
		return *this;
	}

	MappedVector &concat(const INITIALIZER_LIST &concatInitializerList) {
		return concat(VECTOR(concatInitializerList));
	}

	MappedVector &concat(const MAPPED_VECTOR &concatMappedVector) {
		return concat(concatMappedVector._vector);
	}

	MappedVector &difference(const VECTOR &differenceVector) {
		for (CONST_ITERATOR differenceVectorIterator = differenceVector.cbegin(); differenceVectorIterator != differenceVector.cend(); differenceVectorIterator++) {
			erase(*differenceVectorIterator);
		}
		return *this;
	}

	MappedVector &difference(const INITIALIZER_LIST &differenceInitializerList) {
		return difference(VECTOR(differenceInitializerList));
	}

	MappedVector &difference(const MAPPED_VECTOR &differenceMappedVector) {
		return difference(differenceMappedVector._vector);
	}

	MappedVector() {
	}

	// we don't need a copy constructor that takes in this class
	// (this class only uses RAII types so can be safely copied with the default behaviour)
	MappedVector(const VECTOR &copyVector) {
		concat(copyVector);
	}

	MappedVector(const INITIALIZER_LIST &copyInitializerList) {
		concat(copyInitializerList);
	}

	MappedVector &operator=(const VECTOR &assignVector) {
		clear();
		return concat(assignVector);
	}

	MappedVector &operator=(const INITIALIZER_LIST &assignInitializerList) {
		clear();
		return concat(assignInitializerList);
	}

	MappedVector &operator+=(const VECTOR &addVector) {
		return concat(addVector);
	}

	MappedVector &operator+=(const INITIALIZER_LIST &addInitializerList) {
		return concat(addInitializerList);
	}

	MappedVector &operator+=(const MAPPED_VECTOR &addMappedVector) {
		return concat(addMappedVector);
	}

	MappedVector &operator-=(const VECTOR &subtractVector) {
		return difference(subtractVector);
	}

	MappedVector &operator-=(const INITIALIZER_LIST &subtractInitializerList) {
		return difference(subtractInitializerList);
	}

	MappedVector &operator-=(const MAPPED_VECTOR &subtractMappedVector) {
		return difference(subtractMappedVector);
	}

	// these are const as to not allow modifying vector values without map values
	const ValueType &operator[](SIZE_TYPE index) const {
		return _vector[index];
	}

	const VECTOR &get() const {
		return _vector;
	}
};

// these are intentionally outside the class to allow for type conversions when adding/subtracting
template <typename ValueType>
inline MappedVector<ValueType> operator+(const MappedVector<ValueType> &addMappedVector, const MappedVector<ValueType> &addMappedVector2) {
	return MappedVector<ValueType>(addMappedVector).concat(addMappedVector2);
}

template <typename ValueType>
inline MappedVector<ValueType> operator-(const MappedVector<ValueType> &subtractMappedVector, const MappedVector<ValueType> &subtractMappedVector2) {
	return MappedVector<ValueType>(subtractMappedVector).difference(subtractMappedVector2);
}