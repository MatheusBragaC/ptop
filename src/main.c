#include "cfg.h"
#include "collector.h"
#include "display.h"
#include "model.h"
#include "netlink_driver.h"
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
    log_msg(LOG_INFO, "Iniciando ptop...");
    
    // Inicializa Netlink (Kernel Hacker Mode)
    if (netlink_init() != 0) {
        log_msg(LOG_WARN, "Aviso: Netlink init falhou. Modo Processos desativado.");
        // Não falha fatalmente, apenas desabilita features avançadas
    }

    // 1. Configura Tratamento de Sinais via signalfd
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGWINCH); // Adiciona handler para WINCH

    // Bloqueia sinais para serem tratados via descritor de arquivo
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

    // 2. Configura Timer via timerfd
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

    // 3. Configura Epoll
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        log_error_errno("epoll_create1");
        return 1;
    }

    struct epoll_event ev, events[MAX_EVENTS];
    
    // Adiciona STDIN ao Epoll para Teclado/Mouse
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1)
    {
        log_error_errno("epoll_ctl: stdin");
        return 1;
    }

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

    // 4. Inicializa Componentes da Aplicação
    CpuCollector *collector = collector_init();
    if (!collector)
    {
        log_msg(LOG_FATAL, "Falha ao inicializar o coletor de CPU.");
        return 1;
    }

    DisplayLayout layout = {0};
    CpuModel model = {0};
    
    setup_terminal();
    update_layout(&layout);
    
    // Renderização Inicial
    log_msg(LOG_INFO, "Atualização Inicial do Coletor...");
    collector_update(collector, &model);
    log_msg(LOG_INFO, "Renderização Inicial...");
    render_interface(&model, &layout);
    log_msg(LOG_INFO, "Entrando no Loop de Eventos...");

    // 5. Loop de Eventos
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
            if (events[n].data.fd == STDIN_FILENO)
            {
                // Trata Entrada
                char buf[32];
                ssize_t n_read = read(STDIN_FILENO, buf, sizeof(buf));
                if (n_read > 0)
                {
                    if (buf[0] == 'q' || buf[0] == 'Q') 
                    {
                        running = 0;
                    }
                    else if (buf[0] == '\033' && n_read >= 6 && buf[1] == '[' && buf[2] == 'M')
                    {
                        // Codificação de Mouse X11: \033 [ M b x y
                        // b = botão + 32
                        // x = x + 32
                        // y = y + 32
                        int btn = buf[3] - 32;
                        int x = buf[4] - 32;
                        int y = buf[5] - 32;
                        
                        // Apenas loga o clique por enquanto (Verificação)
                        if (btn == 0) // Clique esquerdo
                        {
                            log_msg(LOG_INFO, "Clique do Mouse em: %d, %d", x, y);
                            // Easter Egg: força atualização explícita no clique
                            render_interface(&model, &layout);
                        }
                    }
                }
            }
            else if (events[n].data.fd == sfd)
            {
                // Trata Sinal
                struct signalfd_siginfo fdsi;
                ssize_t s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
                if (s == sizeof(struct signalfd_siginfo))
                {
                    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM)
                    {
                        running = 0;
                    }
                    else if (fdsi.ssi_signo == SIGWINCH)
                    {
                        // Trata Redimensionamento
                        update_layout(&layout);
                        // Força sequencia de limpeza de tela para evitar artefatos
                        printf("\033[2J"); 
                        fflush(stdout);
                        render_interface(&model, &layout);
                    }
                }
            }
            else if (events[n].data.fd == tfd)
            {
                // Trata Timer
                uint64_t expirations;
                ssize_t s = read(tfd, &expirations, sizeof(uint64_t));
                if (s == sizeof(uint64_t))
                {
                    collector_update(collector, &model);
                    render_interface(&model, &layout);
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
