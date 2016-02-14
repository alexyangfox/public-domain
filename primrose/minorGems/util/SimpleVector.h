// Jason Rohrer
// SimpleVector.h

/**
*
*	Simple vector template class. Supports pushing at end and random-access deletions.
*	Dynamically sized.
*
*
*	Created 10-24-99
*	Mods:
*		Jason Rohrer	12-11-99	Added deleteAll function
*		Jason Rohrer	1-30-2000	Changed to return NULL if get called on non-existent element
*		Jason Rohrer	12-20-2000	Added a function for deleting a particular
*									element.
*		Jason Rohrer	12-14-2001	Added a function for getting the index of
*									a particular element.
*		Jason Rohrer	1-24-2003	Added a functions for getting an array or
*									string of all elements.
*		Jason Rohrer	7-26-2005	Added template <> to explicitly specialized
*									getElementString.
*		Jason Rohrer	1-9-2009	Added setElementString method.
*/

#include "minorGems/common.h"



#ifndef SIMPLEVECTOR_INCLUDED
#define SIMPLEVECTOR_INCLUDED

#include <string.h>		// for memory moving functions


const int defaultStartSize = 50;

template <class Type>
class SimpleVector {
	public:
	
		SimpleVector();		// create an empty vector
		SimpleVector(int sizeEstimate); // create an empty vector with a size estimate
		
		~SimpleVector();
		
		
		void push_back(Type x);		// add x to the end of the vector
		
		Type *getElement(int index);		// get a ptr to element at index in vector
		
		
		int size();		// return the number of allocated elements in the vector
		
		bool deleteElement(int index);		// delete element at an index in vector
		
		/**
		 * Deletes a particular element.  Deletes the first element
		 * in vector == to inElement.
		 *
		 * @param inElement the element to delete.
		 *
		 * @return true iff an element was deleted.
		 */
		bool deleteElement( Type inElement );



		/**
		 * Gets the index of a particular element.  Gets the index of the
		 * first element in vector == to inElement.
		 *
		 * @param inElement the element to get the index of.
		 *
		 * @return the index if inElement, or -1 if inElement is not found.
		 */
		int getElementIndex( Type inElement );

		
		
		void deleteAll();		// delete all elements from vector


        
		/**
		 * Gets the elements as an array.
		 *
		 * @return the a new array containing all elements in this vector.
         *   Must be destroyed by caller, though elements themselves are
         *   not copied.
		 */
		Type *getElementArray();


        
        /**
		 * Gets the char elements as a \0-terminated string.
		 *
		 * @return a \0-terminated string containing all elements in this
         *   vector.
         *   Must be destroyed by caller.
		 */
		char *getElementString();


        /**
		 * Sets the char elements as a \0-terminated string.
		 *
		 * @param inString a \0-terminated string containing all elements to 
         *   this vector with.
         *   Must be destroyed by caller.
		 */
		void setElementString( char *inString );

        
	private:
		Type *elements;
		int numFilledElements;
		int maxSize;
		int minSize;		// number of allocated elements when vector is empty
		};
		
		
template <class Type>		
inline SimpleVector<Type>::SimpleVector() {
	elements = new Type[defaultStartSize];
	numFilledElements = 0;
	maxSize = defaultStartSize;
	minSize = defaultStartSize;
	}

template <class Type>
inline SimpleVector<Type>::SimpleVector(int sizeEstimate) {
	elements = new Type[sizeEstimate];
	numFilledElements = 0;
	maxSize = sizeEstimate;
	minSize = sizeEstimate;
	}
	
template <class Type>	
inline SimpleVector<Type>::~SimpleVector() {
	delete [] elements;
	}	

template <class Type>
inline int SimpleVector<Type>::size() {
	return numFilledElements;
	}

template <class Type>
inline Type *SimpleVector<Type>::getElement(int index) {
	if( index < numFilledElements && index >=0 ) {
		return &(elements[index]);
		}
	else return NULL;
	}
	

template <class Type>
inline bool SimpleVector<Type>::deleteElement(int index) {
	if( index < numFilledElements) {	// if index valid for this vector
		
		if( index != numFilledElements - 1)  {	// this spot somewhere in middle
		
			// move memory towards front by one spot
			int sizeOfElement = sizeof(Type);
		
			int numBytesToMove = sizeOfElement*(numFilledElements - (index+1));
		
			Type *destPtr = &(elements[index]);
			Type *srcPtr = &(elements[index+1]);
		
			memmove((void *)destPtr, (void *)srcPtr, numBytesToMove);
			}
			
		numFilledElements--;	// one less element in vector
		return true;
		}
	else {				// index not valid for this vector
		return false;
		}
	}


template <class Type>
inline bool SimpleVector<Type>::deleteElement( Type inElement ) {
	int index = getElementIndex( inElement );
	if( index != -1 ) {
		return deleteElement( index );
		}
	else {
		return false;
		}
	}



template <class Type>
inline int SimpleVector<Type>::getElementIndex( Type inElement ) {
	// walk through vector, looking for first match.
	for( int i=0; i<numFilledElements; i++ ) {
		if( elements[i] == inElement ) {
			return i;
			}
		}
	
	// no element found
	return -1;
	}



template <class Type>
inline void SimpleVector<Type>::deleteAll() {
	numFilledElements = 0;
	if( maxSize > minSize ) {		// free memory if vector has grown
		delete [] elements;
		elements = new Type[minSize];	// reallocate an empty vector
		maxSize = minSize;
		}
	}


template <class Type>
inline void SimpleVector<Type>::push_back(Type x)	{
	if( numFilledElements < maxSize) {	// still room in vector
		elements[numFilledElements] = x;
		numFilledElements++;
		}
	else {					// need to allocate more space for vector
		int newMaxSize = maxSize << 1;		// double size
		
		Type *newAlloc = new Type[newMaxSize];
		int sizeOfElement = sizeof(Type);
		int numBytesToMove = sizeOfElement*(numFilledElements);
		
		// move into new space
		memcpy((void *)newAlloc, (void *) elements, numBytesToMove);
		
		// delete old space
		delete [] elements;
		
		elements = newAlloc;
		maxSize = newMaxSize;	
		
		elements[numFilledElements] = x;
		numFilledElements++;	
		}
	}



template <class Type>
inline Type *SimpleVector<Type>::getElementArray() {
    Type *newAlloc = new Type[ numFilledElements ];
    int sizeOfElement = sizeof( Type );
    int numBytesToCopy = sizeOfElement * numFilledElements;
		
    // copy into new space
    memcpy( (void *)newAlloc, (void *)elements, numBytesToCopy );

    return newAlloc;
    }



template <>
inline char *SimpleVector<char>::getElementString() {
    char *newAlloc = new char[ numFilledElements + 1 ];
    int sizeOfElement = sizeof( char );
    int numBytesToCopy = sizeOfElement * numFilledElements;
		
    // copy into new space
    memcpy( (void *)newAlloc, (void *)elements, numBytesToCopy );

    newAlloc[ numFilledElements ] = '\0';
    
    return newAlloc;
    }



template <>
inline void SimpleVector<char>::setElementString( char *inString ) {
    // slow but correct
    
    deleteAll();
    
    int numChars = strlen( inString );
    for( int i=0; i<numChars; i++ ) {
        push_back( inString[i] );
        }
    }



#endif
