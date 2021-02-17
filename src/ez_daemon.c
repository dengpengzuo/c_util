#include "ez_daemon.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int ez_daemonize(int nochdir, int noclose)
{
    int fd;
    int pid;

    pid = fork();
    switch (pid) {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        // parent process exit, but not close parent open resources.
        // let childrent safe go ....
        _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1) /* become session leader */
        return (-1);

    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        perror("child signal(SIGHUP, SIG_IGN) error!");
        return -1;
    }

    /* 2nd fork turns child into a non-session leader: cannot acquire terminal */
    pid = fork();
    switch (pid) {
    case -1:
        return -1;
    case 0:
        break;
    default:
        /* 1st child terminates */
        _exit(EXIT_SUCCESS);
    }

    /* change working directory */
    if (nochdir == 0) {
        if (chdir("/") != 0) {
            perror("chdir");
            return (-1);
        }
    }

    /* clear file mode creation mask */
    umask(0);

    /* redirect stdin, stdout and stderr to "/dev/null" */
    // children process set (stdin,stdout,stderr) -> fd
    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) {
        // close STDIN_FILENO, switch children process STDIN_FILENO file pointer to fd
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            return (-1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            return (-1);
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            return (-1);
        }
        // fd 超过x stderrno, close fd
        if (fd > STDERR_FILENO) {
            if (close(fd) < 0) {
                perror("close");
                return (-1);
            }
        }
    }
    return (0);
}
