#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <ez_util.h>
#include <ez_net.h>
#include <ez_log.h>

char read_char_from_stdin(void)
{
	char buf[2];
	ssize_t nread;
	int r;

	while (1) {
		r = ez_net_read(STDIN_FILENO, buf, 2, &nread);

		if (r == ANET_OK && nread > 0)
			return buf[0];
		else if (r == ANET_ERR)
			return 0;
		else
			continue;
	}
}

int wait_quit(int c)
{
	char buf[44];
    char red[16];
	char s;
	int r;
	ssize_t nwrite , nread;

	while (1) {
		s = read_char_from_stdin();
		if (s == 0)
			break;
        else if (s == 'e' || s == 'q')
			break;
        else if (s == 'w' || s == 'W') {
			r = ez_net_write(c, buf, sizeof(buf), &nwrite);
			log_info("client id:%d write %d bytes, result: %d .", c, nwrite, r);
			if (r == ANET_ERR)
				break;
		}
        else if (s == 'r' || s == 'R') {
            r = ez_net_read(c, red, sizeof(red), &nread);
            log_hexdump(LOG_INFO, red, nwrite, "client id:%d read %d bytes, result: %d.", c, nread, r);
        }
	}

	return 1;
}

int main(int argc, char **argv)
{
	int port = 9090;
	char *addr = "localhost";
    if(argc > 1) {
    	addr = argv[1];
    }

	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	log_init(LOG_VVERB, NULL);

	int c = ez_net_tcp_connect(addr, port);
	if (c > 0) {
		log_info("client %d connect server [%s:%d]", c, addr, port);
		ez_net_set_non_block(c);
		while (!wait_quit(c)) ;
		ez_net_close_socket(c);
	}

	log_release();
	return 0;
}
