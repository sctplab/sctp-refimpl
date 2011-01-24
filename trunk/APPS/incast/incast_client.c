#include <incast_fmt.h>

struct incast_control ctrl;

void
incast_add_peer(char *body, int linecnt)
{
	struct incast_peer *peer;
	char *ipaddr, *portstr=NULL, *tok;
	int len;

	if (body == NULL) {
		printf("adding peer - error line:%d no body\n",
		       linecnt);
		return;
	}
	ipaddr = body;
	tok = strtok(body, ":") ;
	if (tok != NULL) {
		/* We have a port string */
		len = strlen(tok) + 1;
		portstr = &tok[len];
	}
	peer = malloc(sizeof(struct incast_peer));
	memset(peer, 0, sizeof(struct incast_peer));
	
	if (translate_ip_address(ipaddr, &peer->addr)) {
		printf("line:%d unrecognizable peer address '%s'\n",
		       linecnt, ipaddr);
		free(peer);
		return;
	}
	if(portstr == NULL) {
		peer->addr.sin_port = htons(DEFAULT_SVR_PORT);
	} else {
		int x;
		x = strtol(portstr, NULL, 0);
		if ((x < 1) || (x > 65535)) {
			if (x) {
				printf("Invalid port number %d - using default\n",
				       x);
			}
			peer->addr.sin_port = htons(DEFAULT_SVR_PORT); 
		} else {
			peer->addr.sin_port = htons(x);
		}
	}
	peer->state = SRV_STATE_NEW;
	peer->sd = -1;
	LIST_INSERT_HEAD(&ctrl.master_list, peer, next);
	ctrl.number_server++;
}

void
incast_set_bind(char *body, int linecnt)
{
	if (body == NULL) {
		printf("setting bind - error line:%d no body\n",
		       linecnt);
		return;
	}
	if (translate_ip_address(body, &ctrl.bind_addr)) {
		printf("line:%d unrecognizable bind address '%s'\n",
		       linecnt, body);
		return;
	}
}

void
parse_config_file(char *configfile)
{
	char buffer[1025];
	FILE *io;
	char *token, *body;
	int linecnt, len, olen;
	/* 
	 * Here we parse the configuration
	 * file. An entry in the file has
	 * the form:
	 * keyword:information
	 *
	 * Allowable keywords and from are:
	 * sctp:  - switch sctp on tcp off
	 * tcp:   - switch tcp on sctp off - defaults to TCP on.
	 * peer:ip.dot.addr.or.name:port   - create a peer entry for this guy
	 * bind:ip.dot.addr.or.name        - specify the bind to address
	 * times:number - Number of passes, if 0 never stop. (default 1)
	 * sends:number - Number of bytes per send (1-9000 - default 1448)
	 * sendc:number - Total number of sends (1-N) - default 3.
	 *
	 * When the format is an address leaving off the port or setting
	 * it to 0 gains the default address.
	 */
	io = fopen(configfile, "r");
	if (io == NULL) {
		printf("Can't open config file '%s':%d\n", configfile, errno);
		return;
	}
	linecnt = 1;
	memset(buffer, 0, sizeof(buffer));
	while (fgets(buffer, (sizeof(buffer)-1), io) != NULL) {
		/* Get rid of cr */
		olen = strlen(buffer);
		if (olen == 1) {
			linecnt++;
			continue;
		}
		if (buffer[olen-1] == '\n') {
			buffer[olen-1] = 0;
			olen--;
		}
		if (buffer[0] == '#') {
			/* commented out */
			linecnt++;
			continue;
		}
		/* tokenize the token and body */
		token = strtok(buffer, ":");
		if (token == NULL) {
			linecnt++;
			continue;
		}
		len = strlen(token);
		if ((len+1) == olen) {
			/* No body */
			body = NULL;
		} else {
			/* past the null */
			body = &token[len+1];
		}
		if (strcmp(token, "sctp") == 0) {
			ctrl.sctp_on = 1;
		} else if (strcmp(token, "tcp") == 0) {
			ctrl.sctp_on = 0;
		} else if (strcmp(token, "peer") == 0) {
			incast_add_peer(body, linecnt);
		} else if (strcmp(token, "bind") == 0) {
			incast_set_bind(body, linecnt);
		} else if (strcmp(token, "times") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d times with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if (cnt) {
				ctrl.cnt_of_times = cnt;
			} else {
				ctrl.cnt_of_times = 1;
				ctrl.decrement_amm = 0;
			}
		} else if (strcmp(token, "sends") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d sends with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if ((cnt) && ((cnt > 0) && (cnt < MAX_SINGLE_MSG))) {
				ctrl.size = cnt;
			} else {
				printf("Warning line:%d sizes invalid ( 0 > %d < %d)\n",
				       linecnt, cnt, MAX_SINGLE_MSG);
				printf("Using default %d\n", DEFAULT_MSG_SIZE);
				ctrl.size = DEFAULT_MSG_SIZE;
			}

		} else if (strcmp(token, "sendc") == 0) {
			int cnt;
			if (body == NULL) {
				printf("Parse error line:%d sendc with no body\n",
					linecnt);
				linecnt++;
				continue;
			}
			cnt = strtol(body, NULL, 0);
			if ((cnt) && (cnt > 0)) {
				ctrl.cnt_req = cnt;
			} else {
				printf("Warning line:%d sizes invalid ( 0 > %d)\n",
				       linecnt, cnt);
				printf("Using default %d\n", DEFAULT_NUMBER_SENDS);
				ctrl.cnt_req = DEFAULT_NUMBER_SENDS;
			}
		} else {
			printf("Unknown token '%s' at line %d\n",
			       token, linecnt);
		}
		linecnt++;
	}
	fclose(io);
}


int
main(int argc, char **argv)
{
	int i;
	char *configfile=NULL;
	
	while ((i = getopt(argc, argv, "c:?")) != EOF) {
		switch (i) {
		case 'c':
			configfile = optarg;
			break;
		default:
		case '?':
		use:
			printf("Use %s -c config-file\n", argv[0]);
			return (-1);
			break;
		};
			       
	}
	if (configfile == NULL) {
		goto use;
	}
	/* Setup our list and init things */
	memset(&ctrl, 0, sizeof(ctrl));
	LIST_INIT(&ctrl.master_list);
	ctrl.decrement_amm = 1;
	ctrl.cnt_of_times = 1;
	ctrl.size = DEFAULT_MSG_SIZE;
	ctrl.cnt_req = DEFAULT_NUMBER_SENDS;
	/* Now parse the configuration file */
	parse_config_file(configfile);
	if (ctrl.bind_addr.sin_family != AF_INET) {
		printf("Fatal error bind address not set by config file\n");
		return (-1);
	}
	if (ctrl.number_server == 0) {
		printf("Fatal error number of servers still 0\n");
		return (-1);
	}
	incast_run_clients(&ctrl);
	return (0);
}
