#include "queue.h"

void QUEUE_Setup(QUEUE_Ptr_t q, uint8 *buffer, uint32 length)
{
    q->buffer = buffer;
    q->length = length;
    q->head   = buffer;
    q->tail   = buffer;
}

void QUEUE_Push(QUEUE_Ptr_t q, const uint8 *bytes, uint32 length) 
{
    uint32 ii = 0;
    for (ii = 0; ii < length; ii++)
    {
        *q->tail = bytes[ii];
        q->tail++;

        if ((intptr_t)(q->tail - q->buffer) >= q->length)
        {
            q->tail = q->buffer;
        }
    }
}

uint32 QUEUE_Pop(QUEUE_Ptr_t q, uint8 *out_bytes, uint32 length) 
{
    uint32 ii = 0;
    for (ii = 0; ii < length; ii++)
    {
        if (q->head == q->tail)
        {
            /* We're caught up */
            break;
        }
        out_bytes[ii] = *q->head;
        *q->head = 0;
        q->head++;
        if ((intptr_t)(q->head - q->buffer) >= q->length)
        {
            q->head = q->buffer;
        }
    }
    return ii;
}
