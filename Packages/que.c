/* @(#)que.c	1.1  07/15/98  @(#) */
/*
 *  ======== que.c ========
 *
 *
 */
#include "que.h"

/*
 *  ======== queGet ========
 *
 */
QueElem     *queGet(Que queue)
{
    QueElem     *elem;

    elem = queue->next;
    queue->next = elem->next;
    queue->next->prev = queue;

    return (elem);
}

/*
 *  ======== quePut ========
 *
 */
void quePut(Que queue, QueElem  *elem)
{
    elem->prev = queue->prev;
    elem->next = queue;
    queue->prev->next = elem;
    queue->prev = elem;
}

/*
 *  ======== queiPut ========
 *
 */
void queiPut(Que queue, QueElem  *elem)
{
    elem->prev = queue->prev;
    elem->next = queue;
    queue->prev->next = elem;
    queue->prev = elem;
}

/*
 *  ======== queEmpty ========
 *
 *
 */
int	queEmpty(Que queue)
{
    return ((queue)->next == (queue));
}

/*
 *  ======== queInit ========
 *
 *
 */
void	queInit(Que elem)
{
    elem->next = elem->prev = elem;
}

/*
 *  ======== queRemove ========
 */
void	queRemove( QueElem   *elem )
{
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
}
