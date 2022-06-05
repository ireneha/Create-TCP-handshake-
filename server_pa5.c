
//create a TCP server and tcp header struct
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define BUFLEN 4096

void die (char *s){
	perror(s);
	exit(1);
}
//calculate checksum
unsigned short int cksum (unsigned short int arr[], int bytes){
	unsigned int sum = 0;
	for (unsigned int i =0; i < bytes; i++){
	//	printf("Element %3d: 0x%04X\n",i,arr[i]);
		sum+= arr[i];
	}
	unsigned int wrap = sum >>16;	
	sum = sum & 0x0000ffff;
	sum += wrap;

	wrap = sum >> 16;	// wrap around one more time
	sum = sum & 0x0000ffff;
	unsigned int result = wrap + sum;
	return ~result;
}

//3.struct of tcp
struct TCPheader{
	unsigned short int source;
	unsigned short int dest;
	unsigned int seq_num;
	unsigned int ack_num;
	
	//unsigned short int flag
	unsigned short int
		offset: 4,
		reserved: 6,
		urg: 1,
		ack: 1,
		psh: 1,
		rst: 1,
	        syn: 1,
       		fin: 1;
	unsigned short int win;
	unsigned short int checksum;
	unsigned short int urg_ptr;
	char payload[512];	
};

// print struct to the console
void print(struct TCPheader header){
        printf("Source port:%d; Destination port: %d\nSequnce number: %d; Acknowledge number: %d\n", header.source, header.dest, header.seq_num, header.ack_num);
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
                        "Header flag: 0x%04x\nChecksum: 0x%04x\n\n",
                        header.source,header.dest,
                        header.seq_num,header.ack_num,
                        header.offset,header.checksum);
}

//send segment struct
void sendSeg(struct TCPheader s1, int fd){
	if(write(fd,(struct TCPheader*) &s1, sizeof(s1)) <0){
		die("write()");
	}
}

unsigned short int assignCS(struct TCPheader s1, unsigned short int arr[266]){
	bzero(arr, 266);
        memcpy(arr, &s1, sizeof(s1));
	return cksum(arr, 266);
}

int main(void){
	struct sockaddr_in sock_struct;
	int listenfd, acceptfd, port;
	char buf[BUFLEN];
	int sz_sock = sizeof(sock_struct);
	unsigned short int chksum_arr[266];
	FILE* f1;


	printf("PORT:");
	scanf("%d",&port);

	//listen socket from client
	if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) <0){
		die("socket");
	}
	sock_struct.sin_family = AF_INET;
	sock_struct.sin_port = htons(port);
	sock_struct.sin_addr.s_addr = htons(INADDR_ANY);
	
	//bind the above details to the socket
	bind(listenfd,(struct sockaddr *) &sock_struct, sz_sock);
	
	//get source port for header
//	socklen_t len = sizeof(sock_struct);
//	getsockname(listenfd, (struct sockaddr*) &sock_struct, &len);
//	int source = ntohs(sock_struct.sin_port);
	//listen to incoming connection
	listen(listenfd, 10);
	if ((acceptfd = accept(listenfd, (struct sockaddr*) &sock_struct, (socklen_t*) &sz_sock)) <0){
		die("accept");
	}

	struct TCPheader recv,send;	//create a struct to read struct from client, and write struct to client
	//read struct from client
	if (read(acceptfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
		die("read()");
	}
	//get checksum, compute and compare
	int cl_cksum = recv.checksum;
	recv.checksum = 0;
	recv.checksum = assignCS(recv,chksum_arr);
	if ((recv.checksum - cl_cksum) != 0){
		printf("0x%04x\n", recv.checksum);
		die("Checksum error\n");
	}
	printf("Ready for check flags\n");
	//check the flags
	if (recv.syn < 0){
		die("syn < 0");
	}
////////create connection//////////////////
	//6b. first send seg to client
	printf("\n##PRINT RECV 1##\n");
	print(recv);
	f1 =fopen("server.out","w+");
	writeFile(recv,f1);
	send.source = recv.source;
	send.dest = port;
	send.offset = 5;
	send.reserved = 0;
	send.urg = 0;
	send.ack = 1;
	send.psh = 0;
	send.rst = 0;
	send.fin = 0;
	send.syn = 1;
	send.seq_num = rand();
	send.ack_num = recv.ack_num + 1;
	send.win = 0;
	send.urg_ptr = 0;
	send.checksum = 0;
	bzero(send.payload,512);

		/*bzero(chksum_arr, 266);
		memcpy(chksum_arr, &send, 532);
		send.checksum = cksum(chksum_arr, 266);*/
	send.checksum = assignCS(send,chksum_arr);
			
		// send struct to client
	sleep(1);
		/*if(write(listenfd, (struct TCPheader*) &send, sizeof(send)) <0){
				die("write struct");
		}*/
	sendSeg(send, acceptfd);
	printf("\n##PRINT SEND 1##\n");
	print(send);
	writeFile(send,f1);
	
	//6d. second send seg to client
	memset(&recv,0, sizeof(recv));
	if (read(acceptfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
                die("read()");
        }
	cl_cksum = recv.checksum;
        printf("0x%04x\n", cl_cksum);
	recv.checksum = 0;
	recv.checksum = assignCS(recv,chksum_arr);
	 if ((recv.checksum - cl_cksum) != 0){
                printf("0x%04x\n", recv.checksum);
                die("Checksum error\n");
        }
        printf("Checksum matched!Ready for check flags\n");
	if (recv.ack < 0){
		die("ack < 0\n");
	}
	printf("\n##PRINT RECV 2##\n");
	print(recv);
	writeFile(recv,f1);
////////close connection//////////////
	//7b.
	memset(&recv,0, sizeof(recv));
        if (read(acceptfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
                die("read()");
        }
        cl_cksum = recv.checksum;
        recv.checksum = 0;
        recv.checksum = assignCS(recv,chksum_arr);
         if ((recv.checksum - cl_cksum) != 0){
                printf("0x%04x != 0x%04x\n",cl_cksum, recv.checksum);
                die("Checksum error\n");
        }
        printf("Checksum matched!Ready for check flags\n");
        if (recv.fin < 0){
                die("fin < 0\n");
        }
        printf("\n##PRINT RECV 3(closing)##\n");
        print(recv);
	writeFile(recv,f1);
	send.seq_num = 1024;
	send.ack_num = recv.seq_num +1;
	send.ack = 1;
	send.syn = 0;
	send.checksum =0;
	send.checksum = assignCS(send,chksum_arr);
	sleep(1);
	sendSeg(send,acceptfd);

	printf("\n##PRINT SEND 2(closing)##\n");
	print(send);
	writeFile(send,f1);
	
	//7d. send to client again
        send.ack_num += 1;
        send.ack = 0;
	send.fin = 1;
        send.checksum =0;
        send.checksum = assignCS(send,chksum_arr);
        sleep(1);
        sendSeg(send,acceptfd);

        printf("\n##PRINT SEND 3(closing)##\n");
        print(send);
        writeFile(send,f1);
	
	//7f.
	memset(&recv,0, sizeof(recv));
        if (read(acceptfd, (struct TCPheader*) &recv, sizeof(struct TCPheader)) < 0){
                die("read()");
        }
        cl_cksum = recv.checksum;
        recv.checksum = 0;
        recv.checksum = assignCS(recv,chksum_arr);
         if ((recv.checksum - cl_cksum) != 0){
                printf("0x%04x != 0x%04x\n",cl_cksum, recv.checksum);
                die("Checksum error\n");
        }
        printf("Checksum matched!Ready for check flags\n");
        if (recv.fin < 0){
                die("fin < 0\n");
        }
        printf("\n##PRINT RECV 4(closing)##\n");
	print(recv);
        writeFile(recv,f1);
	printf("CONNECTION CLOSED\n");
	fprintf(f1,"\nCONNECTION CLOSED\n");
	

	fclose(f1);
	close(listenfd);
	return 0;
}	
