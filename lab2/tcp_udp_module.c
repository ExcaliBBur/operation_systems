#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <net/tcp.h>
#include <net/tcp_states.h>
#include <net/udp.h>

#include <net/net_namespace.h>


static const char *const state_names[] = {
		"ESTABLISHED",
		"SYN_SENT",
		"SYN_RECV",
		"FIN_WAIT1",
		"FIN_WAIT2",
		"TIME_WAIT",
		"CLOSE",
		"CLOSE_WAIT",
		"LACT_ACK",
		"LISTEN",
		"CLOSING",
		"NEW_SYN_RECV"
};


static void addr_port_show(struct seq_file *seq, const void* addr, __u16 port) 
{
	seq_setwidth(seq, 23);
	seq_printf(seq, "%pI4", addr);
	if (port == 0) {
		seq_puts(seq, ":*");
	} else {
		seq_printf(seq, ":%d", port);
	}
	seq_pad(seq, ' ');
}

static int tcp_seq_show(struct seq_file *seq, void *v) 
{
	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "%5s %-7s %-7s %-23s %-23s %s\n", "Proto", 
		"Recv-Q", "Send-Q", "Local Address", "Foreign Address", "State");
		return 0;	
	}
		
	int rx_queue;
	int tx_queue;
		
	const void *dest;
	const void *src;
		
	__u16 destp;
	__u16 srcp;
	int state;
		
	struct sock *sk;
	
	sk = v;
	if (sk->sk_state == TCP_LISTEN) {
		return 0;
	}
				
	if (sk->sk_state == TCP_TIME_WAIT) {
		const struct inet_timewait_sock *tw = v;

		rx_queue = 0;
		tx_queue = 0;
					
		dest = &tw->tw_daddr;
		src = &tw->tw_rcv_saddr;
						
		destp = ntohs(tw->tw_dport);
		srcp = ntohs(tw->tw_sport);
		state = sk->sk_state;
	} else {
		const struct tcp_sock *tp;
		const struct inet_sock *inet;

		tp = tcp_sk(sk);
		inet = inet_sk(sk);

		rx_queue = max_t(int, tp->rcv_nxt - tp->copied_seq, 0);
		tx_queue = tp->write_seq - tp->snd_una;

		dest = &inet->inet_daddr;
		src = &inet->inet_rcv_saddr;
					
		destp = ntohs(inet->inet_dport);
		srcp = ntohs(inet->inet_sport);
		state = sk->sk_state;
	}

	seq_printf(seq, "%5s %-7d %-7d ", "tcp", rx_queue, tx_queue);
	addr_port_show(seq, src, srcp);
	addr_port_show(seq, dest, destp);

	seq_printf(seq, "%s ", state_names[state - 1]);
		
	seq_printf(seq, "\n");
		
	return 0;
}

static const struct seq_operations tcpstat_seq_ops = {
	.show		= tcp_seq_show,
	.start		= tcp_seq_start,
	.next		= tcp_seq_next,
	.stop		= tcp_seq_stop,
};

static struct tcp_seq_afinfo tcpstat_seq_afinfo = {
	
};

static int udp_seq_show(struct seq_file *seq, void *v) 
{
	if (v == SEQ_START_TOKEN) {
		seq_printf(seq, "%5s %-7s %-7s %-23s %-23s %s\n", "Proto", 
		"Recv-Q", "Send-Q", "Local Address", "Foreign Address", "State");
		return 0;
	}
	int state;
	struct sock *sk;
		
	sk = v;
		
	state = sk->sk_state;
	
	if (sk->sk_state == TCP_CLOSE) {
		return 0;
	}
		
	int tx_queue = sk_wmem_alloc_get(sk);
	int rx_queue = sk_rmem_alloc_get(sk);
		
	struct inet_sock *inet = inet_sk(sk);
		
	const void *dest;
	const void *src;
		
	__u16 destp;
	__u16 srcp;
		
	dest = &inet->inet_daddr;
	src = &inet->inet_rcv_saddr;

	destp = ntohs(inet->inet_dport);
	srcp = ntohs(inet->inet_sport);

	seq_printf(seq, "%5s %-7d %-7d ", "udp", rx_queue, tx_queue);
		
	addr_port_show(seq, src, srcp);
	addr_port_show(seq, dest, destp);
		
	seq_printf(seq, "%s ", state_names[state - 1]);
		
	seq_printf(seq, "\n");
		
	return 0;
}

static const struct seq_operations udpstat_seq_ops = {
	.start		= udp_seq_start,
	.next		= udp_seq_next,
	.stop		= udp_seq_stop,
	.show		= udp_seq_show,
};

static struct udp_seq_afinfo udpstat_seq_afinfo = {

};

static int __net_init knetstat_net_init(struct net *net) 
{
	if (!proc_create_net_data("tcpstat", 0444, net->proc_net, &tcpstat_seq_ops,
			sizeof(struct tcp_iter_state), &tcpstat_seq_afinfo))
		goto exit;

	if (!proc_create_net_data("udpstat", 0444, net->proc_net, &udpstat_seq_ops,
			sizeof(struct udp_iter_state), &udpstat_seq_afinfo))
		goto exit;

	return 0;

exit:
	return -ENOMEM;
}

static void __net_exit knetstat_net_exit(struct net *net) 
{
	remove_proc_entry("tcpstat", net->proc_net);
	remove_proc_entry("udpstat", net->proc_net);
}

static struct pernet_operations knetstat_net_ops = { 
	.init = knetstat_net_init,
	.exit = knetstat_net_exit, 
};

static int knetstat_init(void) 
{
	int err;

	err = register_pernet_subsys(&knetstat_net_ops);
	if (err < 0)
		return err;

	return 0;
}

static void knetstat_exit(void) 
{
	unregister_pernet_subsys(&knetstat_net_ops);
}

module_init(knetstat_init)
module_exit(knetstat_exit)

MODULE_LICENSE("GPL");

MODULE_AUTHOR("ExcaliBBur");

MODULE_DESCRIPTION("A Linux model designed for the first part of netstat utility");
