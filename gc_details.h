/* 
    This class defines an element that is stored in
    the garbage collection information list.
*/
template <class T>
class PtrDetails {
    public:
        unsigned refcount;  // current reference count
        T *memPtr;          // pointer to allocated memory
        bool isArray;       // true if pointing to array
        unsigned arraySize; // size of array

        PtrDetails(T *ptr, unsigned size = 0) {
            refcount = 1;
            memPtr = ptr;
            arraySize = size;
            isArray = size > 0;
        }
};

template <class T>
bool operator==(const PtrDetails<T> &ob1,
                const PtrDetails<T> &ob2) {
    return ob1.memPtr == ob2.memPtr;
}
