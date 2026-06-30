struct arg {
    string in<>;
    int count;
};

struct result {
    string out<>;
};


program ECHO_PROG {
    version ECHO_VERS {
        void ECHOPROC_NULL(void) = 0;
        result ECHOPROC_ECHO(arg) = 1;
    } = 2;
} = 0x20000000;

