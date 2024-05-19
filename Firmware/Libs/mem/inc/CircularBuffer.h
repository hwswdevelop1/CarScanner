
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "Status.h"



namespace Static {

    // Don't beleve youself. The world is not the same as you think ...
    template< typename DataType = uint8_t, size_t ElementCount = 0, bool SafeMode = true >
    struct CircularBuffer {

        struct Iterator {

            Iterator( CircularBuffer& buffer, size_t index ) : _buffer( buffer ), _index(index) {
            }

            inline Iterator& operator ++() {
                size_t nextIndex = _index;
                if ( Status::Success != _buffer.isElementIndexValid( nextIndex ) ) {
                    _index = _buffer._headIndex;
                    return *this;
                }
                if ( Status::Success != _buffer.next( nextIndex ) ) {
                    _index = _buffer._headIndex;
                    return *this;
                } 
                _index = nextIndex;
                return *this;
            }

            inline bool operator ==( const Iterator& other ) {
                return ( _index == other._index );
            }

            inline bool operator !=( const Iterator& other ) {
                return ( _index != other._index );
            }

            inline Status getData( DataType& data ) {
                Status status =  _buffer.isElementIndexValid( _index );
                if ( Status::Success != status ) return status;
                data = _buffer._pool[_index];
                return Status::Success;          
            }

            inline Status getData( DataType* data ) {
                if (nullptr == data ) return Status::Nullptr;
                Status status =  isElementIndexValid( _index );
                if ( Status::Success != status ) return status;
                *data = _buffer._pool[_index];
                return Status::Success;      
            }

            inline DataType operator*() {
                if ( Status::Success != _buffer.isElementIndexValid( _index ) ) {
                    // Something is wrong, but I can't throw exception
                    return DataType {};
                }
                if ( _index == _buffer._headIndex ) {
                    // No valid data available, but I can't throw exception
                    return DataType {};
                }
                // I don't know, what about copy....
                return _buffer._pool[_index];
            }

        private:
            CircularBuffer< DataType, ElementCount, SafeMode> _buffer;
            volatile size_t  _index;
        };

        inline Iterator begin() {
            return Iterator( *this, _tailIndex );
        }

        inline Iterator end() {
            return Iterator( *this, _headIndex );
        }
        
        inline Status clear() {
            _headIndex = _firstElementIndex;
            _tailIndex = _firstElementIndex;
            if constexpr (SafeMode) {
                for ( auto it = begin(); it != end(); ++it ) {
                    //*it = {};
                }
            }
            return Status::Success;
        }

        Status appendNew( DataType& data ) {
            if ( Status::Success != isElementIndexValid( _tailIndex ) ) return Status::OutOfBoundary;
            if ( Status::Success != isElementIndexValid( _headIndex ) ) return Status::OutOfBoundary;
            size_t nextIndex = _headIndex;
            if ( Status::Success != next( nextIndex ) ) return Status::OutOfBoundary;
            if ( nextIndex == _tailIndex ) return Status::Full;
            _pool[ _headIndex ] = data;
            _headIndex = nextIndex;
            return Status::Success;
        }

        Status getLast( DataType& data ) {
            if ( Status::Success != isElementIndexValid( _tailIndex ) ) return Status::OutOfBoundary;
            if ( Status::Success != isElementIndexValid( _headIndex ) ) return Status::OutOfBoundary;
            if ( _headIndex == _tailIndex ) return Status::Empty;
            data = _pool[ _tailIndex ];
            return Status::Success;
        }

        Status removeLast() {
            if ( Status::Success != isElementIndexValid( _tailIndex ) ) return Status::OutOfBoundary;
            if ( Status::Success != isElementIndexValid( _headIndex ) ) return Status::OutOfBoundary;            
            if ( _tailIndex == _headIndex ) return Status::Empty;
            size_t nextIndex = _tailIndex;
            if ( Status::Success != next( nextIndex ) ) return Status::OutOfBoundary;
            _tailIndex = nextIndex;
            return Status::Success;
        }

        Status count( size_t& usedElementCount ) {
            if ( ( Status::Success != isElementIndexValid( _tailIndex )) ||
                 ( Status::Success != isElementIndexValid( _headIndex )) ) {
                usedElementCount = 0;
                return Status::OutOfBoundary;
            }
            size_t size = 0;
            if ( _headIndex >= _tailIndex ) {
                size = (_headIndex - _tailIndex);
            } else {
                size = ( ElementCount - _tailIndex ) + _headIndex + 1;
            }
            usedElementCount = size;
            return Status::Success;
        }

        Status space( size_t& freeElementCount ) {
            size_t usedElementCount = 0;
            if ( ( Status::Success != isElementIndexValid( _tailIndex )) ||
                 ( Status::Success != isElementIndexValid( _headIndex )) ) {
                freeElementCount = 0;
                return Status::OutOfBoundary;
            }
            if ( Status::Success != count(usedElementCount) ){
                freeElementCount = 0;
                return Status::OutOfBoundary;  
            }
            freeElementCount = ElementCount - usedElementCount;
            return Status::Success;
        }

        static constexpr size_t capacity() {
            return ElementCount;
        }

    private:

        static inline Status next( size_t& index ) {
            if ( index != _lastElementIndex ) {
                // Have to check, because of unknown value possible, if I got it from crashed iterator
                if ( Status::Success != isElementIndexValid( index ) ) return Status::OutOfBoundary;
                // So it is better use of interlocked increment function if &index pointer to the _head or _tail.
                index++;
                // Don't have to check, because of minimum size
            } else {
                index = _firstElementIndex;
            }
            return Status::Success;
        }

        static inline Status isElementIndexValid( size_t index ) {
            static_assert( ( ElementCount > 0 ), "Minimum data element count is one (1)\n" );
            if constexpr ( SafeMode ) {
                if ( index < _firstElementIndex ) return Status::Underflow;
                if ( index > _lastElementIndex ) return Status::Overflow;
            }
            return Status::Success;
        }

    private:
        static constexpr const size_t HeadEqualsOverheadSize = 1;
        static constexpr const size_t SafeProgrammingOverheadSize = 1;

        // This Circular buffer don't use pointers, becuse of potential pointer operations outside of class source code
        // So, if you wish to get safe memory later, think about this:
        //  DataType* const _firstElementPointer = &_pool[0];
        //  Yes, of cause, you will get single const, that is pointer to the first elemnt of pool
        //  But, if you use size_t firstElementIndex = 0, you have to unindex it. 
        //  So, I mean reg[0] <= pointer, reg[1]  <= index, reg[0] <= reg[0] + reg[1].
        //  It is not so good from speed optimization point of view, but is safer, 
        //  when you will use memory in the different memory pools.

        static constexpr const size_t _firstElementIndex = 0;
        static constexpr const size_t _lastElementIndex = ElementCount;
        
        DataType _pool[ ElementCount + HeadEqualsOverheadSize  ];
        volatile size_t _headIndex = _firstElementIndex;
        volatile size_t _tailIndex = _firstElementIndex;
    };

}

