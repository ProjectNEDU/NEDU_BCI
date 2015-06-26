#include "ThnkrEegDecoder.h"

/* Declare private function prototypes */
int parsePacketPayload(
	ThnkrEegDecoder *pParser
);

int parseDataRow(
	ThnkrEegDecoder* pParser,
	unsigned char* rowPtr
);

/*
 * See header file for interface documentation.
 */
int ThnkrEegDecoderInit(
	ThnkrEegDecoder* pParser,
    unsigned char parserType,
    void (*handleDataValueFunc) (
		unsigned char extendedCodeLevel,
        unsigned char code,
		unsigned char numBytes,
        const unsigned char *value,
		void *customData
	),
    void *customData
) {

    if(!pParser) return -1;

    /* Initialize the parser's state based on the parser type */
    switch(parserType) {
        case THNKR_TYPE_PACKETS:
            pParser->state = THNKR_STATE_SYNC;
            break;
			
        case THNKR_TYPE_2BYTERAW:
            pParser->state = THNKR_STATE_WAIT_HIGH;
            break;
			
        default:
			return -2;
    }

    /* Save parser type */
    pParser->type = parserType;

    /* Save user-defined handler function and data pointer */
    pParser->handleDataValue = handleDataValueFunc;
    pParser->customData = customData;

    return 0;
}

int ThnkrEegDecoderParse(
	ThnkrEegDecoder* pParser,
	unsigned char byte
) {
    int returnValue = 0;

    if(!pParser) return -1;

    // Pick handling according to current state... 
    switch(pParser->state) {

        // Waiting for SyncByte 
        case THNKR_STATE_SYNC:
            if( byte == THNKR_SYNC_BYTE ) {
                pParser->state = THNKR_STATE_SYNC_CHECK;
            }
            break;

        // Waiting for second SyncByte
        case THNKR_STATE_SYNC_CHECK:
            if(byte == THNKR_SYNC_BYTE) {
                pParser->state = THNKR_STATE_PAYLOAD_LENGTH;
            } else {
                pParser->state = THNKR_STATE_SYNC;
            }
            break;

        // Waiting for Data[] length 
        case THNKR_STATE_PAYLOAD_LENGTH:
            pParser->payloadLength = byte;
			
			if(pParser->payloadLength == MAX_PAYLOAD_SIZE) {
				pParser->state = THNKR_STATE_SYNC;
                returnValue = -2;
            } else if(pParser->payloadLength > MAX_PAYLOAD_SIZE) {
                pParser->state = THNKR_STATE_SYNC;
                returnValue = -3;
            } else {
                pParser->payloadBytesReceived = 0;
                pParser->payloadSum = 0;
                pParser->state = THNKR_STATE_PAYLOAD;
            }
            break;

        // Waiting for Payload[] bytes
        case THNKR_STATE_PAYLOAD:
            pParser->payload[pParser->payloadBytesReceived++] = byte;
            pParser->payloadSum = (unsigned char)(pParser->payloadSum + byte);
            
			if(pParser->payloadBytesReceived >= pParser->payloadLength) {
                pParser->state = THNKR_STATE_CHKSUM;
            }
            break;

        // Waiting for CKSUM byte
        case THNKR_STATE_CHKSUM:
            pParser->chksum = byte;
            pParser->state = THNKR_STATE_SYNC;

			if(pParser->chksum != ((~pParser->payloadSum) & 0xFF)) {
                returnValue = -2;
            } else {
                returnValue = 1;
                parsePacketPayload(pParser);
            }
            break;

        // Waiting for high byte of 2-byte raw value
        case THNKR_STATE_WAIT_HIGH:
            // Check if current byte is a high byte
            if((byte & THNKR_CODE_CONNECT) == THNKR_CODE_RAW_SIGNAL) {
                // High byte recognized, will be saved as parser->lastByte
                pParser->state = THNKR_STATE_WAIT_LOW;
            }
            break;

        // Waiting for low byte of 2-byte raw value
        case THNKR_STATE_WAIT_LOW:
            // Check if current byte is a valid low byte
            if((byte & THNKR_CODE_CONNECT) == THNKR_CODE_LOW_VALUE) {

                // Stuff the high and low part of the raw value into an array
                pParser->payload[0] = pParser->lastByte;
                pParser->payload[1] = byte;

                // Notify the handler function of received raw value
                if(pParser->handleDataValue) {
                    pParser->handleDataValue(
						0,
						THNKR_CODE_RAW_SIGNAL,
						2,
                        pParser->payload,
                        pParser->customData
					);
                }

                returnValue = 1;
            }

            // Return to start state waiting for high
            pParser->state = THNKR_STATE_WAIT_HIGH;

            break;

        // unrecognized state
        default:
            pParser->state = THNKR_STATE_SYNC;
            returnValue = -5;
            break;
    }

    // Save current byte
    pParser->lastByte = byte;

    return returnValue;

return 0;
}

int parsePacketPayload(
	ThnkrEegDecoder* pParser
) {

    unsigned char i = 0;
    unsigned char extendedCodeLevel = 0;
    unsigned char code = 0;
    unsigned char numBytes = 0;

    /* Parse all bytes from the payload[] */
    while(i < pParser->payloadLength) {

        /* Parse possible EXtended CODE bytes */
        while(pParser->payload[i] == THNKR_EXCODE_BYTE) {
            extendedCodeLevel++;
            i++;
        }

        /* Parse CODE */
        code = pParser->payload[i++];

        /* Parse value length */
        if(code >= THNKR_CODE_RAW_SIGNAL) {
			numBytes = pParser->payload[i++];
        } else {
			numBytes = 1;
		}

        /* Call the callback function to handle the DataRow value */
        if(pParser->handleDataValue) {
            pParser->handleDataValue(
				extendedCodeLevel,
				code,
				numBytes,
                pParser->payload + i,
				pParser->customData
			);
        }
		
        i = (unsigned char)(i + numBytes);
    }

    return 0;
}

void handleDataValueFunc(
	unsigned char extendedCodeLevel,
	unsigned char code,
	unsigned char valueLength,
	const unsigned char *value,
	void *customData
) {
	if(extendedCodeLevel == 0) {
		EegData eegItem = {
			.attention = 0,
			.meditation = 0,
			.delta = 0.0f,
			.theta = 0.0f,
			.lAlpha = 0.0f,
			.hAlpha = 0.0f,
			.lBeta = 0.0f,
			.hBeta = 0.0f,
			.lGamma = 0.0f,
			.mGamma = 0.0f
		};

		switch(code) {

			case THNKR_CODE_ATTENTION:
				eegItem.attention = value[0] & 0xFF;
			break;
				
			case THNKR_CODE_MEDITATION:
				eegItem.meditation = value[0] & 0xFF;
			break;
			
			case THNKR_CODE_ASIC_EEG_POWER_INT:
			
			/**
			 * This Data Value represents the current magnitude of 8 commonly-recognized types of EEG (brainwaves).
			 * It is the ASIC equivalent of EEG_POWER, with the main difference being that this Data Value is output as a series of
			 * eight 3-byte unsigned integers instead of 4-byte floating point numbers.
			 * These 3-byte unsigned integers are in big-endian format.
			**/
			
				eegItem.delta = (value[0] << 16) | (value[1] << 8) | value[2];
				eegItem.theta = (value[3] << 16) | (value[4] << 8) | value[5];
				eegItem.lAlpha = (value[6] << 16) | (value[7] << 8) | value[8];
				eegItem.hAlpha = (value[9] << 16) | (value[10] << 8) | value[11];
				eegItem.lBeta = (value[12] << 16) | (value[13] << 8) | value[14];
				eegItem.hBeta = (value[15] << 16) | (value[16] << 8) | value[17];
				eegItem.lGamma = (value[18] << 16) | (value[19] << 8) | value[20];
				eegItem.mGamma = (value[21] << 16) | (value[22] << 8) | value[23];
				
				eegDataQueue.push(&eegDataQueue, eegItem);
			break;
			
			/* Other [CODE]s */
			default:
				// not implemented yet
			break;
		}
	}
}

char* getThnkrDataJSON() {
	
	if(eegDataQueue.size == 0) return ""; 
	
	EegData eegItem = eegDataQueue.pop(&eegDataQueue);
	
	// {"attention":"","meditation":"","delta":"","theta":"","low_alpha":"","high_alpha":"","low_beta":"","high_beta":"","low_gamma":"","mid_gamma":""}
	char num[25];
	char* buf = (char*)malloc(255 * sizeof(char));
	
	strncpy(buf, "{\"attention\":\"", 14);
	snprintf(num, 25, "%d", eegItem.attention);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"meditation\":\"", 16);
	snprintf(num, 25, "%d", eegItem.attention);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"delta\":\"", 11);
	snprintf(num, 25, "%d", eegItem.delta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"theta\":\"", 11);
	snprintf(num, 25, "%d", eegItem.theta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_alpha\":\"", 15);
	snprintf(num, 25, "%d", eegItem.lAlpha);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"high_alpha\":\"", 16);
	snprintf(num, 25, "%d", eegItem.hAlpha);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_beta\":\"", 14);
	snprintf(num, 25, "%d", eegItem.lBeta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"high_beta\":\"", 15);
	snprintf(num, 25, "%d", eegItem.hBeta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_gamma\":\"", 15);
	snprintf(num, 25, "%d", eegItem.lGamma);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"mid_gamma\":\"", 15);
	snprintf(num, 25, "%d", eegItem.mGamma);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\"}\0", 3);
	
	return buf;
}

int setInterfaceAttributes(
	int fd,
	int speed,
	int parity
) {
	struct termios tty;
	memset(&tty, 0, sizeof(tty));
	
	if(tcgetattr(fd, &tty) != 0) return errno;

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
									// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
									// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if(tcsetattr(fd, TCSANOW, &tty) != 0) return errno;
			
	return 0;
};

void* initialize(void* args) {

	eegDataQueue = createQueue();
	
	char conBuf;
	
	dev = open(PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC);
	setInterfaceAttributes(dev, BAUD_RATE, 0);
	
	conBuf = (char)THNKR_CODE_DISCONNECT;
	write(dev, (char*)&conBuf, 1);
	sleep(5);
	
	conBuf = (char)THNKR_CODE_AUTOCONNECT;
	write(dev, (char*)&conBuf, 1);
	sleep(5);
	
	ThnkrEegDecoder parser;
	ThnkrEegDecoderInit(&parser, THNKR_TYPE_PACKETS, handleDataValueFunc, NULL);
	
	size_t bufSize  = 1 * sizeof(char);
	unsigned char buf = 0;
	while(1) {
		read(dev, &buf, bufSize);
		ThnkrEegDecoderParse(&parser, buf);
	}
}

void disconnectAndClose() {
	if(dev != 0) {
		write(dev, (char*)THNKR_CODE_DISCONNECT, 1);
		close(dev);
	}
}

void libmain() {
	pthread_t tid = NULL;
	int err = pthread_create(&tid, NULL, &initialize, NULL);
	
	if(err != 0) printf("\ncan't create thread :[%s]", strerror(err));
}

void push(
	Queue* queue,
	EegData item
) {
	if(queue->size > MAX_QUEUE_SIZE) {
		queue->pop(queue);
	}
	
    Node* n = (Node*)malloc(sizeof(Node));
    n->item = item;
    n->next = NULL;

    if (queue->head == NULL) { // no head
        queue->head = n;
    } else{
        queue->tail->next = n;
    }
    queue->tail = n;
    queue->size++;
}

EegData pop(
	Queue* queue
) {
    Node* head = queue->head;
    EegData item = head->item;
    queue->head = head->next;
    queue->size--;
    free(head);
	
    return item;
}

EegData peek(
	Queue* queue
) {
    Node* head = queue->head;
    return head->item;
}

Queue createQueue() {
    Queue queue;
    queue.size = 0;
    queue.head = NULL;
    queue.tail = NULL;
    queue.push = &push;
    queue.pop = &pop;
    queue.peek = &peek;
	
    return queue;
}
