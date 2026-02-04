#include "cfg.h"
#include "collector.h"
#include "display.h"
#include "model.h"
#include "logger.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define MAX_EVENTS 2

int main()
{
    log_init();
    log_msg(LOG_INFO, "Starting ptop...");

    // 1. Setup Signal Handling via signalfd
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);

    // Block signals so they are handled via file descriptor
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        log_error_errno("sigprocmask");
        return 1;
    }

    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1)
    {
        log_error_errno("signalfd");
        return 1;
    }

    // 2. Setup Timer via timerfd
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1)
    {
        log_error_errno("timerfd_create");
        return 1;
    }

    struct itimerspec ts;
    ts.it_interval.tv_sec = DELAY_MS / 1000;
    ts.it_interval.tv_nsec = (DELAY_MS % 1000) * 1000000;
    ts.it_value = ts.it_interval;

    if (timerfd_settime(tfd, 0, &ts, NULL) == -1)
    {
        log_error_errno("timerfd_settime");
        return 1;
    }

    // 3. Setup Epoll
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        log_error_errno("epoll_create1");
        return 1;
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1)
    {
        log_error_errno("epoll_ctl: sfd");
        return 1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev) == -1)
    {
        log_error_errno("epoll_ctl: tfd");
        return 1;
    }

    // 4. Initialize Application Components
    CpuCollector *collector = collector_init();
    if (!collector)
    {
        log_msg(LOG_FATAL, "Failed to initialize CPU collector.");
        return 1;
    }

    CpuModel model = {0};
    setup_terminal();
    
    // Initial render
    collector_update(collector, &model);
    render_interface(&model);

    // 5. Event Loop
    int running = 1;
    while (running)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            if (errno == EINTR) continue;
            log_error_errno("epoll_wait");
            break;
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == sfd)
            {
                // Handle Signal
                struct signalfd_siginfo fdsi;
                ssize_t s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
                if (s == sizeof(struct signalfd_siginfo))
                {
                    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM)
                        running = 0;
                }
            }
            else if (events[n].data.fd == tfd)
            {
                // Handle Timer
                uint64_t expirations;
                ssize_t s = read(tfd, &expirations, sizeof(uint64_t));
                if (s == sizeof(uint64_t))
                {
                    collector_update(collector, &model);
                    render_interface(&model);
                }
            }
        }
    }

    restore_terminal();
    collector_cleanup(collector);
    close(epollfd);
    close(tfd);
    close(sfd);

    log_msg(LOG_INFO, "Shutdown complete.");
    log_cleanup();

    return 0;
}
