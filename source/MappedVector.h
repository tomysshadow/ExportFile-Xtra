#pragma once
#include <stdexcept>
#include <map>
#include <vector>
#include <initializer_list>

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
	using Type = MappedVector<ValueType, Comparer>;
	using InitializerList = std::initializer_list<ValueType>;

	public:
	using Vector = std::vector<ValueType>;
	using SizeType = typename Vector::size_type;
	using ConstIterator = typename Vector::const_iterator;

	static constexpr SizeType NPOS = (SizeType)-1;

	private:
	using Map = std::map<ValueType, SizeType, Comparer>;

	Map _map = {};
	Vector _vector = {};

	public:
	SizeType findIndex(const ValueType &value) const {
		auto mapIterator = _map.find(value);

		if (mapIterator == _map.cend()) {
			return NPOS;
		}
		return mapIterator->second;
	}

	ConstIterator findIterator(const ValueType &value) const {
		SizeType index = findIndex(value);
		
		if (index != NPOS) {
			return _vector.cbegin() + index;
		}
		return _vector.cend();
	}

	bool find(const ValueType &value) const {
		return findIndex(value) != NPOS;
	}

	const ValueType &front() const {
		return _vector.front();
	}

	const ValueType &back() const {
		return _vector.back();
	}

	// allow getting const iterators only
	// to modify the values, the other more specific methods must be used instead
	ConstIterator cbegin() const {
		return _vector.cbegin();
	}

	ConstIterator cend() const {
		return _vector.cend();
	}

	bool empty() const {
		return _vector.empty();
	}

	SizeType size() const {
		return _vector.size();
	}

	void reserve(SizeType size) {
		_vector.reserve(size);
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
	bool push(const ValueType &value, SizeType &index) {
		std::pair<Map::iterator, bool> emplaced = _map.emplace(value, _vector.size());

		if (!emplaced.second) {
			index = emplaced.first->second;
			return false;
		}

		try {
			_vector.push_back(value);
		} catch (...) {
			_map.erase(emplaced.first);
			throw;
		}

		index = emplaced.first->second;
		return true;
	}

	bool push(const ValueType &value, ConstIterator &iterator) {
		SizeType index = 0;
		bool result = push(value, index);
		iterator = _vector.cbegin() + index;
		return result;
	}

	bool push(const ValueType &value) {
		SizeType index = NPOS;
		return push(value, index);
	}

	ValueType pop() {
		ValueType value = _vector.back();
		_map.erase(value);
		_vector.pop_back();
		return value;
	}

	// this is implemented using an index
	// because we need to insert one into the map anyway
	// (the map needs to use indices to avoid iterator invalidation on vector erase)
	bool replace(const ValueType &value, SizeType index) {
		SizeType size = _vector.size();
		
		if (index >= size) {
			throw std::invalid_argument("index must not be greater than or equal to size");
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

	bool replace(const ValueType &value, ConstIterator iterator) {
		return replace(value, iterator - _vector.cbegin());
	}

	// this is implemented using an iterator
	// because it's the only way to know the next element after the one we erased
	// and we need that information in order to correct the map
	// these methods have iterator and index in the name to differentiate them from erase
	// (what if ValueType is an iterator or index?)
	ConstIterator eraseIterator(ConstIterator beginIterator, ConstIterator endIterator) {
		// having an iterator outside of the vector is undefined behaviour
		// we don't check for that, since the UB has already been "committed" in that case
		// so, we shouldn't try and make sense of that
		// however the compiler has no way of knowing I expect the beginIterator to be
		// before the endIterator here, so I do check this to avoid smashing the stack
		if (endIterator < beginIterator) {
			throw std::invalid_argument("endIterator must not be less than beginIterator");
		}

		for (
			auto vectorIterator = beginIterator;
			vectorIterator != endIterator;
			vectorIterator++
		) {
			_map.erase(*vectorIterator);
		}

		SizeType index = (SizeType)(endIterator - beginIterator);
		ConstIterator iterator = _vector.erase(beginIterator, endIterator);

		// may be zero if begin and end are the same (which is valid)
		if (index) {
			for (
				auto vectorIterator = iterator;
				vectorIterator != _vector.cend();
				vectorIterator++
			) {
				_map[*vectorIterator] -= index;
			}
		}
		return iterator;
	}

	ConstIterator eraseIterator(ConstIterator iterator) {
		// would cause undefined behaviour if allowed to continue
		ConstIterator endIterator = _vector.cend();

		if (iterator == endIterator) {
			throw std::invalid_argument("iterator must not be equal to endIterator");
		}
		return eraseIterator(iterator, iterator + 1);
	}

	Type &eraseIndex(SizeType beginIndex, SizeType endIndex) {
		if (endIndex < beginIndex) {
			throw std::invalid_argument("endIndex must not be less than beginIndex");
		}

		SizeType size = _vector.size();

		if (endIndex > size) {
			throw std::invalid_argument("endIndex must not be greater than size");
		}

		auto beginIterator = _vector.cbegin();
		eraseIterator(beginIterator + beginIndex, beginIterator + endIndex);
		return *this;
	}

	Type &eraseIndex(SizeType index) {
		SizeType size = _vector.size();

		if (index >= size) {
			throw std::invalid_argument("index must not be greater than or equal to size");
		}

		eraseIterator(_vector.cbegin() + index);
		return *this;
	}

	// find and erase a value in the mapped vector
	// this can be better optimized when written as its own implementation
	// so this doesn't use the other overloads
	// once again this returns true if the value was erased, false otherwise
	bool erase(const ValueType &value) {
		auto mapIterator = _map.find(value);

		if (mapIterator == _map.cend()) {
			return false;
		}

		for (
			auto vectorIterator = _vector.erase(_vector.cbegin() + mapIterator->second);
			vectorIterator != _vector.cend();
			vectorIterator++
		) {
			_map[*vectorIterator]--;
		}

		_map.erase(mapIterator);
		return true;
	}

	// these return the MappedVector reference to allow use with a temporary
	Type &concat(const Vector &concatVector) {
		_vector.reserve(_vector.size() + concatVector.size());
		
		for (
			auto concatVectorIterator = concatVector.cbegin();
			concatVectorIterator != concatVector.cend();
			concatVectorIterator++
		) {
			push(*concatVectorIterator);
		}
		return *this;
	}

	Type &concat(const InitializerList &concatInitializerList) {
		return concat(Vector(concatInitializerList));
	}

	Type &concat(const Type &concatMappedVector) {
		return concat(concatMappedVector._vector);
	}

	Type &difference(const Vector &differenceVector) {
		for (
			auto differenceVectorIterator = differenceVector.cbegin();
			differenceVectorIterator != differenceVector.cend();
			differenceVectorIterator++
		) {
			erase(*differenceVectorIterator);
		}
		return *this;
	}

	Type &difference(const InitializerList &differenceInitializerList) {
		return difference(Vector(differenceInitializerList));
	}

	Type &difference(const Type &differenceMappedVector) {
		return difference(differenceMappedVector._vector);
	}

	Type() = default;

	// we don't need a copy constructor that takes in this class
	// (this class only uses RAII types so can be safely copied with the default behaviour)
	Type(const Vector &copyVector) {
		concat(copyVector);
	}

	Type(const InitializerList &copyInitializerList) {
		concat(copyInitializerList);
	}

	Type &operator=(const Vector &assignVector) {
		clear();
		return concat(assignVector);
	}

	Type &operator=(const InitializerList &assignInitializerList) {
		clear();
		return concat(assignInitializerList);
	}

	Type &operator+=(const Vector &addVector) {
		return concat(addVector);
	}

	Type &operator+=(const InitializerList &addInitializerList) {
		return concat(addInitializerList);
	}

	Type &operator+=(const Type &addMappedVector) {
		return concat(addMappedVector);
	}

	Type &operator-=(const Vector &subtractVector) {
		return difference(subtractVector);
	}

	Type &operator-=(const InitializerList &subtractInitializerList) {
		return difference(subtractInitializerList);
	}

	Type &operator-=(const Type &subtractMappedVector) {
		return difference(subtractMappedVector);
	}

	// these are const as to not allow modifying vector values without map values
	const ValueType &operator[](SizeType index) const {
		return _vector[index];
	}

	const Vector &get() const {
		return _vector;
	}
};

// these are intentionally outside the class to allow for type conversions when adding/subtracting
template <typename ValueType>
inline MappedVector<ValueType> operator+(
	const MappedVector<ValueType> &addMappedVector,
	const MappedVector<ValueType> &addMappedVector2
) {
	return MappedVector<ValueType>(addMappedVector).concat(addMappedVector2);
}

template <typename ValueType>
inline MappedVector<ValueType> operator-(
	const MappedVector<ValueType> &subtractMappedVector,
	const MappedVector<ValueType> &subtractMappedVector2
) {
	return MappedVector<ValueType>(subtractMappedVector).difference(subtractMappedVector2);
}