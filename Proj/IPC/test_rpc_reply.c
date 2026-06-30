 * ipc_test_rpc_proc()
 *
 * Callback routine for RPC testing
 *
 * Inputs:
 * message - message received
 * context - not used
 * error - not used
 *
 * Outputs:
 * NONE
 *
 * Returns:
 * NONE
 */
static void ipc_test_rpc_proc (ipc_message *message,
                               void *context,
                               ipc_error_code error)
{
    ipc_error_code error_code;
    ipc_message *reply_message;

    if ((message->header->flags & IPC_FLAG_IS_RPC) == 0) {
        if (ipc_debug_events) {
            buginf("\nIPC: non RPC message from %x.%x arrived on echo port.",
                   ipc_portid_to_seat(message->header->source_port),
                   ipc_portid_to_port(message->header->source_port));
        }
        ipc_return_message(message);
        return;
    } else {
        reply_message = ipc_get_pak_message(message->header->size, 0, 0);

        if (reply_message == NULL) {
            ipc_return_message(message);
            return;
        }
        printf("\n Current function is ipc_test_rpc_proc()\n");
        bcopy(message->data, reply_message->data, message->header->size);

        error_code = ipc_send_rpc_reply_blocked(message, reply_message);
        if (error_code != IPC_OK) {
            if (ipc_debug_events) {
                buginf("\nIPC: echo port couldn't reply "
                       "to echo request: (%s).\n",
                       ipc_decode_error(error_code));
            }
        }
    }
}
