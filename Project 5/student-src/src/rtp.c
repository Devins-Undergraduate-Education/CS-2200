#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "queue.h"
#include "network.h"
#include "rtp.h"

/**
 * PLESE ENTER YOUR INFORMATION BELOW TO RECEIVE MANUAL GRADING CREDITS
 * Name: Devin Fromond
 * GTID: 903761713
 * Spring 2024
 */

typedef struct message {
    char *buffer;
    int length;
} message_t;

/* ================================================================ */
/*                  H E L P E R    F U N C T I O N S                */
/* ================================================================ */

/**
 * --------------------------------- PROBLEM 1 --------------------------------------
 * 
 * Convert the given buffer into an array of PACKETs and returns the array.  The 
 * value of (*count) should be updated so that it contains the number of packets in 
 * the array. Keep in mind that if length % MAX_PAYLOAD_LENGTH != 0, you might have
 * to manually specify the payload_length.
 * 
 * Hint: can we use a heap function to make space for the packets? How many packets?
 * 
 * @param buffer pointer to message buffer to be broken up packets
 * @param length length of the message buffer.
 * @param count number of packets in the returning array
 * 
 * @returns array of packets
 */
packet_t *packetize(char *buffer, int length, int *count) {

    packet_t * packets;

    /* ----  FIXME  ---- */

    // calculate number of packets needed
    int numPackets = length / MAX_PAYLOAD_LENGTH;
    int remainingLength = MAX_PAYLOAD_LENGTH;

    // edge case logic where we manually specify payload_length
    if (length % MAX_PAYLOAD_LENGTH) {
        remainingLength = length % MAX_PAYLOAD_LENGTH;
        numPackets++;
    }

    // allocate memory for packets
    packets = malloc(sizeof(packet_t) * (size_t) numPackets);

    // create packets
    for (int i = 0; i < numPackets - 1; i++) {
        // add regular data packets
        packets[i].payload_length = MAX_PAYLOAD_LENGTH;
        packets[i].type = DATA;

        // fill payload with data from buffer
        for (int j = 0; j < MAX_PAYLOAD_LENGTH; j++) {
            packets[i].payload[j] = buffer[i * MAX_PAYLOAD_LENGTH + j];
        }
        
        // calculate checksum and hope it works
        packets[i].checksum = checksum(packets[i].payload, MAX_PAYLOAD_LENGTH);
    }
    
    // update last data packets
    packets[numPackets - 1].type = LAST_DATA;
    packets[numPackets - 1].payload_length = remainingLength;

    // fill payload with remaining packets (overflow)
    for (int i = 0; i < remainingLength; i++) {
        packets[numPackets - 1].payload[i] = buffer[(numPackets - 1) * MAX_PAYLOAD_LENGTH + i];
    }
    
    // calculate checksum for overflow
    packets[numPackets - 1].checksum = checksum(packets[numPackets - 1].payload, remainingLength);

    // update packet count
    * count = numPackets;

    // return packets
    return packets;
}

/**
 * --------------------------------- PROBLEM 2 --------------------------------------
 * 
 * Compute a checksum based on the data in the buffer.
  * 
 * Checksum calcuation: 
 * Suppose we are given a string in the form 
 *      "a_1 a_2 a_3 b_1 b_2 b_3"
 * Then shuffle the string such that the characters appear in the form 
 *      "a_1 b_1 a_2 b_2 a_3 b_3"
 * 
 * i.e. reorder the string such that the first and second half elements appear one after the other.
 * Following this, the checksum will be equal to the sum of the ASCII values of the first two 
 * characters multiplied by 2, plus the ASCII values of the next two characters divided by 2, and so on. 
 * 
 * Remember to account for odd length strings.
 * 
 * @param buffer pointer to the char buffer that the checksum is calculated from
 * @param length length of the buffer
 * 
 * @returns calcuated checksum
 */
int checksum(char* buffer, int length) {

    // initalize variables
    int index_start = 0;
    int index_mid = length / 2;
    int checksum_value = 0;
    int position_counter = 0;

    // initalize array to be returned (NOT using malloc)
    char reordered_string[length + 1];
    reordered_string[0] = '\0';  // add null char

    // add even/odd logic
    int lastElement;
    if (length % 2 == 0) {
        lastElement = length; 
    } else {
        lastElement = length - 1;
    }
    
    // do some funky stuff that's left over from strcat
    char bufferString[2]; // it's a string of one char....
    bufferString[1] = '\0'; // can confirm...

    // reorder logic... please work
    for (int k = 0; k < lastElement; k++) {
        if (k % 2 == 0) {
            bufferString[0] = buffer[index_start];
            reordered_string[k] = bufferString[0];
            index_start++;
        } else {
            bufferString[0] = buffer[index_mid];
            reordered_string[k] = bufferString[0];
            index_mid += 1;
        }
    }

    // add null terminator
    reordered_string[lastElement] = '\0';  
    
    // add odd logic because of weird stuff
    // this was the source of weird stuff in the old rtp-server.py file... souts out for updating...
    if (length % 2 != 0) {
        bufferString[0] = buffer[length - 1];
        reordered_string[lastElement] = bufferString[0];
        reordered_string[lastElement + 1] = '\0';  // Add null terminator at the end
    }
    
    // sum logic.... this works (hopfully) (gaslight)
    for (int i = 0; i < length; i++) {
        // position_counter %= 2;
        position_counter %= 4;
        if (position_counter == 0 || position_counter == 1) {
            checksum_value += reordered_string[i] * 2;
        } else {
            checksum_value += reordered_string[i] / 2;
        }
        position_counter++;
    }

    // return calculated sum
    return checksum_value;
}


/* ================================================================ */
/*                      R T P       T H R E A D S                   */
/* ================================================================ */

static void *rtp_recv_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;

    do {
        message_t *message;
        int buffer_length = 0;
        char *buffer = NULL;
        packet_t packet;

        /* Put messages in buffer until the last packet is received  */
        do {
            if (net_recv_packet(connection->net_connection_handle, &packet) <= 0 || packet.type == TERM) {
                /* remote side has disconnected */
                connection->alive = 0;
                pthread_cond_signal(&connection->recv_cond);
                pthread_cond_signal(&connection->send_cond);
                break;
            }

            /*  ----  FIXME: Part III-A ----
            *
            * 1. Check to make sure payload of packet is correct
            * 2. Send an ACK or a NACK, whichever is appropriate
            * 3. If this is the last packet in a sequence of packets
            *    and the payload was corrupted, make sure the loop
            *    does not terminate
            * 4. If the payload matches, add the payload to the buffer
            */
            
            // for every packet, allocate space for a return packet (for N/ACK)
            if (packet.type == DATA || packet.type == LAST_DATA) {
                packet_t * returnedPacket = malloc(sizeof(packet_t));

                // checksum logic (Yes? --> ACK) (No... --> NACK)
                if (checksum(packet.payload, packet.payload_length) != packet.checksum) {
                    // UH OH... CHECKSUM NO MATCHY... NACK
                    (* returnedPacket).type = NACK;

                    // update edge case of last packet
                    if (packet.type == LAST_DATA) {
                        packet.type = DATA;
                    }

                } else {
                    // send ACK
                    (* returnedPacket).type = ACK;

                    // allocate space correctly
                    buffer = buffer ? realloc(buffer, (unsigned long) (buffer_length + packet.payload_length)) : malloc((unsigned long) packet.payload_length);

                    // add buffer logic
                    for (int i = 0; i < packet.payload_length; i++) {
                        buffer[buffer_length + i] = packet.payload[i];
                    }

                    // update buffer length
                    buffer_length += packet.payload_length;

                }
                
                // send the packet!
                net_send_packet((* connection).net_connection_handle, returnedPacket);

                // free allocated memory
                free(returnedPacket);

            } else if (packet.type == NACK || packet.type == ACK) {
                // locks to ensure we do not send multiple ack
                pthread_mutex_lock(&(*connection).ack_mutex);
                // send... and signal/unlock
                (* connection).ack = 1;
                if(packet.type != ACK) {
                    (* connection).ack++;
                }
                pthread_cond_signal(&(*connection).ack_cond);
                pthread_mutex_unlock(&(*connection).ack_mutex);
           }

            /*
            *  What if the packet received is not a data packet?
            *  If it is a NACK or an ACK, the sending thread should
            *  be notified so that it can finish sending the message.
            *   
            *  1. Add the necessary fields to the CONNECTION data structure
            *     in rtp.h so that the sending thread has a way to determine
            *     whether a NACK or an ACK was received
            *  2. Signal the sending thread that an ACK or a NACK has been
            *     received.
            */


        } while (packet.type != LAST_DATA);

        if (connection->alive == 1) {

            /*  ----  FIXME: Part III-B ----
            *
            * Now that an entire message has been received, we need to
            * add it to the queue to provide to the rtp client.
            *
            * 1. Add message to the received queue.
            * 2. Signal the client thread that a message has been received.
            */

            // allocate space for message
            message = malloc(sizeof(message_t));

            // set variables for message
            ( * message).length = buffer_length;
            ( * message).buffer = buffer;

            // lock so we do not add message to queue multiple itmes
            pthread_mutex_lock(&(*connection).recv_mutex);
            // add to queue
            queue_add(&(*connection).recv_queue, message);
            // unlock and signal
            pthread_mutex_unlock(&(*connection).recv_mutex); 
            pthread_cond_signal(&(*connection).recv_cond);

        } else free(buffer);

    } while (connection->alive == 1);

    return NULL;
}

static void *rtp_send_thread(void *void_ptr) {

    rtp_connection_t *connection = (rtp_connection_t *) void_ptr;
    message_t *message;
    int array_length = 0;
    int i;
    packet_t *packet_array;

    do {
        /* Extract the next message from the send queue */
        pthread_mutex_lock(&connection->send_mutex);
        while (queue_size(&connection->send_queue) == 0 && connection->alive == 1) {
            pthread_cond_wait(&connection->send_cond, &connection->send_mutex);
        }

        if (connection->alive == 0) {
            pthread_mutex_unlock(&connection->send_mutex);
            break;
        }

        message = queue_extract(&connection->send_queue);

        pthread_mutex_unlock(&connection->send_mutex);

        /* Packetize the message and send it */
        packet_array = packetize(message->buffer, message->length, &array_length);

        for (i = 0; i < array_length; i++) {

            /* Start sending the packetized messages */
            if (net_send_packet(connection->net_connection_handle, &packet_array[i]) <= 0) {
                /* remote side has disconnected */
                connection->alive = 0;
                break;
            }

            /*  ----FIX ME: Part III-C ---- 
             * 
             *  1. Wait for the recv thread to notify you of when a NACK or
             *     an ACK has been received
             *  2. Check the data structure for this connection to determine
             *     if an ACK or NACK was received.  (You'll have to add the
             *     necessary fields yourself)
             *  3. If it was an ACK, continue sending the packets.
             *  4. If it was a NACK, resend the last packet
             */
            
            // lock and wait for condition as per instructions
            pthread_mutex_lock(&(*connection).ack_mutex);

            // wait for signal
            while (!(*connection).ack) {
                pthread_cond_wait(&(*connection).ack_cond, &(*connection).ack_mutex);
            }

            // update vars accordingly
            if ((*connection).ack == 2) {
                i--;
            }

            // update ack
            ( * connection).ack = 0;

            // unlock... yay!
            pthread_mutex_unlock( & ( * connection).ack_mutex);
        }

        free(packet_array);
        free(message->buffer);
        free(message);
    } while (connection->alive == 1);
    return NULL;
}

/* ================================================================ */
/*                           R T P    A P I                         */
/* ================================================================ */

static rtp_connection_t *rtp_init_connection(int net_connection_handle) {
    rtp_connection_t *rtp_connection = malloc(sizeof(rtp_connection_t));

    if (rtp_connection == NULL) {
        fprintf(stderr, "Out of memory!\n");
        exit(EXIT_FAILURE);
    }

    rtp_connection->net_connection_handle = net_connection_handle;

    queue_init(&rtp_connection->recv_queue);
    queue_init(&rtp_connection->send_queue);

    pthread_mutex_init(&rtp_connection->ack_mutex, NULL);
    pthread_mutex_init(&rtp_connection->recv_mutex, NULL);
    pthread_mutex_init(&rtp_connection->send_mutex, NULL);
    pthread_cond_init(&rtp_connection->ack_cond, NULL);
    pthread_cond_init(&rtp_connection->recv_cond, NULL);
    pthread_cond_init(&rtp_connection->send_cond, NULL);

    rtp_connection->alive = 1;

    pthread_create(&rtp_connection->recv_thread, NULL, rtp_recv_thread,
                   (void *) rtp_connection);
    pthread_create(&rtp_connection->send_thread, NULL, rtp_send_thread,
                   (void *) rtp_connection);

    return rtp_connection;
}

rtp_connection_t *rtp_connect(char *host, int port) {

    int net_connection_handle;

    if ((net_connection_handle = net_connect(host, port)) < 1)
        return NULL;

    return (rtp_init_connection(net_connection_handle));
}

int rtp_disconnect(rtp_connection_t *connection) {

    message_t *message;
    packet_t term;

    term.type = TERM;
    term.payload_length = term.checksum = 0;
    net_send_packet(connection->net_connection_handle, &term);
    connection->alive = 0;

    net_disconnect(connection->net_connection_handle);
    pthread_cond_signal(&connection->send_cond);
    pthread_cond_signal(&connection->recv_cond);
    pthread_join(connection->send_thread, NULL);
    pthread_join(connection->recv_thread, NULL);
    net_release(connection->net_connection_handle);

    /* emtpy recv queue and free allocated memory */
    while ((message = queue_extract(&connection->recv_queue)) != NULL) {
        free(message->buffer);
        free(message);
    }
    queue_release(&connection->recv_queue);

    /* emtpy send queue and free allocated memory */
    while ((message = queue_extract(&connection->send_queue)) != NULL) {
        free(message);
    }
    queue_release(&connection->send_queue);

    free(connection);

    return 1;

}

int rtp_recv_message(rtp_connection_t *connection, char **buffer, int *length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;
    /* lock */
    pthread_mutex_lock(&connection->recv_mutex);
    while (queue_size(&connection->recv_queue) == 0 && connection->alive == 1) {
        pthread_cond_wait(&connection->recv_cond, &connection->recv_mutex);
    }

    if (connection->alive == 0) {
        pthread_mutex_unlock(&connection->recv_mutex);
        return -1;
    }

    /* extract */
    message = queue_extract(&connection->recv_queue);
    *buffer = message->buffer;
    *length = message->length;
    free(message);

    /* unlock */
    pthread_mutex_unlock(&connection->recv_mutex);

    return *length;
}

int rtp_send_message(rtp_connection_t *connection, char *buffer, int length) {

    message_t *message;

    if (connection->alive == 0)
        return -1;

    message = malloc(sizeof(message_t));
    if (message == NULL) {
        return -1;
    }
    message->buffer = malloc((size_t) length);
    message->length = length;

    if (message->buffer == NULL) {
        free(message);
        return -1;
    }

    memcpy(message->buffer, buffer, (size_t) length);

    /* lock */
    pthread_mutex_lock(&connection->send_mutex);

    /* add */
    queue_add(&(connection->send_queue), message);

    /* unlock */
    pthread_mutex_unlock(&connection->send_mutex);
    pthread_cond_signal(&connection->send_cond);
    return 1;

}
