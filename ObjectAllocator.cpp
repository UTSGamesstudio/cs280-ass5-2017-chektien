//#define DEBUG

#include "ObjectAllocator.h"

#ifdef DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config) : Config_(config)
{
	ObjectSize_ = ObjectSize;

#ifdef DEBUG
    cout << "ObjectAllocator: ctoring with ObjectSize=" << ObjectSize_ << endl;
#endif

}

void *ObjectAllocator::Allocate() throw (OAException)
{

#ifdef DEBUG
    cout << "ObjectAllocator: Allocate char[" << ObjectSize_ << "]" << endl;
#endif

	return new char[ObjectSize_];
}

void ObjectAllocator::Free(void *anObject) throw(OAException)
{
	// Defer to C++ heap manager
	delete [] reinterpret_cast<char *>(anObject);
}
