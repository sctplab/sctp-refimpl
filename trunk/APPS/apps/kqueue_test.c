/*
 * Copyright (C) 2014 Randal R Stewart
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <poll.h>
#include <sys/uio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <netinet/sctp.h>
#include <netinet/tcp.h>
#include <sys/event.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_ARGS 16

typedef int f2(char **, int);

struct command {
    char *co_name;      /* Command name. */
    char *co_desc;      /* Command description. */
    f2 *co_func;        /* Function to call to execute the command. */
};

int kq=-1;
pthread_mutex_t mut;
int kqueue_stopped = 0;

/* Line editing.
 * Store the commands and the help descriptions.
 * Please keep this list sorted in alphabetical order.
 */
static int cmd_quit(char *argv[], int argc);
static int cmd_help(char *argv[], int argc);
static int cmd_kevent(char *argv[], int argc);
static int cmd_silence(char *argv[], int argc);
static int cmd_stop(char *argv[], int argc);
static int cmd_start(char *argv[], int argc);
static int cmd_socket(char *argv[], int argc);
static int cmd_socklist(char *argv[], int argc);
static int cmd_connect(char *argv[], int argc);
static int cmd_close(char *argv[], int argc);
static int cmd_send(char *argv[], int argc);
static int cmd_bind(char *argv[], int argc);
static int cmd_recv(char *argv[], int argc);
static int cmd_accept(char *argv[], int argc);
static int cmd_listen(char *argv[], int argc);
static int cmd_nb(char *argv[], int argc);


static struct command commands[] = {
    {"accept", "accept sd - accept on a socket",
     cmd_accept},
    {"akq", "akq fd type {options} - Add a kqueue event to watch for fd type=read/write",
     cmd_kevent},
    {"bind", "bind fd port - Bind a socket to a port",
     cmd_bind},

    {"close", "close fdnum - Close a previously opened socket",
     cmd_close},
    {"connect", "connect fd addr port - connect to a remote address/port",
     cmd_connect},
    {"help", "help [cmd] - display help for cmd, or for all the commands if no cmd specified",
     cmd_help},
    {"listen", "listen sd - listen on a socket",
     cmd_listen},
    {"nb", "nb sd - Toggle blocking on the socket",
     cmd_nb},
    {"quit", "quit - Exit the test",
     cmd_quit},
    { "recv", "recv sd (blen) - Recv a msg on socket sd", 
      cmd_recv},
    {"s", "s - toggle silence",
     cmd_silence},
    {"stop", "stop - Prevent the kqueue thread from getting more events after next one",
     cmd_stop},
    {"start", "start - Allow the kqueue thread to get more events",
     cmd_start},
    {"socket", "socket {type} - Open a socket and get a fd back -- type can be sctp or tcp",
     cmd_socket},
    {"socklist", "socklist - List out sockets",
     cmd_socklist},
    { "send", "send sd msg - Send a msg on socket sd (msg one string)", 
      cmd_send},
    {NULL, NULL, NULL}
};


/* Generator function for command completion, called by the readline library.
 * It returns a list of commands which match "text", one entry per invocation.
 * Each entry must be freed by the function which has been setup with
 * rl_callback_handler_install. The end of the list is marked by returning
 * NULL. When called for the first time with a new "text", "state" is set to
 * zero to allow for initialization.
 */
struct socket_reg {
	char *type;
	int fd;
	uint32_t flags;
	int port;
};

#define EXPAND_BY 32
int cnt_of_sock=0;
int cnt_alloc=0;
struct socket_reg *sock_reg=NULL;

#define SOCK_OPEN   0x00001
#define SOCK_CLOSE  0x00002
#define SOCK_LISTEN 0x00004
#define SOCK_CONN   0x00008
#define SOCK_ACCEPT 0x00010
#define SOCK_NB     0x00020
#define SOCK_BOUND  0x00040

int
register_sd(int sd, char *t)
{
	size_t mallen, cplen;
	struct socket_reg *tmp;

	if ((cnt_of_sock+1) > cnt_alloc) {
		/* Time to expand */
		mallen = sizeof(struct socket_reg) * (cnt_alloc+EXPAND_BY);
		tmp = malloc(mallen);
		if (tmp == NULL) {
			printf("Can't expand socket reg -- no memory\n");
			return(-1);
		}
		memset(tmp, 0, mallen);
		if (sock_reg) {
			cplen = sizeof(struct socket_reg) * cnt_of_sock;
			memcpy(tmp, sock_reg, cplen);
			free(sock_reg);
		}
		sock_reg = tmp;			
	}
	sock_reg[cnt_of_sock].fd = sd;
	sock_reg[cnt_of_sock].type = t;
	sock_reg[cnt_of_sock].flags = SOCK_OPEN;
	cnt_of_sock++;
	return(0);
}

struct socket_reg *
get_reg(int fd)
{
	struct socket_reg *ret=NULL;
	int i;

	for(i=0; i<cnt_of_sock; i++) {
		if (sock_reg[i].fd == fd) {
			ret = &sock_reg[i];
			break;
		}
	}
	return(ret);
}

int 
cmd_stop(char *argv[], int argc)
{
	if (kqueue_stopped) {
		printf("Kqueue is already stopped\n");
		return(0);
	}
	pthread_mutex_lock(&mut);
	kqueue_stopped = 1;
	printf("The kqueue is stopped - it will wake once more and then wait until a start\n");
}

int 
cmd_start(char *argv[], int argc)
{
	if (kqueue_stopped == 0) {
		printf("Kqueue is already started\n");		
		return(0);
	}
	kqueue_stopped = 0;
	pthread_mutex_unlock(&mut);
	printf("The kqueue thread has been released\n");
}

int 
cmd_socklist(char *argv[], int argc)
{
	int i, cnt;

	if (cnt_of_sock == 0) {
		printf("No sockets are registered\n");
		return(0);
	}	
	printf("%d sockets are registered:\n", cnt_of_sock);
	for(i=0; i<cnt_of_sock; i++) {
		printf("Sd:%d type:%s ",
		       sock_reg[i].fd,
		       sock_reg[i].type);
		if (sock_reg[i].flags & SOCK_CLOSE) {
			printf( "CLOSED\n");
			continue;
		}
		cnt = 0;
		if (sock_reg[i].flags & SOCK_OPEN) {
			printf("OPEN");
			cnt++;
		}
		if (sock_reg[i].flags & SOCK_LISTEN) {
			if (cnt) {
				printf("|");
			}
			cnt++;
			printf("LISTEN");
		}
		if (sock_reg[i].flags & SOCK_CONN) {
			if (cnt) {
				printf("|");
			}
			cnt++;
			printf("CONN");
		}
		if (sock_reg[i].flags & SOCK_ACCEPT) {
			if (cnt) {
				printf("|");
			}
			cnt++;
			printf("ACCEPT");
		}
		if (sock_reg[i].flags & SOCK_NB) {
			if (cnt) {
				printf("|");
			}
			cnt++;
			printf("NBIO");
		}
		if (sock_reg[i].flags & SOCK_BOUND) {
			if (cnt) {
				printf("|");
			}
			cnt++;
			printf("BOUND");
			printf("(%d)", sock_reg[i].port);
		}
		printf("\n");
	}
}

static char *
command_generator(char *text, int state)
{
    static int index, len;
    char *name;

    /* If this is a new word to complete, initialize now. */
    if (state == 0) {
        index = 0;
        len = strlen(text);
    }
    /* Return the next name which partially matches from the command list. */
    while ( (name = commands[index].co_name) != NULL) {
        index++;
        if (strncmp(name, text, len) == 0)
            return strdup(name);
    }
    return NULL;    /* no names matched */
}


static struct command *
find_command(char *name)
{
	int i;
	if(name == NULL)
		return(NULL);
	if(name[0] == 0)
		return(NULL);
	for (i = 0; commands[i].co_name != NULL; i++)
		if (strcmp(name, commands[i].co_name) == 0)
			return(&commands[i]);

	return NULL;
}

static int
execute_line(const char *line)
{
    struct command *command;
    char *argv[MAX_ARGS];
    char *buf, *cmd;
    int i, ret;

    if (*line == '\0')
	return -1;

    memset(argv, 0, sizeof(argv));
    if ((buf = strdup(line)) == NULL) {
	    fprintf(stderr, "No memory for strdup err:%d\n", errno);
	    return (-1);
    }
    /* 
     * Readline gives us an array in buf seperated by tabs.
     * Lets break it into pieces to fit our argv[n] strings.
     */
    cmd = strtok(buf, " \t");
    for (i=0; i<MAX_ARGS; i++) {
	    argv[i] = strtok(NULL, " \t");
	    if (argv[i] == NULL) {
		    break;
	    }
    }
    /* Since the line may have been generated by command completion
     * or by the user, we cannot be sure that it is a valid command name.
     */
    command = find_command(cmd);
    if (command == NULL) {
        printf("%s: No such command.\n", cmd);
        return -1;
    }
    ret = command->co_func(argv, i);
    free(buf);
    return ret;
}

/*
 * Called with a complete line of input by the readline library.
 */
static void
handle_stdin(char *line)
{
    if (line == NULL || *line == '\0')
        return;
    execute_line(line);
    add_history(line);
    free(line);
}

/* Initialize user interface handling.
 */
pthread_t tid;
int not_done = 1;

static char *
get_filter_name(short fil)
{
	static char buf[100];
	if (fil == EVFILT_READ) {
		return("read");
	} else if (fil == EVFILT_WRITE) {
		return("write");
	} else if (fil == EVFILT_AIO) {
		return("aio");
	} else if (fil == EVFILT_VNODE) {
		return("vnode");
	} else if (fil == EVFILT_PROC) {
		return("proc");
	} else if (fil == EVFILT_SIGNAL) {
		return("signal");
	} else if (fil == EVFILT_TIMER) {
		return("timer");
	} else if (fil == EVFILT_USER) {
		return("user");
	}
	sprintf(buf, "unknown:%d", fil);
	return(buf);
}

static char *
get_flags_name(u_short flag)
{
	char buf[512];
	char buf1[32], *ret;
	int num=0;
	int len;

	memset(buf, 0, sizeof(buf));
	if (flag & EV_ADD) {
		strcat(buf, "EV_ADD");
		num++;
	}
	if (flag & EV_ENABLE) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_ENABLE");
		num++;
	}
	if (flag & EV_DISABLE) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_DISABLE");
		num++;
	}
	if (flag & EV_DISPATCH) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_DISPATCH");
		num++;
	}
	if (flag & EV_DELETE) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_DELETE");
		num++;
	}
	if (flag & EV_RECEIPT) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_RECEIPT");
		num++;
	}
	if (flag & EV_ONESHOT) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_ONESHOT");
		num++;
	}
	if (flag & EV_CLEAR) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_CLEAR");
		num++;
	}
	if (flag & EV_ERROR) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_ERROR");
		num++;
	}
	if (flag & EV_EOF) {
		if (num)
			strcat(buf, "|");
		strcat(buf, "EV_EOF");
		num++;
	}
	sprintf(buf1, ":0x%x", flag);
	len = strlen(buf) + strlen(buf1) + 1;
	ret = malloc(len);
	if (ret == NULL) {
		printf("Help %d\n", errno);
		exit(-1);
	}
	memset(ret, 0, len);
	strcpy(ret, buf);
	strcat(ret, buf1);
	return(ret);
}

int silence=0;

void *
kqwork(void *arg)
{
	int ret;
	struct kevent ev;
	char *name, *flags;
	
	while(not_done) {
		/* Do our stopping thing */
		pthread_mutex_lock(&mut);
		pthread_mutex_unlock(&mut);
		/* Now go get the kqueue */
		ret = kevent(kq, NULL, 0, &ev, 1, NULL);
		if (ret > 0) {
			name = get_filter_name(ev.filter);
			flags = get_flags_name(ev.flags);
			if (silence == 0) {
				printf("Filter %s ident:%d flags:%s fflags:0x%x data:%p udata:%p\n",
				       name, (int)ev.ident, flags, (uint32_t)ev.fflags,
				       (void *)ev.data, ev.udata);
			}
			if (flags) {
				free(flags);
			}
		} else if (ret < 0) {
			printf("Kq bad event errno:%d -- exiting thread\n",
			       errno);
			break;
		}
	}
	return(NULL);
}



static void
init_user_int(void)
{
	/* Init the readline library, callback mode. */
	rl_completion_entry_function = (rl_compentry_func_t *) command_generator;
	rl_callback_handler_install(">>>", handle_stdin);

	/* Setup our kqueue */
	kq = kqueue();
	if (kq == -1) {
		printf("Can't initialize kq err:%d\n", errno);
		exit(-1);
	}
	if (pthread_mutex_init(&mut, NULL)) {
		printf("Can't init kqueue handler mutex err:%d\n",
		       errno);
		exit(-1);
	}

	if(pthread_create(&tid, NULL, kqwork, NULL)) {
		printf("Can't create pthread -- err:%d\n", errno);
		exit(-1);
	}
}

uint64_t count=0;

int 
cmd_kevent(char *argv[], int argc)
{
	struct kevent event;
	u_short def;
	int fd, i, ret;
	short filt;
	char *flags;

	if (argc < 2) {
	unknown:
		printf("You need at minimum two args fd and type\n");
		printf("akq fd type ..options..\n");
		printf("fd = a file descriptor\n");
		printf("type = read or write\n");
		printf("options include:\n");
		printf(" - add\n");	
		printf(" - clear\n");
		printf(" - delete\n");
		printf(" - disable\n");
		printf(" - dispatch\n");
		printf(" - enable\n");
		printf(" - oneshot\n");
		printf(" - receipt\n");
		return(0);
	}
	fd = strtol(argv[0], NULL, 0);
	if (strcmp(argv[1], "read") == 0) {
		filt = EVFILT_READ;
	} else if (strcmp(argv[1], "write") == 0) {
		filt = EVFILT_WRITE;
	} else {
		printf("Unkown filter type %s\n", argv[1]);
		goto unknown;
	}
	if (argc == 2) {
		def = EV_ADD|EV_ENABLE|EV_DISPATCH;
	} else {
		def = 0;
		for(i=2; i<argc; i++) {
			if (strcmp(argv[i], "disable") == 0) {
				def |= EV_DISABLE;
			} else if (strcmp(argv[i], "delete") == 0) {
				def |= EV_DELETE;
			} else if (strcmp(argv[i], "enable") == 0) {
				def |= EV_ENABLE;
			} else if (strcmp(argv[i], "add") == 0) {
				def |= EV_ADD;
			} else if (strcmp(argv[i], "dispatch") == 0) {
				def |= EV_DISPATCH;
			} else if (strcmp(argv[i], "receipt") == 0) {
				def |= EV_RECEIPT;
			} else if (strcmp(argv[i], "oneshot") == 0) {
				def |= EV_ONESHOT;
			} else if (strcmp(argv[i], "clear") == 0) {
				def |= EV_CLEAR;
			} else {
				printf("Unknown option %s - skipping\n", argv[i]);
			}
		}
	}
	EV_SET(&event, 
	       fd, 
	       filt, 
	       def, 
	       0, 
	       0, 
	       (void *)count);
	flags = get_flags_name(event.flags);
	printf("Set in fd:%d fil:%s flags:%s count:%d\n",
	       fd,
	       get_filter_name(event.filter),
	       flags, count);
	count++;
	if (flags) {
		free(flags);
	}
	errno = 0;
	ret = kevent(kq, &event, 1, NULL, 0, NULL);
	printf("Returns %d errno:%d\n", ret, errno);
	return(0);
}

int 
cmd_silence(char *argv[], int argc)
{
	if (silence == 0) {
		silence = 1;
	} else {
		silence = 0;
	}
	return(0);
}

int 
cmd_quit(char *argv[], int argc)
{
	not_done = 0;
}

int
cmd_help(char *argv[], int argc)
{
	int i, printed;
	char *cmdname;
	struct command *cmd;

	if (argc > 0) {
		cmdname = argv[0];
		if ( (cmd = find_command(cmdname)) != NULL) {
			printf("%s\n", cmd->co_desc);
		} else {
			printf("help: no match for `%s'. Command\n", cmdname);
			printf("Use one of:\n");
			goto dump_all;
		}
	} else {
	dump_all:
		for (i = 0; commands[i].co_name != NULL; i++) {
			printf("%s\n", commands[i].co_desc);
		}
	}
	return(0);
}

int 
cmd_socket(char *argv[], int argc)
{
	int sock_type, proto;
	int sd;
	char *tof;
	
	proto = IPPROTO_TCP;
	sock_type = SOCK_STREAM;
	tof = "tcp";
	if (argc > 0) {
		if (strcmp(argv[0], "sctp") == 0) {
			proto = IPPROTO_SCTP;
			sock_type = SOCK_SEQPACKET;
			tof = "sctp";
		} else 	if (strcmp(argv[0], "udp") == 0) {
			proto = IPPROTO_UDP;
			sock_type = SOCK_DGRAM;
			tof = "udp";
		} else if (strcmp(argv[0], "tcp") == 0) {
			proto = IPPROTO_TCP;
			sock_type = SOCK_STREAM;
		} else {
			printf("Unknown socket type not tcp or sctp or udp\n");
			return(0);
		}

	}
	sd = socket(AF_INET, sock_type, proto);
	printf("fd is %d\n", sd);
	register_sd(sd, tof);
}

int 
cmd_close(char *argv[], int argc)
{
	int sd;

	if (argc < 1) {
		printf("You must specify a fd\n");
		return(0);
	}
	sd = strtol(argv[0], NULL, 0);
	if (close(sd) == 0) {
		struct socket_reg *reg;

		reg = get_reg(sd);
		if (reg) {
			reg->flags = SOCK_CLOSE;
			reg->fd = -1;
		}
		printf("Success\n");
	} else {
		printf("Failed errno:%d\n", errno);
	}
	return(0);
}

int 
cmd_send(char *argv[], int argc)
{
	struct socket_reg *reg;
	int sd, ret, len, tlen, i;
	char buffer[1024];

	if (argc < 2) {
		printf("recv sd -- need a sd and msg\n");
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	if ((reg->flags & (SOCK_CONN|SOCK_ACCEPT)) == 0) {
		printf("You can't send on a non-connected socket\n");
		return(0);
	}
	memset(buffer, 0, sizeof(buffer));
	tlen = 0;
	for(i=1; i<argc; i++) {
		len = strlen(argv[i]);
		if ((len + tlen) > 1024)
			break;
		strcat(buffer, argv[i]);
		tlen += len;
	}
	errno = 0;
	ret = send(sd, buffer, tlen, 0);
	if (ret > 0) {
		printf("Sent %d bytes on sd:%d\n", ret, sd);
	} else {
		printf("Send Ret %d errno:%d tlen:%d\n", ret, errno, tlen);
	}
	return(0);
}

int 
cmd_recv(char *argv[], int argc)
{
	struct socket_reg *reg;
	int sd, ret;
	char buffer[1024];
	size_t blen;

	if (argc < 1) {
		printf("recv sd -- need a sd specified\n");
	}
	if (argc > 1) {
		blen = strtol(argv[1], NULL, 0);
		if (blen > sizeof(buffer)) {
			blen = sizeof(buffer);
		}
	} else {
		blen - sizeof(buffer);
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	if ((reg->flags & (SOCK_CONN|SOCK_ACCEPT)) == 0) {
		printf("You can't recv on a non-connected socket\n");
		return(0);
	}
	memset(buffer, 0, sizeof(buffer));
	errno = 0;
	ret = recv(sd, buffer, blen, 0);
	if (ret > 0) {
		printf("Recv %d bytes\n", ret);
		if (ret > (sizeof(buffer)-2)) {
			buffer[(sizeof(buffer)-2)] = '\n';
			buffer[(sizeof(buffer)-1)] = 0;
		} else {
			buffer[ret] = '\n';
		}
		printf("%s", buffer);
	} else {
		printf("Recv returns %d errno:%d\n", ret, errno);
	}
	return(0);
}

int 
cmd_connect(char *argv[], int argc)
{
	struct socket_reg *reg;
	struct sockaddr_in sin;
	socklen_t slen;
	uint16_t port;
	int sd, ret;

	if (argc < 3) {
		printf("connect sd addr port\n");
		return(0);
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	memset(&sin, 0, sizeof(sin));
	port = (uint16_t)strtol(argv[2], NULL, 0);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	slen = sizeof(struct sockaddr_in);
	sin.sin_len = slen;
	ret = inet_pton(AF_INET, argv[1], &sin.sin_addr);
	if (ret != 1) {
		printf("Sorry can't parse address %s\n", argv[1]);
		return(0);
	}
	printf("Connecting to %s:%d on sd:%d\n", argv[1], port, sd);

	ret = connect(sd, (struct sockaddr *)&sin, slen);
	if (ret) {
		printf("Connect fails ret:%d err:%d\n", ret, errno);
	} else {
		reg->flags |= SOCK_CONN;
		printf("Success\n");
	}
}

int 
cmd_accept(char *argv[], int argc)
{
	struct socket_reg *reg;
	int sd, newsd, ret, backlog=10;
	struct sockaddr_in sin;
	socklen_t slen;
	char from[64];

	if (argc < 1) {
		printf("Need to specify a sd to accept on\n");
		return(0);
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	slen = sizeof(struct sockaddr_in);
	newsd = accept(sd, (struct sockaddr *)&sin, &slen);
	if (newsd == -1) {
		printf("accept error %d\n", errno);
		return(0);
	}
	inet_ntop(sin.sin_family, &sin.sin_addr, from, sizeof(from));
	printf("Accept gets a new connection to sd:%d from %s:%d\n",
	       newsd, from, ntohs(sin.sin_port));
	register_sd(newsd, reg->type);
	reg = get_reg(newsd);
	if(reg == NULL) {
		printf("Can't find registry?\n");
		return(0);
	}
	reg->flags |= SOCK_ACCEPT;
	return(0);
}

int 
cmd_listen(char *argv[], int argc)
{
	struct socket_reg *reg;
	int sd, ret, backlog=10;

	if (argc < 1) {
		printf("Need to specify a sd to listen on\n");
		return(0);
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	if (listen(sd, backlog)) {
		printf("Listen on sd:%d failed err:%d\n", sd, errno);
	} else {
		reg->flags |= SOCK_LISTEN;
		printf("Success\n");
	}
	return(0);
}

int 
cmd_nb(char *argv[], int argc)
{
	struct socket_reg *reg;
	int nbio, sd;
	
	if (argc < 1) {
		printf("Need to specify a sd to toggle nbio on\n");
		return(0);
	}
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	if (reg->flags & SOCK_NB) {
		printf("set nbio to 0\n");
		nbio = 0;
	} else {
		printf("set nbio to 1\n");
		nbio = 1;
	}
	if (ioctl(sd, FIONBIO, (caddr_t)&nbio)) {
		printf("Failed to toggle nb to:%d err:%d\n", nbio, errno);
	} else {
		if (nbio) {
			printf("add flag SOCK_NB\n");
			reg->flags |= SOCK_NB;
		} else {
			printf("remove flag SOCK_NB\n");
			reg->flags &= ~SOCK_NB;
		}
		printf("Success\n");
	}
	return(0);
}

int 
cmd_bind(char *argv[], int argc)
{
	struct socket_reg *reg;
	int sd, ret;
	uint16_t port;
	struct sockaddr_in sin, baddr;
	socklen_t slen;

	if (argc < 2) {
		printf("Need to specify a sd and port\n");
		return(0);
	}	
	sd = strtol(argv[0], NULL, 0);
	reg = get_reg(sd);
	if (reg == NULL) {
		printf("Can't find sd:%d in registry\n", sd);
		return(0);
	}
	port = (uint16_t)strtol(argv[1], NULL, 0);
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(sin);
	sin.sin_port = ntohs(port);
	printf("Binding port %d\n", port);
	slen = sizeof(sin);
	errno = 0;
	ret = bind(sd, (struct sockaddr *)&sin, slen);
	if (ret) {
		printf("Bind fails ret:%d errno:%d\n", ret, errno);
		return(0);
	}
	errno = 0;
	slen = sizeof(baddr);
	if (getsockname(sd, (struct sockaddr  *)&baddr, &slen) == 0) {
		reg->port = port;
		reg->flags |= SOCK_BOUND;
		if (ntohs(baddr.sin_port) != port) {
			reg->port = ntohs(baddr.sin_port);
			printf("Success but new port %d\n", reg->port);
		} else {
			printf("Success\n");
		}
	} else {
		printf("getsockname failes err:%d\n", errno);
	}

}


int 
main(int argc, char **argv) 
{
	int ret;
	struct pollfd fds[2];

	init_user_int();
	
	memset(fds, 0, sizeof(fds));
	while(not_done) {
		fds[0].fd = 0; /* Stdin */
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		ret = poll(fds, 1, INFTIM);
		if (ret)  {
			rl_callback_read_char();
		}
	}
	return(0);
}

