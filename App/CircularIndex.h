/*
 *      Author: Evgeny Sobolev 09.02.1984 y.b.
 *      Tel: +79003030374
 *      e-mail: hwswdevelop@gmail.com
*/


#pragma once


#include <stddef.h>
#include <stdint.h>

template<typename IndexType = size_t, IndexType MinValue = 0, IndexType MaxValue = IndexType(-1)>
struct CircularIndex {

	inline CircularIndex() : _index(MinValue) {
	}

	inline CircularIndex(const IndexType index) : _index(index) {
	}

    inline CircularIndex& operator = ( const IndexType index ) {
        _index = index;
        return *this;
    }

    inline CircularIndex& operator = ( const CircularIndex& other ) {
        _index = other._index;
        return *this;
    }

    inline IndexType operator * () {
        return _index;
    }

    inline CircularIndex& operator += ( const IndexType value ) {
        const IndexType upToMax = _maxValue - _index;
        if ( upToMax >= value) {
            _index += value;
        } else {
            const IndexType afterMin = value - upToMax - 1;
            _index = _minValue + afterMin;
        }
        return *this;
    }

    inline CircularIndex& operator -= ( const IndexType value ) {
        const IndexType downToMin = _index - _minValue;
        if ( downToMin >= value ) {
            _index -= value;
        } else {
            const IndexType beforeMax = value - downToMin - 1;
            _index = _maxValue - beforeMax;
        }
        return *this;
    }

    inline bool operator == ( const CircularIndex& other ) {
        return ( _index == other._index );
    }

    inline bool operator < (const CircularIndex& other) {
        const IndexType half = ((_maxValue - _minValue)) / 2;
        if ( _index == other._index ) return false;
        IndexType diff = indexDiff( _index, other._index );
        if ( diff < half ) {
            return ( _index < other._index );
        } else {
            return ( _index > other._index );
        }
    }

    inline bool operator <= (const CircularIndex& other) {
        const IndexType half = ((_maxValue - _minValue)) / 2;
        if (_index == other._index) return true;
        IndexType diff = indexDiff(_index, other._index);
        if (diff < half) {
            return (_index < other._index);
        }
        else {
            return (_index > other._index);
        }
    }

    inline bool operator >= (const CircularIndex& other) {
        const IndexType half = ((_maxValue - _minValue)) / 2;
        if (_index == other._index) return true;
        IndexType diff = indexDiff(_index, other._index);
        if ( diff < half ) {
            return ( _index > other._index );
        }
        else {
            return ( _index < other._index );
        }
    }

private:
    inline IndexType indexDiff( const IndexType index1, const IndexType index2 ) const {
        if ( index1 >= index2 ) {
            return ( index1 - index2 );
        } else {
            return ( index2 - index1 );
        }
    }

private:
    static constexpr const IndexType _minValue = MinValue;
    static constexpr const IndexType _maxValue = MaxValue;
    IndexType       _index = _minValue;

};

