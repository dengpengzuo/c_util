#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include "ez_macro.h"
#include "ez_util.h"
#include "ez_log.h"

#define LOG_MAX_LEN 256		/* max length of log message */

struct logger {
	char *name;		/* log file name */
	int fd;			/* log file descriptor */
	LOG_LEVEL level;	/* log level */
	int nerror;		/* # log error */
};
struct logLevelInfo {
	LOG_LEVEL level;
	const char *name;
};
static struct logger logger;

static struct logLevelInfo LOG_LEVEL_INFO[] = {
	{.name = "EMERG",.level = LOG_EMERG},
	{.name = "ALERT",.level = LOG_ALERT},
	{.name = "CRIT",.level = LOG_CRIT},
	{.name = "ERR",.level = LOG_ERR},
	{.name = "WARN",.level = LOG_WARN},
	{.name = "NOTICE",.level = LOG_NOTICE},
	{.name = "INFO",.level = LOG_INFO},
	{.name = "DEBUG",.level = LOG_DEBUG},
	{.name = "VERB",.level = LOG_VERB},
	{.name = "VVERB",.level = LOG_VVERB},
	{.name = "VVVERB",.level = LOG_VVVERB},
	{.name = "PVERB",.level = LOG_PVERB},
	{.name = NULL}
};

#define get_log_level_name(level)  ((level < LOG_EMERG || level > LOG_PVERB) ? "-" : LOG_LEVEL_INFO[level].name)

int log_init(LOG_LEVEL level, char *name)
{
	struct logger *l = &logger;

	l->level = level;
	l->name = name;
	if (name == NULL || !strlen(name)) {
		l->fd = STDERR_FILENO;
	} else {
		l->fd = open(name, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (l->fd < 0) {
			log_stderr("opening log file '%s' failed: %s", name, strerror(errno));
			return -1;
		}
	}

	return 0;
}

void log_release(void)
{
	struct logger *l = &logger;

	if (l->fd != STDERR_FILENO) {
		close(l->fd);
		l->fd = -1;
	}
}

void log_reopen(void)
{
	struct logger *l = &logger;

	if (l->fd != -1 && l->fd > STDERR_FILENO) {
		close(l->fd);
	}
	if (l->name == NULL || !strlen(l->name)) {
		l->fd = STDERR_FILENO;
	} else {
		l->fd = open(l->name, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (l->fd < 0) {
			log_stderr("reopening log file '%s' failed, ignored: %s", l->name,
				   strerror(errno));
		}
	}
}

void log_level_up(void)
{
	struct logger *l = &logger;

	if (l->level < LOG_PVERB) {
		l->level++;
		log_stderr("up log level to %s", get_log_level_name(l->level));
	}
}

void log_level_down(void)
{
	struct logger *l = &logger;

	if (l->level > LOG_EMERG) {
		l->level--;
		log_stderr("down log level to %s", get_log_level_name(l->level));
	}
}

void log_level_set(LOG_LEVEL level)
{
	struct logger *l = &logger;

	l->level = level;
	log_stderr("set log level to %s", get_log_level_name(l->level));
}

int log_loggable(LOG_LEVEL level)
{
	struct logger *l = &logger;

	if (level > l->level) {
		return 0;
	}

	return 1;
}

void _log(LOG_LEVEL log_level, const char *file, int line, int panic, const char *fmt, ...)
{
	struct logger *l = &logger;
	size_t len, size;
	ssize_t n;
	int save_error_no;
	char buf[LOG_MAX_LEN];
	struct timeval tv;
	time_t cur_sec;
	struct tm lt;

	va_list args;

	if (l->fd < 0) {
		return;
	}

	save_error_no = errno;
	len = 0;		    /* length of output buffer */
	size = LOG_MAX_LEN;	/* size of output buffer */

	gettimeofday(&tv, NULL);
	cur_sec = tv.tv_sec;
	ez_localtime_r(&cur_sec, &lt);

    /* fmt: yyyy-MM-dd HH:mm:ss,SSS
     * len += strftime(buf + len, size - len, "%Y-%m-%d %H:%M:%S", &lt);
     * */
	len +=  ez_snprintf(buf + len, size - len,
						"%04d-%02d-%02d %02d:%02d:%02d,%03d [%6s] %s:%d ",
						lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday,
						lt.tm_hour, lt.tm_min, lt.tm_sec, (int) (tv.tv_usec / 1000),
						get_log_level_name(log_level), file, line);

	va_start(args, fmt);
	len += ez_vsnprintf(buf + len, size - len, fmt, args);
	va_end(args);

	buf[len++] = '\n';

	n = write(l->fd, buf, len);
	if (n < 0) {
		l->nerror++;
	}

	errno = save_error_no;

	if (panic) {
		abort();
	}
}

void _log_stderr(const char *file, int line, const char *fmt, ...)
{
	size_t len, size;
	char buf[4 * LOG_MAX_LEN];
	va_list args;

	len = 0;		/* length of output buffer */
	size = 4 * LOG_MAX_LEN;	/* size of output buffer */

	len += ez_snprintf(buf + len, size - len, "%s:%d ", file, line);

	va_start(args, fmt);
	len += ez_vsnprintf(buf + len, size - len, fmt, args);
	va_end(args);

	buf[len++] = '\n';

	write(STDERR_FILENO, buf, len);
}

/*
 * Hexadecimal dump in the canonical hex + ascii display
 * See -C option in man hexdump
 */
void _log_hexdump(char *data, int datalen)
{
	struct logger *l = &logger;
	char buf[8 * LOG_MAX_LEN];
	size_t i, off, len, size;
	int save_error_no;
	ssize_t n;

	if (l->fd < 0) {
		return;
	}

	/* log hexdump */
	save_error_no = errno;
	off = 0;		/* data offset */
	len = 0;		/* length of output buffer */
	size = 8 * LOG_MAX_LEN;	/* size of output buffer */

	while (datalen != 0 && (len < size - 1)) {
		char *save, *str;
		unsigned char c;
		int savelen;

		len += ez_snprintf(buf + len, size - len, "%08x  ", off);

		save = data;
		savelen = datalen;

		for (i = 0; datalen != 0 && i < 16; data++, datalen--, i++) {
			c = (unsigned char)(*data);
			str = (i == 7) ? "  " : " ";
			len += ez_snprintf(buf + len, size - len, "%02x%s", c, str);
		}
		for (; i < 16; i++) {
			str = (i == 7) ? "  " : " ";
			len += ez_snprintf(buf + len, size - len, "  %s", str);
		}

		data = save;
		datalen = savelen;

		len += ez_snprintf(buf + len, size - len, "  |");

		for (i = 0; datalen != 0 && i < 16; data++, datalen--, i++) {
			c = (unsigned char)(isprint(*data) ? *data : '.');
			len += ez_snprintf(buf + len, size - len, "%c", c);
		}
		len += ez_snprintf(buf + len, size - len, "|\n");

		off += 16;
	}

	n = write(l->fd, buf, len);
	if (n < 0) {
		l->nerror++;
	}

	errno = save_error_no;
}
