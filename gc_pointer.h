#include <iostream>
#include <list>
#include <typeinfo>
#include <cstdlib>
#include "gc_details.h"
#include "gc_iterator.h"
/*
    Pointer implements a pointer type that uses
    garbage collection to release unused memory.
    A Pointer must only be used to point to memory
    that was dynamically allocated using new.
    When used to refer to an allocated array,
    specify the array size.
*/
template <class T, int size = 0>
class Pointer {
    private:
        // refContainer maintains the garbage collection list.
        static std::list<PtrDetails<T>> refContainer;
        // addr points to the allocated memory to which
        // this Pointer pointer currently points.
        T *addr;
        bool isArray; // true if pointing to array 
        unsigned arraySize; // size of the array
        static bool first; // true when first Pointer is created
        
        // Return an iterator to pointer details in refContainer.
        typename std::list<PtrDetails<T>>::iterator findPtrInfo(T *ptr);
    public:
        typedef Iter<T> GCiterator;
        // Empty constructor
        // NOTE: templates aren't able to have prototypes with default arguments
        // this is why constructor is designed like this:
        Pointer() {
            Pointer(NULL);
        }
        Pointer(T*);
        // Copy constructor.
        Pointer(const Pointer&);
        // Destructor for Pointer.
        ~Pointer();
        // Collect garbage. Returns true if at least
        // one object was freed.
        static bool collect();
        // Overload assignment of pointer to Pointer.
        T *operator=(T *t);
        // Overload assignment of Pointer to Pointer.
        Pointer &operator=(Pointer &rv);
        // Return a reference to the object pointed
        // to by this Pointer.
        T &operator*() {
            return *addr;
        }
        // Return the address being pointed to.
        T *operator->() { return addr; }
        // Return a reference to the object at the
        // index specified by i.
        T &operator[](int i) { return addr[i]; }
        // Conversion function to T *.
        operator T *() { return addr; }
        // Return an Iter to the start of the allocated memory.
        Iter<T> begin() {
            int _size;
            if (isArray)
                _size = arraySize;
            else
                _size = 1;
            return Iter<T>(addr, addr, addr + _size);
        }
        // Return an Iter to one past the end of an allocated array.
        Iter<T> end() {
            int _size;
            if (isArray)
                _size = arraySize;
            else
                _size = 1;
            return Iter<T>(addr + _size, addr, addr + _size);
        }
        // Return the size of refContainer for this type of Pointer.
        static int refContainerSize() { return refContainer.size(); }
        // A utility function that displays refContainer.
        static void showlist();
        // Clear refContainer when program exits.
        static void shutdown();
};

// STATIC INITIALIZATION
// Creates storage for the static variables
template <class T, int size>
std::list<PtrDetails<T>> Pointer<T, size>::refContainer;
template <class T, int size>
bool Pointer<T, size>::first = true;

// Constructor for both initialized and uninitialized objects. -> see class interface
template<class T, int size>
Pointer<T, size>::Pointer(T *t){
    // Register shutdown() as an exit function.
    if (first)
        atexit(shutdown);
    first = false;

    // Does it exists in the refContainer
    typename std::list<PtrDetails<T> >::iterator ptrInfo = findPtrInfo(t);
    if (ptrInfo == refContainer.end()) {
        PtrDetails<T> pointerDetails(t, size);
        refContainer.push_back(pointerDetails);
        addr = pointerDetails.memPtr;
        isArray = pointerDetails.isArray;
        arraySize = pointerDetails.arraySize;
    } else {
        ptrInfo->refcount++;
        addr = ptrInfo->memPtr;
        isArray = ptrInfo->isArray;
        arraySize = ptrInfo->arraySize;
    }
    
}
// Copy constructor.
template<class T, int size>
Pointer<T, size>::Pointer(const Pointer &ob) {

    typename std::list<PtrDetails<T>>::iterator ptrInfo = findPtrInfo(ob.addr);
    ptrInfo->refcount++;
    addr = ob.addr;
    arraySize = ob.arraySize;
    isArray = ob.isArray;

}

// Destructor for Pointer.
template <class T, int size>
Pointer<T, size>::~Pointer() {
    
    typename std::list<PtrDetails<T>>::iterator ptrInfo = findPtrInfo(addr);
    if (ptrInfo->refcount)
        ptrInfo->refcount--;  // decrement ref count
    
    collect();

}

// Collect garbage. Returns true if at least
// one object was freed.
template <class T, int size>
bool Pointer<T, size>::collect() {
    bool memFreed = false;
    typename std::list<PtrDetails<T>>::iterator p;
    
    do {
        // Scan refContainer looking for unreferenced pointers.
        for (p = refContainer.begin(); p != refContainer.end(); p++) {
            if (p->refcount > 0)
                // If in-use, skip.
                continue;
            
            // Remove unused entry from refContainer.
            refContainer.erase(p);

            // Free memory unless the Pointer is null.
            if (p->memPtr != nullptr)
                if (p->isArray)
                    delete[] p->memPtr;
                else
                    delete p->memPtr;
                
            memFreed = true;
            
            // Restart the search.
            break;
        }
    } while (p != refContainer.end());

    return memFreed;
}

// Overload assignment of pointer to Pointer.
template <class T, int size>
T *Pointer<T, size>::operator=(T *t) {
    typename std::list<PtrDetails<T> >::iterator ptrInfo = findPtrInfo(addr);
    ptrInfo->refcount--;
    
    ptrInfo = findPtrInfo(t);
    if (ptrInfo == refContainer.end()) {
        PtrDetails<T> pointerDetails(t, size);
        refContainer.push_back(pointerDetails);
        addr = pointerDetails.memPtr;
        isArray = pointerDetails.isArray;
        arraySize = pointerDetails.arraySize;
    } else {
        ptrInfo->refcount++;
        addr = ptrInfo->memPtr;
        isArray = ptrInfo->isArray;
        arraySize = ptrInfo->arraySize;
    }

    return addr;

}
// Overload assignment of Pointer to Pointer.
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv){
    typename std::list<PtrDetails<T>>::iterator ptrInfo = findPtrInfo(addr);
    ptrInfo->refcount--;
    
    ptrInfo = findPtrInfo(rv.addr);
    ptrInfo->refcount++;
    
    this.addr = rv.addr;
    return rv;
}

// A utility function that displays refContainer.
template <class T, int size>
void Pointer<T, size>::showlist() {
    typename std::list<PtrDetails<T> >::iterator p;
    std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
    std::cout << "memPtr refcount value\n ";

    if (refContainer.begin() == refContainer.end())
        std::cout << " Container is empty!\n\n ";
    
    for (p = refContainer.begin(); p != refContainer.end(); p++) {
        std::cout << "[" << (void *)p->memPtr << "]"
             << " " << p->refcount << " ";
        if (p->memPtr)
            std::cout << " " << *p->memPtr;
        else
            std::cout << "---";
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T>>::iterator Pointer<T, size>::findPtrInfo(T *ptr) {
    typename std::list<PtrDetails<T>>::iterator p;
    // Find ptr in refContainer.
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        if (p->memPtr == ptr)
            return p;
    return p;
}
// Clear refContainer when program exits.
template <class T, int size>
void Pointer<T, size>::shutdown() {
    if (refContainerSize() == 0)
        return; // list is empty

    typename std::list<PtrDetails<T>>::iterator p;
    // Set all reference counts to zero
    for (p = refContainer.begin(); p != refContainer.end(); p++)
        p->refcount = 0;
    
    collect();
}
