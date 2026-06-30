/* @(#)que.h	1.1  07/15/98  @(#) */
#ifndef _H_QUE_
#define _H_QUE_

/*
 *  ======== queue ========
 *
 *  Queue module.
 * 
 *  Queue is implemented as double link list. If a link list
 *  is empty, it double links to itself.
 */

typedef struct queElem {
    struct queElem  *next;
    struct queElem  *prev;
} QueElem;

/* queue is the pointer to a queue element */
typedef struct queElem *Que;

extern int	queEmpty(Que queue);
extern void	queInit(Que elem);
extern void	queRemove( QueElem *elem );
extern QueElem  *queGet(Que queue);
extern void     quePut(Que queue, QueElem *elem);
extern void     queiPut(Que queue, QueElem *elem);

#endif	/* _H_QUE_ */
