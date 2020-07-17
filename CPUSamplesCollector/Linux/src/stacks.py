import struct
from bcc import BPF, DEBUG_SOURCE

bpf_text = """
#include <uapi/linux/ptrace.h>
#include <uapi/linux/bpf_perf_event.h>

struct event_t
{
  u64 ts;
  u64 ip;
  u64 pid_tgid;
  u64 cgroup_id;
  u32 user_stack_id;
  u32 kernel_stack_id;
};

BPF_PERCPU_ARRAY(evt_buf, struct event_t, 1);
BPF_STACK_TRACE(stacks, 32 * 1024);
BPF_PERF_OUTPUT(events);

int do_perf_event(struct bpf_perf_event_data *ctx)
{
    u64 ts = bpf_ktime_get_ns();

    int key = 0;
    struct event_t* evt = evt_buf.lookup(&key);
    if (evt == 0) { return 0; }

    evt->ts = ts;
    evt->ip = PT_REGS_IP(&ctx->regs);
    evt->pid_tgid = bpf_get_current_pid_tgid();
    evt->cgroup_id = bpf_get_current_cgroup_id();
    evt->user_stack_id = stacks.get_stackid(&ctx->regs, BPF_F_USER_STACK);
    evt->kernel_stack_id = stacks.get_stackid(&ctx->regs, 0);

    events.perf_submit(ctx, evt, sizeof(struct event_t));

    return 0;
};
"""

bpf = BPF(text=bpf_text, debug=DEBUG_SOURCE)
bytecode = bpf.dump_func("do_perf_event")
f = open("stacks.ebpf", "w")
f.write(bytecode)
f.close()
