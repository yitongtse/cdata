typedef struct {
    int qam_id;
    int pid;
} key;


void print_key(key *k)
{
    printf("QAM_id %d, pid %d\n", k->qam_id, k->pid);
}


int main()
{
    int qam_id = 1;
    int pid = 2;

#if 0
// NOTE: Both cases work!
    key k = {qam_id, pid};
    print_key(&k);
#else
    print_key(&(key){qam_id, pid});
#endif

    return 0;
}
