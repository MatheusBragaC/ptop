#include "netlink_driver.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>
#include <linux/rtnetlink.h> // for RTA_* macros

#ifndef GENLMSG_DATA
#define GENLMSG_DATA(glh) ((void *)((char*)(glh) + GENL_HDRLEN))
#endif

#ifndef GENLMSG_PAYLOAD
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#endif

#define NLA_ALIGNTO		4
#define NLA_ALIGN(len)		(((len) + NLA_ALIGNTO - 1) & ~(NLA_ALIGNTO - 1))
#define NLA_HDRLEN		((int) NLA_ALIGN(sizeof(struct nlattr)))
#define NLA_DATA(nla)		((void *)((char*)(nla) + NLA_HDRLEN))
#define NLA_NEXT(nla,len)	((len) -= NLA_ALIGN((nla)->nla_len), \
				  (struct nlattr*)(((char*)(nla)) + NLA_ALIGN((nla)->nla_len)))
#define NLA_OK(nla,len)		((len) >= (int)sizeof(struct nlattr) && \
				 (nla)->nla_len >= sizeof(struct nlattr) && \
				 (nla)->nla_len <= (len))
#define NLA_PAYLOAD(nla)	((int)((nla)->nla_len) - NLA_HDRLEN)

// Macros auxiliares para tamanho de buffer
#define MAX_MSG_LEN 4096

static int nl_sock = -1;
static int family_id = 0;

// Estrutura de Requisição Generic Netlink
struct nl_req_s {
    struct nlmsghdr n;
    struct genlmsghdr g;
    char buf[256];
};

// --- Funções Auxiliares Internas ---

static int netlink_send_cmd(int sock_fd, uint16_t family_id, uint32_t pid, uint8_t cmd, uint16_t version, void* payload, int payload_len)
{
    struct nl_req_s req;
    struct sockaddr_nl nladdr;
    
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN + payload_len);
    req.n.nlmsg_type = family_id;
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_seq = 0;
    req.n.nlmsg_pid = getpid();
    
    req.g.cmd = cmd;
    req.g.version = version;

    if (payload && payload_len > 0)
    {
        memcpy(GENLMSG_DATA(&req.g), payload, payload_len);
    }
    
    // Envia
    if (sendto(sock_fd, (char*)&req, req.n.nlmsg_len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0)
    {
        log_error_errno("netlink sendto");
        return -1;
    }
    return 0;
}

static int add_attr(struct nlmsghdr *n, int maxlen, int type, const void *data, int alen)
{
    int len = NLA_HDRLEN + NLA_ALIGN(alen); 
    // Construção padrão RTA/NLA:
    // nla_len = NLA_HDRLEN + alen; // Comprimento real
    // Espaço total usado = NLA_ALIGN(nla_len);
    
    int nla_len = NLA_HDRLEN + alen;
    
    if (NLMSG_ALIGN(n->nlmsg_len) + NLA_ALIGN(nla_len) > maxlen)
        return -1;

    struct nlattr *nla = (struct nlattr *) (((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
    nla->nla_type = type;
    nla->nla_len = nla_len;
    memcpy(NLA_DATA(nla), data, alen);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + NLA_ALIGN(nla_len);
    return 0;
}

static int get_family_id(int sock_fd)
{
    struct {
        struct nlmsghdr n;
        struct genlmsghdr g;
        char buf[256];
    } req;
    
    struct sockaddr_nl nladdr;
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    req.n.nlmsg_type = GENL_ID_CTRL;
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_seq = 1;
    req.n.nlmsg_pid = getpid();
    req.g.cmd = CTRL_CMD_GETFAMILY;
    req.g.version = 0x1;

    // Adiciona atributo string "TASKSTATS"
    if (add_attr(&req.n, sizeof(req), CTRL_ATTR_FAMILY_NAME, TASKSTATS_GENL_NAME, strlen(TASKSTATS_GENL_NAME)+1) < 0)
    {
        log_msg(LOG_ERROR, "Netlink: Falha ao adicionar atributo");
        return -1;
    }

    if (sendto(sock_fd, (char*)&req, req.n.nlmsg_len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0)
    {
        log_error_errno("Netlink: Envia Requisição de Family ID");
        return -1;
    }

    // Recebe Resposta
    char buf[4096];
    ssize_t rep_len = recv(sock_fd, buf, sizeof(buf), 0);
    if (rep_len < 0) return -1;

    struct nlmsghdr *nh = (struct nlmsghdr*)buf;
    if (!NLMSG_OK(nh, rep_len)) return -1;

    if (nh->nlmsg_type == NLMSG_ERROR) return -1;

    struct genlmsghdr *gh = (struct genlmsghdr*) NLMSG_DATA(nh);
    struct nlattr *na = (struct nlattr*) ((char*)gh + GENL_HDRLEN);
    int len = nh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);

    while (len > 0 && NLA_OK(na, len))
    {
        if (na->nla_type == CTRL_ATTR_FAMILY_ID)
        {
            return *(uint16_t*) NLA_DATA(na);
        }
        na = NLA_NEXT(na, len);
    }
    return -1;
}

// --- Funções Públicas ---

int netlink_init(void)
{
    nl_sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_sock < 0)
    {
        log_error_errno("Netlink: socket");
        return -1;
    }

    struct sockaddr_nl addr;
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();

    if (bind(nl_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        log_error_errno("Netlink: bind");
        close(nl_sock);
        nl_sock = -1;
        return -1;
    }

    family_id = get_family_id(nl_sock);
    if (family_id <= 0)
    {
        log_msg(LOG_ERROR, "Netlink: Erro ao obter TASKSTATS family id");
        close(nl_sock);
        nl_sock = -1;
        return -1;
    }

    log_msg(LOG_INFO, "Netlink: TASKSTATS Family ID = %d", family_id);
    return 0;
}

void netlink_cleanup(void)
{
    if (nl_sock >= 0) close(nl_sock);
}

int netlink_get_stats(pid_t pid, ProcessStats* stats)
{
    if (nl_sock < 0 || family_id <= 0) return -1;

    // Envia Requisição
    struct {
        struct nlmsghdr n;
        struct genlmsghdr g;
        char buf[256];
    } req;
    
    struct sockaddr_nl nladdr;
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    req.n.nlmsg_type = family_id;
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_seq = 2; // Seq não importa muito aqui
    req.n.nlmsg_pid = getpid();
    req.g.cmd = TASKSTATS_CMD_GET;
    req.g.version = 0x1;

    // Adiciona atributo PID
    if (add_attr(&req.n, sizeof(req), TASKSTATS_CMD_ATTR_PID, &pid, sizeof(pid)) < 0)
        return -1;

    if (sendto(nl_sock, (char*)&req, req.n.nlmsg_len, 0, (struct sockaddr*)&nladdr, sizeof(nladdr)) < 0)
        return -1;

    // Recebe
    char buf[4096];
    ssize_t len = recv(nl_sock, buf, sizeof(buf), 0);
    if (len < 0) return -1;
    
    struct nlmsghdr *nh = (struct nlmsghdr*)buf;
    if (!NLMSG_OK(nh, len) || nh->nlmsg_type == NLMSG_ERROR) return -1;

    struct genlmsghdr *gh = (struct genlmsghdr*) NLMSG_DATA(nh);
    struct nlattr *na = (struct nlattr*) ((char*)gh + GENL_HDRLEN);
    int p_len = nh->nlmsg_len - NLMSG_LENGTH(GENL_HDRLEN);

    while (p_len > 0 && NLA_OK(na, p_len))
    {
        if (na->nla_type == TASKSTATS_TYPE_AGGR_PID)
        {
            // Dentro de AGGR_PID há outro atributo aninhado: TASKSTATS_TYPE_STATS
            struct nlattr *nested_na = (struct nlattr*) NLA_DATA(na);
            int nested_len = NLA_PAYLOAD(na);
            
             while (nested_len > 0 && NLA_OK(nested_na, nested_len))
             {
                 if (nested_na->nla_type == TASKSTATS_TYPE_STATS)
                 {
                     struct taskstats *ts = (struct taskstats*) NLA_DATA(nested_na);
                     // Popula nossa struct
                     stats->pid = pid;
                     strncpy(stats->comm, ts->ac_comm, 32);
                     stats->cpu_usage = ts->cpu_run_real_total; // nanosegundos
                     // ts->ac_etime (decorrido)
                     
                     // VmRSS é mais complexo em taskstats (as vezes mais simples em /proc), 
                     // mas vamos ver se usamos um dos campos de memória se disponível
                     // taskstats tem 'rss_stat'? kernels antigos podem não ter.
                     // usando 0 por enquanto.
                     stats->vm_rss = 0; 
                     
                     return 0;
                 }
                 nested_na = NLA_NEXT(nested_na, nested_len);
             }
        }
        na = NLA_NEXT(na, p_len);
    }

    return -1;
}
