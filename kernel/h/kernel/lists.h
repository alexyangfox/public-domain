#ifndef KERNEL_LISTS_H
#define KERNEL_LISTS_H



/* -----------------------------------------------------------------------------
** SYNOPSIS
**
** Double linked list with head and tail pointers in the header.
**
** -----------------------------------------------------------------------------
*/

/* Could add a name, makes it easier for arrays */

#define LIST(type)															\
	struct																	\
	{																		\
		struct type *head;													\
		struct type *tail;													\
	}


#define LIST_DECLARE(name, type)											\
	struct name																\
	{																		\
		struct type *head;													\
		struct type *tail;													\
	}

     
#define LIST_DEFINE(name)													\
		struct name
     

#define LIST_ENTRY( type)													\
	struct																	\
	{																		\
		struct type *next;													\
		struct type *prev;													\
	}
	

#define LIST_INIT( header)													\
	{																		\
		(header)->head = NULL;												\
		(header)->tail = NULL;												\
	}
	

#define LIST_CONCAT( dest, src, field)										\
	{																		\
		if ((src)->head != NULL)											\
		{																	\
			if ((dest)->head != NULL)										\
			{																\
				((dest)->tail)->field.next = (src)->head;					\
				((src)->head)->field.prev = (dest)->tail;					\
				(dest)->tail = (src)->tail;									\
			}																\
			else															\
			{																\
				((src)->head)->field.prev = NULL;							\
				(dest)->head = (src)->head;									\
				(dest)->tail = (src)->tail;									\
			}																\
		}																	\
	}


#define LIST_EMPTY( header)													\
	(((header)->head == NULL) ? 1 : 0)

	
#define LIST_HEAD( header)													\
	((header)->head)


#define LIST_TAIL( header)													\
	((header)->tail)


#define LIST_NEXT( entry, field)											\
	((entry)->field.next)


#define LIST_PREV( entry, field)											\
	((entry)->field.prev)


#define LIST_ADD_HEAD( header, new_head, field)								\
	{																		\
		(new_head)->field.next = (header)->head;							\
		(new_head)->field.prev = NULL;										\
																			\
		if ((header)->head != NULL)											\
			 (header)->head->field.prev = new_head;							\
		else																\
			(header)->tail = new_head;										\
																			\
		(header)->head = new_head;											\
	}


#define LIST_ADD_TAIL( header, new_tail, field)								\
	{																	\
		(new_tail)->field.next = NULL;										\
		(new_tail)->field.prev = (header)->tail;							\
		if ((header)->tail != NULL)											\
			(header)->tail->field.next = new_tail;							\
		else																\
			(header)->head = new_tail;										\
		(header)->tail = new_tail;											\
	}


#define LIST_REM_HEAD( header, field)										\
	{																	\
		(header)->head = (header)->head->field.next;						\
		if ((header)->head != NULL)											\
			(header)->head->field.prev = NULL;								\
		else																\
			(header)->tail = NULL;											\
	}


#define LIST_REM_TAIL( header, field)										\
	{																		\
		(header)->tail = (header)->tail->field.prev;						\
		if ((header)->tail != NULL)											\
			(header)->tail->field.next = NULL;								\
		else																\
			(header)->head = NULL;											\
	}


#define LIST_INSERT_AFTER( header, prev_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.next = (prev_entry)->field.next;					\
		(new_entry)->field.prev = prev_entry;								\
		(prev_entry)->field.next = new_entry;								\
		if ((new_entry)->field.next != NULL)								\
			(new_entry)->field.next->field.prev = new_entry;				\
		else																\
			(header)->tail = new_entry;										\
	}
	

#define LIST_INSERT_BEFORE( header, next_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.prev = (next_entry)->field.prev;					\
		(new_entry)->field.next = next_entry;								\
		(next_entry)->field.prev = new_entry;								\
		if ((new_entry)->field.prev != NULL)								\
			(new_entry)->field.prev->field.next = new_entry;				\
		else																\
			(header)->head = new_entry;										\
	}
	

#define LIST_REM_ENTRY( header, entry, field)								\
	{																		\
		if ((entry)->field.prev != NULL)									\
			(entry)->field.prev->field.next = (entry)->field.next;			\
		else																\
			(header)->head = (entry)->field.next;								\
		if ((entry)->field.next != NULL)									\
			(entry)->field.next->field.prev = (entry)->field.prev;			\
		else																\
			(header)->tail = (entry)->field.prev;								\
	}




/* -----------------------------------------------------------------------------
** SYNOPSIS
**
** Single linked queue with head and tail pointers in the header.
**
** -----------------------------------------------------------------------------
*/

#define QUEUE( type)														\
	struct																	\
	{																		\
		struct type *head;													\
		struct type *tail;													\
	}


#define QUEUE_DECLARE(name, type)											\
	struct name																\
	{																		\
		struct type *head;													\
		struct type *tail;													\
	}

     
#define QUEUE_DEFINE(name)													\
		struct name


#define QUEUE_ENTRY( type)													\
	struct																	\
	{																		\
		struct type *next;													\
	}


#define QUEUE_INIT( header)													\
	{																		\
		(header)->head = NULL;												\
		(header)->tail = NULL;												\
	}


#define QUEUE_EMPTY( header)												\
	(((header)->head == NULL) ? 1 : 0)

	
#define QUEUE_HEAD( header)													\
	((header)->head)


#define QUEUE_TAIL( header)													\
	((header)->tail)


#define QUEUE_NEXT( entry, field)											\
	((entry)->field.next)


#define QUEUE_ADD_TAIL( header, new_tail, field)							\
	{																		\
		(new_tail)->field.next = NULL;										\
		if ((header)->tail != NULL)											\
			(header)->tail->field.next = new_tail;							\
		else																\
			(header)->head = new_tail;										\
		(header)->tail = new_tail;											\
	}


#define QUEUE_REM_HEAD( header, field)										\
	{																		\
		(header)->head = (header)->head->field.next;						\
		if ((header)->head == NULL)											\
			(header)->tail = NULL;											\
	}


#define QUEUE_INSERT_AFTER( header, prec_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.next = (prev_entry)->field.next;					\
		(prev_entry)->field.next = new_entry;								\
		if ((new_entry)->field.next == NULL)								\
			(header)->tail = new_entry;										\
	}

#define QUEUE_REM_AFTER( header, prec_entry, rem_entry, field)				\
	{																		\
		if (prec_entry != NULL)												\
			(prec_entry)->field.next = (rem_entry)->field.next;				\
		else																\
		{																	\
			(header)->head = (header)->head->field.next;					\
			if ((header)->head == NULL)										\
				(header)->tail = NULL;										\
		}																	\
	}


/* -----------------------------------------------------------------------------
** SYNOPSIS
**
** Single linked list with only a head pointer in the header.
**
** -----------------------------------------------------------------------------
*/

#define STACK( type)														\
	struct																	\
	{																		\
		struct type *head;													\
	}


#define STACK_DECLARE(name, type)											\
	struct name																\
	{																		\
		struct type *head;													\
	}

     
#define STACK_DEFINE(name)													\
		struct name


#define STACK_ENTRY( type)													\
	struct																	\
	{																		\
		struct type *next;													\
	}


#define STACK_INIT( header)													\
	(header)->head = NULL;


#define STACK_EMPTY( header)												\
	(((header)->head == NULL) ? 1 : 0)

	
#define STACK_HEAD( header)													\
	((header)->head)


#define STACK_NEXT( entry, field)											\
	((entry)->field.next)


#define STACK_ADD_HEAD( header, new_head, field)							\
	{																		\
		(new_head)->field.next = (header)->head;							\
		(header)->head = new_head;											\
	}


#define STACK_REM_HEAD( header, field)										\
	(header)->head = (header)->head->field.next;
	

#define STACK_INSERT_AFTER( header, prec_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.next = (prev_entry)->field.next;					\
		(prev_entry)->field.next = new_entry;								\
	}




/* -----------------------------------------------------------------------------
** SYNOPSIS
**
** Double linked list with only a head pointer in the header.
**
** -----------------------------------------------------------------------------
*/

#define HEAP( type)															\
	struct																	\
	{																		\
		struct type *head;													\
	}


#define HEAP_DECLARE(name, type)											\
	struct name																\
	{																		\
		struct type *head;													\
	}

     
#define HEAP_DEFINE(name)													\
		struct name


#define HEAP_ENTRY( type)													\
	struct																	\
	{																		\
		struct type *next;													\
		struct type *prev;													\
	}


#define HEAP_INIT( header)													\
	(header)->head = NULL;


#define HEAP_EMPTY( header)													\
	(((header)->head == NULL) ? 1 : 0)

	
#define HEAP_HEAD( header)													\
	((header)->head)


#define HEAP_NEXT( entry, field)											\
	((entry)->field.next)


#define HEAP_PREV( entry, field)											\
	((entry)->field.prev)


#define HEAP_ADD_HEAD( header, new_head, field)								\
	{																		\
		(new_head)->field.next = (header)->head;							\
		(new_head)->field.prev = NULL;										\
		if ((header)->head != NULL)											\
			(header)->head->field.prev = new_head;							\
		(header)->head = new_head;											\
	}


#define HEAP_REM_HEAD( header, field)										\
	{																		\
		(header)->head = (header)->head->field.next;						\
		if ((header)->head != NULL)											\
			(header)->head->field.prev = NULL;								\
	}


#define HEAP_INSERT_AFTER( header, prev_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.next = (prev_entry)->field.next;					\
		(new_entry)->field.prev = prev_entry;								\
		(prev_entry)->field.next = new_entry;								\
		if ((new_entry)->field.next != NULL)								\
			(new_entry)->field.next->field.prev = new_entry;				\
	}


#define HEAP_INSERT_BEFORE( header, next_entry, new_entry, field)			\
	{																		\
		(new_entry)->field.prev = (next_entry)->field.prev;					\
		(new_entry)->field.next = next_entry;								\
		(next_entry)->field.prev = new_entry;								\
		if ((new_entry)->field.prev != NULL)								\
			(new_entry)->field.prev->field.next = new_entry;				\
		else																\
			(header)->head = new_entry;										\
	}


#define HEAP_REM_ENTRY( header, entry, field)								\
	{																		\
		if ((entry)->field.prev != NULL)									\
			(entry)->field.prev->field.next = (entry)->field.next;			\
		else																\
			(header)->head = entry->field.next;								\
		if ((entry)->field.next != NULL)									\
			(entry)->field.next->field.prev = (entry)->field.prev;			\
	}




/* -----------------------------------------------------------------------------
** SYNOPSIS
**
** Circular queue with pointer to current head in header.
**
** -----------------------------------------------------------------------------
*/

#define CIRCLEQ( type)														\
	struct																	\
	{																		\
		struct type *head;													\
	}


#define CIRCLEQ_DECLARE(name, type)											\
	struct name																\
	{																		\
		struct type *head;													\
	}

     
#define CIRCLEQ_DEFINE(name)												\
		struct name


#define CIRCLEQ_ENTRY( type)												\
	struct																	\
	{																		\
		struct type *next;													\
		struct type *prev;													\
	}


#define CIRCLEQ_INIT( header)												\
	(header)->head = NULL;

#define CIRCLEQ_SET_HEAD( header, new_head)									\
	(header)->head = new_head;

#define CIRCLEQ_EMPTY( header)												\
	(((header)->head == NULL) ? 1 : 0)


#define CIRCLEQ_HEAD( header)												\
	((header)->head)


#define CIRCLEQ_TAIL( header, field)										\
	 (((header)->head != NULL) ? (header)->head->field.prev : NULL)
	 
	 
#define CIRCLEQ_NEXT( entry, field)											\
	((entry)->field.next)


#define CIRCLEQ_PREV( entry, field)											\
	((entry)->field.prev)


#define CIRCLEQ_FORWARD( header, field)										\
	{																		\
		if ((header)->head != NULL)											\
			(header)->head = (header)->head->field.next;					\
	}


#define CIRCLEQ_REVERSE( header, field)										\
	{																		\
		if ((header)->head != NULL)											\
			(header)->head = (header)->head->field.next;					\
	}


#define CIRCLEQ_ADD_HEAD( header, new_head, field)							\
	{																		\
		if ((header)->head != NULL)											\
		{																	\
			(new_head)->field.next = (header)->head;						\
			(new_head)->field.prev = (header)->head->field.prev;			\
			(header)->head->field.prev = (new_head);						\
			(new_head)->field.prev->field.next = (new_head);				\
		}																	\
		else																\
		{																	\
			(new_head)->field.next = (new_head);							\
			(new_head)->field.prev = (new_head);							\
		}																	\
		(header)->head = (new_head);										\
	}


#define CIRCLEQ_ADD_TAIL( header, new_tail, field)							\
	{																		\
		if ((header)->head != NULL)											\
		{																	\
			(new_tail)->field.next = (header)->head;						\
			(new_tail)->field.prev = (header)->head->field.prev;			\
			(new_tail)->field.next->field.prev = (new_tail);				\
			(new_tail)->field.prev->field.next = (new_tail);				\
		}																	\
		else																\
		{																	\
			(new_tail)->field.next = (new_tail);							\
			(new_tail)->field.prev = (new_tail);							\
			(header)->head = (new_tail);									\
		}																	\
	}


#define CIRCLEQ_REM_HEAD( header, field)									\
	{																		\
		if ((header)->head->field.next != (header)->head)					\
		{																	\
			(header)->head->field.next->field.prev = (header)->head->field.prev;\
			(header)->head->field.prev->field.next = (header)->head->field.next;\
			(header)->head = (header)->head->field.next;					\
		}																	\
		else																\
			(header)->head = NULL;											\
	}


#define CIRCLEQ_REM_TAIL( header, field)									\
	{																		\
		if ((header)->head->field.next != (header)->head)					\
		{																	\
			(header)->head->field.prev->field.prev->field.next = (header)->head;\
			(header)->head->field.prev = (header)->head->field.prev->field.prev;\
		}																	\
		else																\
			(header)->head = NULL;											\
	}


#define CIRCLEQ_REM_ENTRY( header, entry, field)							\
	{																		\
		if ((entry)->field.next != (entry))									\
		{																	\
			(entry)->field.next->field.prev = (entry)->field.prev;			\
			(entry)->field.prev->field.next = (entry)->field.next;			\
			if ((header)->head == (entry))									\
				(header)->head = (entry)->field.next;						\
		}																	\
		else																\
			(header)->head = NULL;											\
	}




#endif
