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
	unsigned char* bytes
) {
	unsigned char* payLoad;
	unsigned short payLoadSize = 0;
	unsigned short checkSum = 0;
	unsigned short excodeCount = 0;
	
	if(bytes[0] == SYNC && bytes[1] == SYNC) {
		read(dev, (char*)&payLoadSize, 1);
		if(payLoadSize <= MAX_PAYLOAD_SIZE && payLoadSize > 0) {
			payLoad = (unsigned char*)malloc(payLoadSize * sizeof(char));
			read(dev, payLoad, payLoadSize);
			read(dev, (char*)&checkSum, 1);
			
			unsigned short it = payLoadSize - 1;
			int checkSumCalc = payLoad[0];
			while(it > 0) {
				checkSumCalc += payLoad[it--];
			}
			checkSumCalc = ((~checkSumCalc) & 0xFF);
			
			printf("checkSumCalc is: 0x%x and checkSum is: 0x%x\n", checkSumCalc, checkSum);
			printf("payLoadSize is: %d\n", payLoadSize);
			for(int i = 0; i < (size_t)payLoadSize; i++) {
				printf("byte %i is: %x\n", i, payLoad[i]);
			}
			printf("================================================\n");
			
			if(checkSumCalc == checkSum) {
				it = 0;
				while(payLoad[it] == EXCODE && it < payLoadSize) {
					excodeCount++;
				}
				
				if(payLoad[excodeCount] < THNKR_CODE_RAW_SIGNAL) {
					switch(payLoad[excodeCount]) {
#ifdef DEBUG
							printf("\tMULTI/SINGLE is: SINGLE\n");
#endif
						case POOR_SIGNAL:
#ifdef DEBUG
							printf("CODE is: POOR_SIGNAL\n");
#endif
							break;
							
						case ATTENTION:
#ifdef DEBUG
							printf("CODE is: ATTENTION\n");
#endif
							break;
							
						case MEDITATION:
#ifdef DEBUG
							printf("CODE is: MEDITATION\n");
#endif
							break;
							
						case BLINK:
#ifdef DEBUG
							printf("CODE is: BLINK\n");
#endif
							break;
							
						default:
#ifdef DEBUG
							printf("CODE is: UNKNOWN\n");
#endif
							break;
					}
				} else {
					switch(payLoad[excodeCount]) {
#ifdef DEBUG
							printf("\tMULTI/SINGLE is: MULTI\n");
#endif
					case RAW_VALUE:
#ifdef DEBUG
							printf("CODE is: RAW_VALUE\n");
#endif
							break;
							
						case HEADSET_CONNECTED:
#ifdef DEBUG
							printf("CODE is: HEADSET_CONNECTED\n");
#endif
							break;
							
						case HEADSET_NOT_FOUND:
#ifdef DEBUG
							printf("CODE is: HEADSET_NOT_FOUND\n");
#endif
							break;
							
						case HEADSET_DISCONNECTED:
#ifdef DEBUG
							printf("CODE is: HEADSET_DISCONNECTED\n");
#endif
							break;
							
						case REQUEST_DENIED:
#ifdef DEBUG
							printf("CODE is: REQUEST_DENIED\n");
#endif
							break;
							
						case STANDBY_SCAN:
#ifdef DEBUG
							printf("CODE is: STANDBY_SCAN\n");
#endif
							break;
							
						default:
#ifdef DEBUG
							printf("CODE is: UNKNOWN\n");
#endif
							break;
					}
				}
			}
		}
	}
/*
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
			
            if(pParser->payloadLength > 170) {
                pParser->state = THNKR_STATE_SYNC;
                returnValue = -3;
            } else if(pParser->payloadLength == 170) {
                returnValue = -4;
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
            if((byte & 0xC0) == 0x80) {
                // High byte recognized, will be saved as parser->lastByte
                pParser->state = THNKR_STATE_WAIT_LOW;
            }
            break;

        // Waiting for low byte of 2-byte raw value
        case THNKR_STATE_WAIT_LOW:
            // Check if current byte is a valid low byte
            if((byte & 0xC0) == 0x40) {

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
*/
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
        if(code >= 0x80) {
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
                pParser->payload+i,
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
			.delta = 0,
			.theta = 0,
			.lAlpha = 0,
			.hAlpha = 0,
			.lBeta = 0,
			.hBeta = 0,
			.lGamma = 0,
			.mGamma = 0
		};
		
		switch(code) {
			/* [CODE]: ATTENTION eSense */
			case(0x04):
				eegItem.attention = value[0] & 0xFF;
			break;
			/* [CODE]: MEDITATION eSense */
			case(0x05):
				eegItem.meditation = value[0] & 0xFF;
			break;
			/* Other [CODE]s */
			default:
				eegItem.delta = value[0] & 0xFF;
				eegItem.theta = value[1] & 0xFF;
				eegItem.lAlpha = value[2] & 0xFF;
				eegItem.hAlpha = value[3] & 0xFF;
				eegItem.lBeta = value[4] & 0xFF;
				eegItem.hBeta = value[5] & 0xFF;
				eegItem.lGamma = value[6] & 0xFF;
				eegItem.mGamma = value[7] & 0xFF;
			break;
		}
		
		eegDataQueue.push(&eegDataQueue, eegItem);
	}
}

char* getThnkrDataJSON() {
	
	if(eegDataQueue.size == 0) return ""; 
	
	EegData eegItem = eegDataQueue.pop(&eegDataQueue);
	
	// {"attention":"","meditation":"","delta":"","theta":"","low_alpha":"","high_alpha":"","low_beta":"","high_beta":"","low_gamma":"","mid_gamma":""}
	char num[25];
	char* buf = (char*)malloc(255 * sizeof(char));
	
	strncpy(buf, "{\"attention\":\"", 14);
	snprintf(num, 25,"%f", eegItem.attention);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"meditation\":\"", 16);
	snprintf(num, 25,"%f", eegItem.attention);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"delta\":\"", 11);
	snprintf(num, 25,"%f", eegItem.delta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"theta\":\"", 11);
	snprintf(num, 25,"%f", eegItem.theta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_alpha\":\"", 15);
	snprintf(num, 25,"%f", eegItem.lAlpha);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"high_alpha\":\"", 16);
	snprintf(num, 25,"%f", eegItem.hAlpha);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_beta\":\"", 14);
	snprintf(num, 25,"%f", eegItem.lBeta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"high_beta\":\"", 15);
	snprintf(num, 25,"%f", eegItem.hBeta);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"low_gamma\":\"", 15);
	snprintf(num, 25,"%f", eegItem.lGamma);
	strncat(buf, num, strlen(num));
	
	strncat(buf, "\",\"mid_gamma\":\"", 15);
	snprintf(num, 25,"%f", eegItem.mGamma);
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

int initialize() {
	eegDataQueue = createQueue();
	
	char conBuf;
	
	dev = open(PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC);
	if(setInterfaceAttributes(dev, BAUD_RATE, 0) != 0) return errno;
	
	conBuf = (char)DISCONNECT;
	write(dev, (char*)&conBuf, 1);
	sleep(5);
	
	conBuf = (char)AUTOCONNECT;
	write(dev, (char*)&conBuf, 1);
	sleep(5);
	
	ThnkrEegDecoder parser;
	ThnkrEegDecoderInit(&parser, THNKR_TYPE_PACKETS, handleDataValueFunc, NULL);
	
	size_t bufSize  = 2 * sizeof(char);
	unsigned char* buf = (unsigned char*)malloc(bufSize);
	while(1) {
		read(dev, (unsigned char*)buf, bufSize);
		ThnkrEegDecoderParse(&parser, buf);
	}
	
	return 0;
}

void disconnectAndClose() {
	if(dev != 0) {
		write(dev, (char*)DISCONNECT, 1);
		close(dev);
	}
}

void push(
	Queue* queue,
	EegData item
) {
	if(queue->size > MAX_QUEUE_SIZE) return;
	
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
