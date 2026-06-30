// Priority experiment

typedef struct {
} prio_que_t;


int prio_que_init (prio_que_t *pq, int max_size);

int prio_que_enqueueget (prio_que_t *pq, pq_key_t *key);

pq_key_t* prio_que_dequeue (prio_que_t *pq);

prio_que_is_empty (prio_que_t (pq);
