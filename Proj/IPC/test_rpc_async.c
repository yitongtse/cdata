// ASYNC RPC TEST
//
void ipc_test_rpc (parseinfo *csb)
{
    ipc_message *message;
    ipc_error_code error;
    ulong size = 0;

    rpc_test_port_info.port_features = IPC_PORT_FEAT_RELIABLE |
        IPC_PORT_CALLBACK;
    rpc_test_port_info.notify_callback = ipc_rpc_test_callback;
    rpc_test_port_info.notify_context = NULL;
    error = ipc_open_port_by_name(IPC_MASTER_ECHO_PORT, &rpc_test_port_info);

    printf(open_port_string);
    if (error == IPC_OK) {
        printf(ipc_test_pass);
    } else {
        printf(ipc_test_fail);
        return;
    }

    size = GETOBJ(int, 2);
    if (size < strlen(test_message)+1) {
        /*
         * If user has not given size or given a size less than the test message
         * default to test_message size.
         */
        size = strlen(test_message)+1;
    }

    if (GETOBJ(int,1)) {
        ipc_test_rpc_forever(&rpc_test_port_info, size);
        ipc_close_port(&rpc_test_port_info);
        return;
    }

    /*
     * Allocate our test message
     */
    message = ipc_get_pak_message(size,
                                  &rpc_test_port_info,
                                  IPC_TYPE_SERVER_ECHO);
    printf(message_create_string);
    if (message != NULL) {
        printf(ipc_test_pass);
    } else {
        printf(ipc_test_fail);
        return;
    }

    strcpy(message->data, test_message);

    printf(send_message_string);

    error = ipc_send_rpc(&rpc_test_port_info, message);

    if (error != IPC_OK) {
        printf(ipc_test_fail_resp, ipc_decode_error(error));
        ipc_return_message(message);
    }
}


static void ipc_rpc_test_callback (ipc_message *message,
                                   void *context,
                                   ipc_error_code error)
{
    ipc_message *response;

    if (message == NULL) {
        printf("NULL message!!!\n");
        return;
    }

    if (error != IPC_OK) {
        printf(ipc_test_fail_resp, ipc_decode_error(error));
        ipc_return_message(message);
        return;
    }

    response = (ipc_message *)message->rpc_result;

    ipc_return_message(message);

    if (response == NULL) {
        printf(no_rpc_reply_string);
    } else {
        if (strcmp(test_message, response->data) == 0) {
            printf(data_correct_string);
        } else {
            printf(data_not_correct_string);
        }

        ipc_return_message(response);
    }
}

