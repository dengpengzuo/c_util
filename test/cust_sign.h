//
// Created by zuodp on 2021/1/23.
//

#ifndef EZ_CUTIL_CUST_SIGN_H
#define EZ_CUTIL_CUST_SIGN_H

#include <signal.h>
#include <error.h>
#include <errno.h>

struct ez_signal {
    int signo;
    int flags;
    char* signame;

    void (*handler)(int signo);

    void (*cust_handler)(struct ez_signal* sig);
}; // 字节对齐, sizeof(struct ez_signal)=32 bytes.

extern void signal_quit_handler(struct ez_signal* sig);

static struct ez_signal cust_signals[] = {
    { SIGHUP, 0, "SIGHUP", SIG_IGN, NULL },
    { SIGPIPE, 0, "SIGPIPE", SIG_IGN, NULL },
    { SIGINT, 0, "SIGINT", SIG_DFL, signal_quit_handler },
    { SIGQUIT, 0, "SIGQUIT", SIG_DFL, signal_quit_handler },
    { SIGTERM, 0, "SIGTERM", SIG_DFL, signal_quit_handler },
    { 0, 0, "NULL", NULL, NULL }
};

static void dispatch_signal_handler(int signo)
{
    struct ez_signal* sig;
    for (sig = cust_signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    log_info("signal %d (%s) received", signo, sig->signame);
    if (sig->signo == 0 || sig->cust_handler == NULL)
        return;

    sig->cust_handler(sig);
}

void cust_signal_init(void)
{
    struct ez_signal* sig;
    int status;
    struct sigaction sa;

    for (sig = cust_signals; sig->signo != 0; sig++) {
        memset(&sa, 0, sizeof(sa));

        if (sig->handler == SIG_DFL)
            sa.sa_handler = &dispatch_signal_handler;
        else
            sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            log_error("sigaction(%s) failed: %s", sig->signame, strerror(errno));
            return;
        }
    }
}

#endif //EZ_CUTIL_CUST_SIGN_H
