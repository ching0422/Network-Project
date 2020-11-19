#include "hw1.h"

struct
{
	char *ext;
	char *filetype;
} extensions[] = {
	{"gif", "image/gif"},
	{"jpg", "image/jpg"},
	{"jpeg", "image/jpeg"},
	{"png", "image/png"},
	{"zip", "image/zip"},
	{"gz", "image/gz"},
	{"tar", "image/tar"},
	{"htm", "text/html"},
	{"html", "text/html"},
	{"exe", "text/plain"},
	{0, 0}};

void load(char buffer[BUFSIZE + 1], int fd, int ret)
{
	char *tmp = strstr(buffer, "filename");
	if (tmp == 0)
		return;
	char filename[BUFSIZE + 1], data[BUFSIZE + 1], location[BUFSIZE + 1];
	memset(filename, '\0', BUFSIZE);
	memset(data, '\0', BUFSIZE);
	memset(location, '\0', BUFSIZE);

	char *a, *b;
	a = strstr(tmp, "\"");
	b = strstr(a + 1, "\"");
	strncpy(filename, a + 1, b - a - 1);
	strcat(location, "upload/");
	strcat(location, filename);

	a = strstr(tmp, "\n");
	b = strstr(a + 1, "\n");
	a = strstr(b + 1, "\n");
	b = strstr(a + 1, "---------------------------");

	int download = open(location, O_CREAT | O_WRONLY | O_TRUNC | O_SYNC, S_IRWXO | S_IRWXU | S_IRWXG);

	char t[BUFSIZE + 1];
	int last_write, last_ret;
	if (b != 0)
		write(download, a+1, b-a-3);
	else
	{
		int start = (int)(a - &buffer[0]) + 1;
		last_write = write(download, a+1, ret-start-61);
		last_ret = ret;
		memcpy(t, a + 1 + last_write, 61);

		while ((ret = read(fd, buffer, BUFSIZE)) > 0)
		{
			write(download, t, 61);
			last_write = write(download, buffer, ret - 61);
			memcpy(t, buffer + last_write, 61);
			last_ret = ret;
			if (ret != 8096)
				break;
		}
	}
	close(download);
	printf("UPLOAD FILE NAME :%s\n", filename);
	return;
}

void handle_socket(int fd)
{
	int j, file_fd, buflen, len;
	long i, ret;
	char *fstr;
	static char buffer[BUFSIZE + 1];
	static char tmp[BUFSIZE + 1];

	//讀取瀏覽器要求
	ret = read(fd, buffer, BUFSIZE);
	//讀取失敗結束行程
	if (ret == 0 || ret == -1)
	{
		exit(3);
	}

	printf("loading...\n");
	memcpy(tmp, buffer, ret);
	load(buffer, fd, ret);

	//尾巴補\0
	if (ret > 0 && ret < BUFSIZE)
		buffer[ret] = 0;
	else
		buffer[0] = 0;
	//刪\r\n
	for (i = 0; i < ret; i++)
		if (buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = 0;
	//判斷GET命令
	if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4))
		exit(3);
	//分隔HTTP/1.0 (GET /index.html HTTP/1.0)
	for (i = 4; i < BUFSIZE; i++)
	{
		if (buffer[i] == ' ')
		{
			buffer[i] = 0;
			break;
		}
	}
	//客戶端要求根目錄時讀取index.html
	if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
		strcpy(buffer, "GET /index.html\0");
	//檢查客戶端所要求的檔案格式
	buflen = strlen(buffer);
	fstr = (char *)0;

	//確認type
	for (i = 0; extensions[i].ext != 0; i++)
	{
		len = strlen(extensions[i].ext);
		if (!strncmp(&buffer[buflen - len], extensions[i].ext, len))
		{
			fstr = extensions[i].filetype;
			break;
		}
	}
	//沒有在以上的檔案格式內
	if (fstr == 0)
	{
		fstr = extensions[i - 1].filetype;
	}
	//開檔
	if ((file_fd = open(&buffer[5], O_RDONLY)) == -1)
		write(fd, "Failed to open file", 19);
	//回傳 200OK 內容格式
	sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
	write(fd, buffer, strlen(buffer));
	//讀檔，輸出到客戶端
	while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
	{
		write(fd, buffer, ret);
	}

}

int main(int argc, char **argv)
{
	int i, maxi, maxfd;
	int listenfd, socketfd, connfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char buf[BUFSIZE];
	socklen_t length;
	struct sockaddr_in cli_addr, serv_addr;

	printf("Creat Socket...\n");
	//開啟網路socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	//網路連線設定
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);		   //port8080
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //使用任何在本機的對外IP
	
	printf("Listening...\n");
	//開啟網路監聽器(TCP)
	bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	//開始監聽//64:允許client連線數目
	listen(listenfd, 64);

	//初始化
	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;
	FD_ZERO(&allset);		   //清空描述符
	FD_SET(listenfd, &allset); //新增

	while (1)
	{
		rset = allset;
		//select(n , readfds , writefds , exceptfds , timeout)
		if ((nready = select(maxfd + 1, &rset, NULL, NULL, NULL)) < 0){
			exit(4);
		}

		//等待客戶端連線
		if (FD_ISSET(listenfd, &rset))
		{
			printf("Waiting...\n");
			length = sizeof(cli_addr);
			connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);

			for (i = 0; i < FD_SETSIZE; i++)
			{
				if (client[i] < 0)
				{
					client[i] = connfd;
					break;
				}
			}
			if (i == FD_SETSIZE)
			{
				exit(3);
			}
			FD_SET(connfd, &allset);
			if (connfd > maxfd)
			{
				maxfd = connfd;
			}
			if (i > maxi)
			{
				maxi = i;
			}
			if (--nready <= 0)
			{
				continue;
			}
		}
		//對現有連線進行讀寫
		for (i = 0; i <= maxi; i++)
		{
			if ((socketfd = client[i]) < 0)
				continue;
			if (FD_ISSET(socketfd, &rset))
			{
				handle_socket(socketfd);
				close(socketfd);
				FD_CLR(socketfd, &allset);
				client[i] = -1;

				if (--nready <= 0)
				{
					break;
				}
			}
		}
	}
	printf("Closing...\n");
	return 0;
}
