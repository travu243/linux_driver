#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define PORT 8080 

int main(int argc, char const *argv[]) 
{ 
    int server_fd, new_socket, byte_read; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char mess_from_client[225];
    char buffer[1024] = {0}; 
    char *hello = "Hello from server";
    int continu = 1;
    //tao socket
    // tao file mo ta soket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    }  
    //gan dia chi cho socket
    // gan cong port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT );  //htons convert to big edian used by network   
    // bind ip address with socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    //dat. socket lam` server o? trang. thai' lang' nghe toi' da 3 ket' noi'
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    //khi chua co' ket' noi' thi` block
    //khi co' ket' noi' thi` accept va` tra? ve` socket moi'
    //dung` socket moi' de? gui? va` nhan. tin
    //socket da~ tao. ben tren chi? de? config khi chua co' ket' noi'
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 

	    char str_cli_ip[INET_ADDRSTRLEN];
	    struct sockaddr_in* ip_client = (struct sockaddr_in*)&address;
	    //convert ip addr from binary form to string form can read by human
	    inet_ntop(AF_INET, &ip_client->sin_addr, str_cli_ip, INET_ADDRSTRLEN);
	    printf("ipclient: %s\n", str_cli_ip );
	    char str_cli_port[INET_ADDRSTRLEN];
	    //ntohs nguoc. lai. cua? htons
	    //convert port tu` big edian dung` cho network thanh` kieu? thu' tu. xep' byte theo host
	    printf("port: %d\n", ntohs(ip_client->sin_port));
	while(continu == 1){
	    printf("Tin nhan ban nhan dc tu client: \n");
	    //read, doc du lieu gan vao bien byte_read tra ve so byte ma no doc duoc
	    byte_read = read( new_socket, buffer, 1024);
	    printf("%s\n",buffer );
		//viet hoa
	    ToUp(&buffer);
	    //gan bien hello tra ve cho client la buffer da viet hoa
	    //hello = buffer;
	    send(new_socket, buffer, strlen(hello), 0 );
	    memset(buffer,0,sizeof(buffer));

	    if(buffer=="bye") continu=0;
    }
    close(new_socket);
    return 0; 
} 
void ToUp( char *p ) 
{ 
	while( *p ) 
	{ 
		*p=toupper( *p ); 
		p++; 
	}
	return; 
} 
