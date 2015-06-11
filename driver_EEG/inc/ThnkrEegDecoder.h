#ifndef EEG_TAGM_THNKR_DECODER_H_
#define EEG_TAGM_THNKR_DECODER_H_

#define CRTSCTS  020000000000
#define DEBUG 1 

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* name of the USB Port to connect to */
#define PORT_NAME "/dev/ttyUSB0"
#define MAX_PAYLOAD_SIZE 169
#define MAX_QUEUE_SIZE 10
#define BAUD_RATE B115200

/* Parser types */
#define THNKR_TYPE_NULL       0x00
#define THNKR_TYPE_PACKETS    0x01    /* Stream bytes as ThinkGear Packets */
#define THNKR_TYPE_2BYTERAW   0x02    /* Stream bytes as 2-byte raw data */

/* Data CODE definitions */
#define THNKR_CODE_BATTERY            0x01
#define THNKR_CODE_POOR_QUALITY       0x02
#define THNKR_CODE_ATTENTION          0x04
#define THNKR_CODE_MEDITATION         0x05
#define THNKR_CODE_BLINK              0x16
#define THNKR_CODE_8BITRAW_SIGNAL     0x06
#define THNKR_CODE_RAW_MARKER         0x07

/* Byte codes */
#define THNKR_CODE_LOW_VALUE            0x40
#define THNKR_CODE_RAW_SIGNAL           0x80
#define THNKR_CODE_EEG_POWERS           0x81
#define THNKR_CODE_ASIC_EEG_POWER_INT   0x83
#define THNKR_CODE_CONNECT              0xC0
#define THNKR_CODE_DISCONNECT           0xC1
#define THNKR_CODE_AUTOCONNECT          0xC2
#define THNKR_CODE_CONNECTED            0xD0
#define THNKR_CODE_NOT_FOUND            0xD1
#define THNKR_CODE_DISCONNECTED         0xD2
#define THNKR_CODE_DENIED               0xD3
#define THNKR_CODE_STANDBY_SCAN         0xD4

/* Decoder states (Packet decoding) */
#define THNKR_STATE_NULL           0x00  /* NULL state */
#define THNKR_STATE_SYNC           0x01  /* Waiting for SYNC byte */
#define THNKR_STATE_SYNC_CHECK     0x02  /* Waiting for second SYNC byte */
#define THNKR_STATE_PAYLOAD_LENGTH 0x03  /* Waiting for payload[] length */
#define THNKR_STATE_PAYLOAD        0x04  /* Waiting for next payload[] byte */
#define THNKR_STATE_CHKSUM         0x05  /* Waiting for chksum byte */

/* Decoder states (2-byte raw decoding) */
#define THNKR_STATE_WAIT_HIGH      0x06  /* Waiting for high byte */
#define THNKR_STATE_WAIT_LOW       0x07  /* High r'cvd.  Expecting low part */

/* Other constants */
#define THNKR_SYNC_BYTE            0xAA  /* Syncronization byte */
#define THNKR_EXCODE_BYTE          0x55  /* EXtended CODE level byte */
#define THNKR_MODE_BYTE            0x0F  /* attention enabled, meditation enabled, raw wave enabled, 57.6k baud rate */

void __attribute__ ((constructor)) libmain(void);
void __attribute__ ((destructor)) disconnectAndClose(void);

/**
* The structure to hold our data from the EEG
* delta, theta, low-alpha, high-alpha, low-beta, high-beta, low-gamma, and mid-gamma
*/
typedef struct EegData {
	double attention;
	double meditation;
	double delta;
	double theta;
	double lAlpha;
	double hAlpha;
	double lBeta;
	double hBeta;
	double lGamma;
	double mGamma;
} EegData;

/**
 * The Node struct,
 * contains item and the pointer that point to next node.
 */
typedef struct Node {
    struct EegData item;
    struct Node* next;
} Node;

/**
 * The Queue struct, contains the pointers that
 * point to first node and last node, the size of the Queue,
 * and the function pointers.
 */
typedef struct Queue {
    Node* head;
    Node* tail;

    void (*push) (struct Queue*, EegData); // add item to tail
    // get item from head and remove it from queue
    EegData (*pop) (struct Queue*);
    // get item from head but keep it in queue
    EegData (*peek) (struct Queue*);

    // size of this queue
    int size;
} Queue;

/**
 * Push an item into queue, if this is the first item,
 * both queue->head and queue->tail will point to it,
 * otherwise the oldtail->next and tail will point to it.
 */
void push(Queue* queue, EegData item);

/**
 * Return and remove the first item.
 */
EegData pop(Queue* queue);

/**
 * Return but not remove the first item.
 */
EegData peek(Queue* queue);

/** 
* Initializing the Queue
*/
Queue createQueue();

/**
* Global queue to hold our data
*/
Queue eegDataQueue;

/**
 * The Parser is a state machine that manages the parsing state.
 */
typedef struct ThnkrEegDecoder {
    unsigned char type;
    unsigned char state;

    unsigned char lastByte;

    unsigned char payloadLength;
    unsigned char payloadBytesReceived;
    unsigned char payload[256];
    unsigned char payloadSum;
    unsigned char chksum;

    void (*handleDataValue) (
		unsigned char extendedCodeLevel,
        unsigned char code,
		unsigned char numBytes,
        const unsigned char* value,
		void* customData
	);
    
	void* customData;

} ThnkrEegDecoder;

/* GLOBAL our device TTY */
int dev = 0;

/**
 * @param parser              Pointer to a ThnkrEegDecoder object.
 * @param parserType          One of the THNKR_TYPE_* constants defined above:
 *                            THNKR_TYPE_PACKETS or THNKR_TYPE_2BYTERAW.
 * @param handleDataValueFunc A user-defined callback function that will
 *                            be called whenever a data value is parsed
 *                            from a Packet.
 * @param customData          A pointer to any arbitrary data that will
 *                            also be passed to the handleDataValueFunc
 *                            whenever a data value is parsed from a
 *                            Packet.
 *
 * @return -1 if @c parser is NULL.
 * @return -2 if @c parserType is invalid.
 * @return 0 on success.
 */
int ThnkrEegDecoderInit(
	ThnkrEegDecoder *pParser,
	unsigned char parserType,
    void (*handleDataValueFunc) (
		unsigned char extendedCodeLevel,
        unsigned char code,
		unsigned char numBytes,
        const unsigned char* value,
		void* customData
	),
    void* customData
);

/**
 * Feeds the @c byte into the @c parser.  If the @c byte completes a
 * complete, valid parser, then the @c parser's handleDataValue()
 * function is automatically called on each DataRow in the Packet.
 * The return value provides an indication of the state of the
 * @c parser after parsing the byte.
 *
 * @param parser Pointer to an initialized ThinkGearDataParser object.
 * @param byte   The next byte of the data stream.
 *
 * @return -1 if @c parser is NULL.
 * @return -2 if a complete Packet was received, but the checksum failed.
 * @return -3 if an invalid Packet with PLENGTH > 170 was detected.
 * @return -4 if an invalid Packet with PLENGTH == 170 was detected.
 * @return -5 if the @c parser is somehow in an unrecognized state.
 * @return 0 if the @c byte did not yet complete a Packet.
 * @return 1 if a Packet was received and parsed successfully.
 *
 */
int ThnkrEegDecoderParse(
	ThnkrEegDecoder* pParser,
	unsigned char byte
);

/**
* Function which acts on the value[] bytes of each ThinkGear DataRow as it is received.
*/
void handleDataValueFunc(
	unsigned char extendedCodeLevel,
	unsigned char code,
	unsigned char valueLength,
	const unsigned char *value,
	void *customData
);

/**
* Sets the TTY (USB) interface attributes
*/
int setInterfaceAttributes(
	int fd,
	int speed,
	int parity
);

/**
* This is the main entry point of the library
* and it will run the initialize function in a separate thread
*/
void libmain();

/**
* Initializez the ThnkGearEegConnector
*/
void* initialize(void* args);


/**
* isconnect and close PORT
*/
void disconnectAndClose();


/**
* EXPORTED function that gets data from the buffer
**/
extern char* getThnkrDataJSON();


#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* EEG_TAGM_THNKR_DECODER_H_ */
