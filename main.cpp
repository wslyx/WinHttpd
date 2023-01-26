#include <iostream>
#include <cstdio>
#include <WinSock2.h>

#pragma comment (lib,"WS2_32.lib")
//Output Macro
#define PRINTF(str) std::cout<<"__func__:"<<__func__<<", __code_line__:"<<__LINE__<<std::endl \
                    <<#str<<": "<<str<<std::endl;
//Error return
void error_die(const char* str)
{
    perror(str);
    exit(1);
}

SOCKET startup(unsigned short *port) {
    WSAData wsaData{};
    int ret = WSAStartup(
            MAKEWORD(1,1),  //The highest version of Windows Sockets specification that the caller can use
            &wsaData
    );
    if(ret) //ret!=0
    {
        error_die("WSAStartup");
    }

    SOCKET server_socket = socket(
            PF_INET,    //ipv4 or ipv6
            SOCK_STREAM,    //data stream or datagram
            IPPROTO_TCP //tcp or udp
    );
    if(server_socket == -1)
    {
        error_die("socket");
    }
    //Set reuse of port
    int opt = 1;
    ret = setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof (opt));
    if(ret == -1)
    {
        error_die("setsockopt");
    }

    struct sockaddr_in server_addr{};
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(*port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind server_add to server_socket
    int bindResult = bind(server_socket, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    if (bindResult<0)
    {
        error_die("bind");
    }
    //If the port is occupied, obtain an available port
    int nameLen = sizeof(server_addr);
    if(*port==0)
    {
        if(getsockname(server_socket, reinterpret_cast<sockaddr *>(&server_addr), &nameLen) < 0)
        {
            error_die("getsockname");
        }
        *port = server_addr.sin_port;
    }
    //listen to server_socket
    if (listen(server_socket,5)<0)
    {
        error_die("listen");
    }

    return server_socket;
}

//read a row
int get_line(int sock,char* buff,int size)
{
    char c = 0;
    int i = 0;
    // Judge the end of the line \r\n
    while (c!='\n' && i<size-1)
    {
        int n = recv(sock,&c,1,0);
        if (n>0)
        {
            if (c=='\r')
            {
                n = recv(sock,&c,1,MSG_PEEK); //check
                if (n>0 && c=='\n')
                {
                    recv(sock,&c,1,0); //read out
                } else {
                    c='\n';
                }
            }
            buff[i++]=c;
        } else {
            c='\n';
        }
    }
    return 0;
}

void unimplement(int client)
{

}

void not_found(int client)
{

}

void server_file(int client,const char* fileName)
{
    return;
}

DWORD WINAPI accept_requset(LPVOID arg)
{
    char buff[1024];
    int client_sock=(SOCKET)arg;
    int num_chars = get_line(client_sock, buff, sizeof(buff));
    PRINTF(buff);
    //Parse function name
    char method[255];
    int j = 0,i = 0;
    while(!isspace(buff[j]) && i < sizeof(method) - 1)
    {
        method[i++] = buff[j++];
    }
    method[i] = 0;  //'\0'
    PRINTF(method);
    //check if this server support the method
    if(_stricmp(method,"GET") != 0 && _stricmp(method,"POST") != 0)
    {
        unimplement(client_sock);
        return 0;
    }
    //Parse path of resource files
    char url[255];
    i=0;
    while(isspace(buff[j]) && j < sizeof(buff) - 1) ++j;
    while(!isspace(buff[j]) && i < sizeof(method) - 1)
    {
        url[i++] = buff[j++];
    }
    url[i]=0;   //'\0'
    PRINTF(url);
    char path[512]="";
    sprintf(path,"htdocs%s",url);
    if(path[strlen(path)-1]=='/')
    {
        strcat_s(path,"index.html");
    }
    PRINTF(path);

    struct stat status{};
    if(stat(path,&status)==-1)
    {
        while (num_chars > 0 && strcmp(buff,"\n") != 0)
        {
            //Remove the remaining characters in the buffer
            num_chars = get_line(client_sock, buff, sizeof(buff));
        }
        not_found(client_sock);
    } else {
        if ((status.st_mode & S_IFMT) == S_IFDIR)
        {
            strcat_s(path,"/index.html");
        }
        server_file(client_sock, path);
    }
    return 0;
}

int main() {
    unsigned short port = 8080;
    SOCKET server_sock = startup(&port);
    std::cout << "httpd start , port: "<< port << std::endl;
    struct sockaddr_in client_addr{};
    while (true)
    {
        //block,wait for a new client
        int client_addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if(client_sock == -1)
        {
            error_die("accept");
        }
        DWORD threadId = 0;
        CreateThread(0,0,accept_requset,(void*)client_sock,0,&threadId);
    }
    closesocket(server_sock);
    system("pause");
    return 0;
}
