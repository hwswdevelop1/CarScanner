/*
 *      Author: Evgeny Sobolev 09.02.1984 y.b.
 *      Tel: +79003030374
 *      e-mail: hwswdevelop@gmail.com
*/

#include "CircularIndex.h"
#include <stdint.h>
#include <stddef.h>



template<size_t PoolSize = 0, size_t BufferCount = 0>
struct CircularPacketBuffer {
	using IndexType = uint16_t;
	static constexpr const bool DebugMode = true;
	static constexpr const IndexType InvalidIndex = IndexType(-1);

	enum class BufferState : uint8_t {
		Free,
		Allocated,
		Commited,
		Checkout,
		Trash
	};

	struct BufferDescriptor {
		IndexType		data = 0;
		IndexType		size = 0;
		IndexType		totalSize = 0;
		BufferState		state = BufferState::Free;
	};

	using AllignType = uint32_t;
	static constexpr size_t AllignSize = sizeof(AllignType);
	static constexpr size_t AllignedElementCount = ( PoolSize + AllignSize - 1 ) / AllignSize;
	using CircularIndexType = CircularIndex<IndexType, 0, (PoolSize - 1) >;
	using DescriptorIndexType = CircularIndex<IndexType, 0, (BufferCount - 1) >;

	IndexType alloc( size_t size, bool allocPossibleCheck = false ) {

		IndexType resDescriptorIndex = InvalidIndex;
		if ( _size >= PoolSize ) return resDescriptorIndex;
		const size_t totalRemainSize = PoolSize - _size;
		if ( size > totalRemainSize ) return resDescriptorIndex;

		if ( 0 == _size ) {
			_tail = 0;
		}

		CircularIndexType head(_tail);
		head += _size;
		const IndexType headIndex = *head;
		const IndexType tailIndex = *_tail;
		if ( headIndex >= tailIndex ) {
			// [empty][tailIndex][data][headIndex][empty]
			const size_t remainAtTheEnd = PoolSize - headIndex;
			const size_t offset = allignedOffset(headIndex);
			const size_t totalSize = size + offset;
			if (totalSize <= remainAtTheEnd) {
				// [empty][tailIndex][data][headIndex][allocate]
				const size_t bufferStartPos = headIndex;
				const size_t bufferSize = totalSize;
				const size_t dataStartPos = headIndex + offset;
				resDescriptorIndex = allocateDescriptor(bufferStartPos, bufferSize, dataStartPos, size, allocPossibleCheck );
			}
			else {
				// [allocate][tailIndex][data][headIndex][notUsed]
				const size_t notUsedSize = PoolSize - headIndex;
				const size_t offsetAtBegin = allignedOffset(0);
				const size_t totalSize = size + offsetAtBegin;
				const size_t remainAtBegin = tailIndex;
				if (totalSize <= remainAtBegin) {
					const size_t bufferStartPos = headIndex;
					const size_t totalSizeIncludingUnused = notUsedSize + totalSize;
					const size_t dataStartPos = offsetAtBegin;
					resDescriptorIndex = allocateDescriptor(bufferStartPos, totalSizeIncludingUnused, dataStartPos, size, allocPossibleCheck );
				}
			}
		} else {
			// [data][tailIndex][empty][headIndex][data]	
			const size_t remainAtTheMiddle =  tailIndex - headIndex;
			const size_t offsetAtTheMiddle = allignedOffset(headIndex);
			const size_t totalSize = size + offsetAtTheMiddle;
			if ( totalSize <= remainAtTheMiddle ) {
				const size_t bufferStartPos = headIndex;
				const size_t dataStartPos = headIndex + offsetAtTheMiddle;
				const size_t bufferSize = totalSize;
				resDescriptorIndex = allocateDescriptor( bufferStartPos, bufferSize, dataStartPos, size, allocPossibleCheck );
			}
		}

		return resDescriptorIndex;
	}

	uint8_t* data( const IndexType index ) {
		if ( !isDescriptorExists(index) ) return nullptr;
		BufferDescriptor* const descr = &_descr[index];
		// Check if
		if ( ( BufferState::Free == descr->state ) ||
			 ( BufferState::Trash == descr->state ) ) return nullptr;
		uint8_t* const dataPtr = &_pool[descr->data];
		return dataPtr;
	}

	size_t size( const IndexType index ) {
		if ( !isDescriptorExists(index) ) return 0;
		BufferDescriptor* const descr = &_descr[index];
		// Check if
		if ( ( BufferState::Free == descr->state ) ||
			 ( BufferState::Trash == descr->state ) ) return 0;
		return descr->size;
	}

	void trunc( const IndexType index, const size_t size ) {
		if ( !isDescriptorExists(index) ) return;
		BufferDescriptor* const descr = &_descr[index];

		if ( ( BufferState::Allocated != descr->state ) &&
			 ( BufferState::Checkout != descr->state ) ) return;

		const IndexType currentSize = descr->size;

		if ( size < currentSize ) {
			if ( BufferState::Allocated == descr->state ) {
				// Get first allocated element position
				DescriptorIndexType head( _descrTail );
				// Append allocated descriptor count
				head += (_descrCount - 1);
				// Get index of last append descriptor
				const IndexType lastDescrIndex = *head;

				if ( lastDescrIndex == index ) {
					const IndexType diff = (currentSize - size);
					descr->size = size;
					descr->totalSize -= diff;
					_size -= diff;
				}
				descr->size = size;
			} else {
				descr->size = size;
			}
		}

	}

	void commit( const IndexType index ) {
		if ( !isDescriptorExists(index) ) return;
		BufferDescriptor* const descr = &_descr[index];
		if ( BufferState::Allocated != descr->state ) return;
		descr->state = BufferState::Commited;
	}

	void checkout( const IndexType index ) {
		if ( !isDescriptorExists(index) ) return;
		BufferDescriptor* const descr = &_descr[index];
		if ( BufferState::Commited != descr->state ) return;
		descr->state = BufferState::Checkout;
	}

	IndexType get() {
		IndexType ret = InvalidIndex;
		// Total descriptor count
		size_t counter = _descrCount;
		// Descriptor circular index, started from first allocated
		DescriptorIndexType currentDescrIndex(_descrTail);
		// Start from first allocated to last allocated
		while (counter > 0) {
			// Index of descriptor
			const IndexType index = *currentDescrIndex;
			// Descriptor pointer
			BufferDescriptor*  const descr = &_descr[index];
			// Check if found
			if ( BufferState::Commited == descr->state ) {
				// Found. First descriptor, in "Commited" state
				ret = index;
				break;
			}
			// Get next descriptor in queue
			if ( ( BufferState::Checkout == descr->state ) ||
				 ( BufferState::Trash == descr->state ) ) {
				// Get next descriptor
				currentDescrIndex += 1;
				counter--;
			} else {
				// Other state descriptors are not possible to use
				break;
			}
		}
		return ret;
	}

	void free( const IndexType index ) {
		if ( !isDescriptorExists(index) ) return;
		// Get descriptor
		BufferDescriptor*  const descr = &_descr[index];
		// Can free only descriptors are not in queue
		if ( ( BufferState::Allocated != descr->state ) &&
			 ( BufferState::Checkout != descr->state ) ) return;
		// Update current descriptor state to trash
		descr->state = BufferState::Trash;
		// Cleanup all descriptors marked as trash
		const IndexType indexOfFirstAllocated = *_descrTail;
		// Check if first descriptors in queue
		if ( indexOfFirstAllocated == index ) {
			// Try to free descriptors, if it start of queue
			while( _descrCount > 0 ) {
				const IndexType index = *_descrTail;
				BufferDescriptor* const descr = &_descr[index];
				if ( BufferState::Trash == descr->state ) {
					freeDescriptor(index);
				} else {
					break;
				}
			}
		} else {
			DescriptorIndexType head(_descrTail);
			if ( _descrCount > 0 ) {
				head += (_descrCount - 1);
				const IndexType indexOfLastAllocated = *head;
				if ( indexOfLastAllocated == index ) {
					BufferDescriptor* const descr = &_descr[index];
					const size_t freeSize =  descr->totalSize;
					_size -= freeSize;
					descr->size = 0;
					descr->totalSize = 0;
					descr->state = BufferState::Free;
					_descrCount--;
				}
			}
		}
	}

private:

	inline bool isDescriptorExists( const IndexType index ) {
		const bool descriptorExists = ((index >= 0) && (index < BufferCount ));
		return descriptorExists;
	}

	void freeDescriptor( const IndexType index ) {
		if ( 0 == _descrCount ) return;
		// Check if descriptor exists
		const IndexType indexOfFirstDescriptor = *_descrTail;
		// Free descriptor if, and only if it is first allocated
		if ( index == indexOfFirstDescriptor ) {
			// Get descriptor
			BufferDescriptor*  const descr = &_descr[index];
			// Get data count associated with descriptor
			const size_t sz = descr->totalSize;
			// Decrement total data size
			_size -= sz;
			// Increment index of first allocated element
			_tail += sz;
			// Free buffer
			descr->state = BufferState::Free;
			_descrTail += 1;
			_descrCount--;
		}
	}

	IndexType allocateDescriptor( const IndexType bufferStartPos, const size_t bufferSize, const IndexType dataStartPos, const size_t dataSize, bool allocPossibleCheck = false ) {

		// Check if allocate is possible
		if ( BufferCount == _descrCount ) {
			// All descriptors are used
			return InvalidIndex;
		}

		// Get first allocated elemnt position
		DescriptorIndexType head( _descrTail );

		// Append allocated descriptor count
		head += _descrCount;

		// "index" value is now, index of last not used descriptor
		const size_t index = *head;

		// Get descriptor pointer
		BufferDescriptor* descr = &_descr[index];

		// Check, if descriptor not used, it is Free
		if ( BufferState::Free == descr->state ) {
			if ( allocPossibleCheck ) return index;
			// Allocate descriptor
			_descrCount++;
			// Fill descriptor data
			descr->data = dataStartPos;
			descr->size = dataSize;
			descr->totalSize = bufferSize;
			descr->state = BufferState::Allocated;
			_size += bufferSize;
			return index;
		} else {
			// Something went wrong, descriptor data corrupt is possible
			return InvalidIndex;
		}
	}

private:
	IndexType allignedOffset(const IndexType index) {
		static constexpr const uint8_t AllignSize = 0x04;
		static constexpr const uint8_t AllignMask = AllignSize - 1;
		const size_t offset = ((AllignSize - (index & AllignMask))) & AllignMask;
		return offset;
	}

private:
	BufferDescriptor	_descr[BufferCount];
	uint8_t				_pool[PoolSize];
	CircularIndexType	_tail;
	DescriptorIndexType	_descrTail;
	size_t _size = 0;
	size_t _descrCount = 0;
};

typedef CircularPacketBuffer<>::IndexType IndexType;
constexpr const IndexType InvalidIndex = CircularPacketBuffer<>::InvalidIndex;



