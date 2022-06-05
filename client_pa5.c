
//create a TCP client and tcp header struct
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER "129.120.160.00"	// change to suitable IP address
#define BUFLEN 4096

void die (char *s){
	perror(s);
	exit(1);
}
//calculate checksum function
unsigned short int cksum(unsigned short int arr[], int bytes){
	unsigned int sum = 0;
	for (unsigned int i = 0; i < bytes; i++){
//		printf("Element %3d: 0x%04x\n",i,arr[i]);
		sum += arr[i];
	}
	unsigned int wrap = sum >> 16;	//get the carry
	sum = sum & 0x0000ffff;		//clear the carry bits
	sum += wrap;			//add carry to the sum

	wrap = sum >>16;		// wrap around once more as the previous sum could have generated a carry
	sum = sum & 0x0000ffff;
	unsigned int result = wrap + sum;
	return ~result;
}


//3.struct of tcp header
struct TCPheader{
	unsigned short int source;
	unsigned short int dest;
	unsigned int seq_num;
	unsigned int ack_num;
	
	//unsigned short int flag
	unsigned short int
		offset:4,
		reserved:6,
		urg:1,
		ack:1,
		psh:1,
		rst:1,
		syn:1,
		fin:1;
	unsigned short int win;
	unsigned short int checksum;
	unsigned short int urg_ptr;
	char payload[512];
		
};

// print struct to the console
void print(struct TCPheader header){
        printf("Source port:%d; Destination port: %d\nSequence number: %d; Acknowledge number: %d\n", header.source, header.dest, header.seq_num, header.ack_num);
        printf("Header flag: 0x%04x\nChecksum: 0x%04x\n", header.offset, header.checksum);
}
// write struct to file
void writeFile(struct TCPheader header, FILE* f1){
	if (f1 == NULL){
		printf("Could not open file");
		return;
	}
	fprintf(f1, "Source port: %d; Destination port: %d\n"
			"Sequence number %d; Acknowledge number: %d\n"
			"Header flag: 0x%04x\nChecksum: 0x%04x\n", 
			header.source,header.dest,
			header.seq_num,header.ack_num,
			header.offset,header.checksum);
}

//assign checksum to struct
unsigned short int assignCS(struct TCPheader s1, unsigned short int arr[266]){
	bzero(arr, 266);
        memcpy(arr, &s1, 532);
       	return cksum(arr, 266);
}

//send segment struct
void sendSeg(struct TCPheader s1, int fd){
        if(write(fd,(struct TCPheader*) &s1, sizeof(s1)) <0){
                die("write()");
        }
}

//recv segment struct
void recvSeg(struct TCPheader s1, int fd){
        if (read(fd,(struct TCPheader*) &s1, sizeof(s1)) <0){
                die("read()");
        }
}

int main (void){
	struct sockaddr_in sock_struct;
	int sockfd, port,temp_ck;
	char buf[BUFLEN];
	unsigned short int chksum_arr[266];
	FILE* f1;


	printf("PORT:");	//get port
	scanf("%d",&port);
	
	//create socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
		die("socket");
	}
	
	sock_struct.sin_family = AF_INET;
	sock_struct.sin_port = htons(port);

	//convert ip address from text to bin
	inet_pton (AF_INET, SERVER, &(sock_struct.sin_addr));

	//connect to server
	if (connect(sockfd, (struct sockaddr *) &sock_struct, sizeof(sock_struct)) < 0){
		die ("connect");
	}
	/*4,5.CREATE TCP HEADER STRUCT AND POPULATE STRUCT*/
	
	struct TCPheader header1;
	socklen_t len = sizeof(sock_struct);
	getsockname(sockfd, (struct sockaddr*) &sock_struct, &len);
	header1.source = ntohs(sock_struct.sin_port);
	header1.dest = port;
	header1.offset = 5;
	header1.reserved = 0;
	header1.urg = 0;
	header1.ack = 0;
	header1.psh = 0;
	header1.rst = 0;
	header1.fin = 0;
	header1.seq_num = rand();	//random sequence
	header1.ack_num = 0;		//0 acknowledge num
	header1.syn = 1;
	header1.win = 0;
	header1.urg_ptr = 0;
	header1.checksum = 0;
	bzero(header1.payload,512);	//set payload null
	
	memcpy(chksum_arr, &header1, 532);
	header1.checksum = cksum(chksum_arr,266);
	
/////////create connection////////////////////
	//6a. send to server

	if (write(sockfd, (struct TCPheadrer*) &header1, sizeof(header1)) < 0){
		die("write struct 1");
	}
	printf("\n##PRINT SEND 1##\n");
	print(header1);
	f1 = fopen("client.out","w+");
	writeFile(header1, f1);
		
	//6c. receive segment from server
	struct TCPheader recv;
	if(read(sockfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
		die("read()");
	}	
	temp_ck = recv.checksum;
	recv.checksum = 0;
	//memcpy(cksum_arr, &recv, sizeof(recv));
	//recv.checksum = cksum(chksum_arr, 266);
	recv.checksum = assignCS(recv,chksum_arr);
	if ((recv.checksum - temp_ck) != 0){
		printf("0x%04x != 0x%04x\n", temp_ck, recv.checksum);
		die("Checksum not matched!\n");
	}	
	//if checksum matched, check flags
	printf("\n##PRINT RECV 1##\n");
	print(recv);
	writeFile(recv,f1);
	if(recv.syn < 0 && recv.ack < 0){
		die("syn and ack < 0");	
	}
	//send segment to server by header1
	header1.ack_num = recv.seq_num + 1;
	header1.seq_num = recv.ack_num;
	header1.ack = 1;
	header1.checksum = 0;
	header1.checksum = assignCS(header1, chksum_arr);
	printf("\n##PRINT SEND 2##\n");
	print(header1);
	writeFile(header1,f1);
	sleep(1);
	sendSeg(header1, sockfd);

//////////closing connection/////////////////
	//7a. send to server
	header1.seq_num = 2046;
	header1.ack_num = 1024;
	header1.fin = 0;
	header1.ack = 0;
	header1.syn = 0;
	header1.checksum = 0;
	header1.checksum = assignCS(header1,chksum_arr);
	printf("\n##PRINT SEND 3(closing)##\n");
	print(header1);
	sendSeg(header1,sockfd);
	writeFile(header1,f1);

	//7c. receive from server
	memset(&recv, 0, sizeof(recv));
	if(read(sockfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
                die("read()");
        }
        temp_ck = recv.checksum;
        recv.checksum = 0;
	recv.checksum = assignCS(recv,chksum_arr);
        if ((recv.checksum - temp_ck) != 0){
                printf("0x%04x != 0x%04x\n", temp_ck, recv.checksum);
                die("Checksum not matched!\n");
        }
        //if checksum matched, check flags
        printf("\n##PRINT RECV 2 (closing)##\n");
        print(recv);
        writeFile(recv,f1);
        if(recv.ack < 0){
                die("ack < 0");
        }
	//7e.
	memset(&recv, 0, sizeof(recv));
        if(read(sockfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
                die("read()");
        }
        temp_ck = recv.checksum;
        recv.checksum = 0;
        recv.checksum = assignCS(recv,chksum_arr);
        if ((recv.checksum - temp_ck) != 0){
                printf("0x%04x != 0x%04x\n", temp_ck, recv.checksum);
                die("Checksum not matched!\n");
        }
        //if checksum matched, check flags
        printf("\n##PRINT RECV 3 (closing)##\n");
        print(recv);
        writeFile(recv,f1);
        if(recv.fin < 0){
                die("fin < 0");
        }
	header1.seq_num +=1;
	header1.ack_num = recv.seq_num +1;
	header1.ack = 1;
	header1.checksum = 0;
        header1.checksum = assignCS(header1,chksum_arr);
        printf("\n##PRINT SEND 4(closing)##\n");
        print(header1);
        sendSeg(header1,sockfd);
        writeFile(header1,f1);

	sleep(2);
	fclose(f1);
	close(sockfd);
	return 0;

}
