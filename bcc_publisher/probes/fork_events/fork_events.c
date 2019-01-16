/*
 * Copyright (c) 2018 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/pid.h>

/// Saves namespace data into the per-cpu map
static int savePidNamespaceData(struct pid* pid) {
  int index_key = 0U;
  u64 initial_slot = 0U;
  u64* index_ptr = perf_cpu_index.lookup_or_init(&index_key, &initial_slot);
  int index = (index_ptr != NULL ? *index_ptr : initial_slot);

  u64 field = (u64)pid->level;
  perf_event_data.update(&index, &field);
  INCREMENT_EVENT_DATA_INDEX(index);

#pragma unroll
  for (int i = 0; i < 3; i++) {
    field = (u64)pid->numbers[i].nr;
    perf_event_data.update(&index, &field);
    INCREMENT_EVENT_DATA_INDEX(index);
  }

  initial_slot = index; // re-use the same var to avoid wasting stack space
  perf_cpu_index.update(&index_key, &initial_slot);

  return 0;
}

/// clone() handler
int on_tracepoint_sys_enter_clone(
    struct tracepoint__syscalls__sys_enter_clone* args) {
  int event_index = saveEventHeader(EVENTID_SYSENTERCLONE, false, 0);
  saveEventValue(args->clone_flags);

  u32 parent_tid = 0U;
  bpf_probe_read(&parent_tid, sizeof(parent_tid), args->parent_tidptr);

  u32 child_tid = 0U;
  bpf_probe_read(&child_tid, sizeof(child_tid), args->child_tidptr);

  u64 parent_child_tid = (((u64)parent_tid) << 32U) | ((u64)child_tid);
  saveEventValue(parent_child_tid);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

int on_tracepoint_sys_exit_clone(
    struct tracepoint__syscalls__sys_exit_clone* args) {
  int event_index = saveEventHeader(EVENTID_SYSEXITCLONE, true, args->ret);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

/// fork() handler
int on_tracepoint_sys_enter_fork(
    struct tracepoint__syscalls__sys_enter_fork* args) {
  int event_index = saveEventHeader(EVENTID_SYSENTERFORK, false, 0);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

int on_tracepoint_sys_exit_fork(
    struct tracepoint__syscalls__sys_exit_fork* args) {
  int event_index = saveEventHeader(EVENTID_SYSEXITFORK, true, args->ret);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

/// vfork() handler
int on_tracepoint_sys_enter_vfork(
    struct tracepoint__syscalls__sys_enter_vfork* args) {
  int event_index = saveEventHeader(EVENTID_SYSENTERVFORK, false, 0);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

int on_tracepoint_sys_exit_vfork(
    struct tracepoint__syscalls__sys_exit_vfork* args) {
  int event_index = saveEventHeader(EVENTID_SYSEXITVFORK, true, args->ret);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

/// pid_vnr() handler
int on_kprobe_pid_vnr_enter(struct pt_regs* ctx, struct pid* pid) {
  int event_index = saveEventHeader(EVENTID_PIDVNR, false, 0);
  savePidNamespaceData(pid);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(ctx, &event_identifier, sizeof(event_identifier));
  return 0;
}

/// exit() handler
int on_tracepoint_sys_enter_exit(
    struct tracepoint__syscalls__sys_enter_exit* args) {
  int event_index = saveEventHeader(EVENTID_SYSENTEREXIT, false, 0);
  saveEventValue(args->error_code);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}

int on_tracepoint_sys_enter_exit_group(
    struct tracepoint__syscalls__sys_enter_exit_group* args) {
  int event_index = saveEventHeader(EVENTID_SYSENTEREXITGROUP, false, 0);
  saveEventValue(args->error_code);

  u32 event_identifier =
      (((struct task_struct*)bpf_get_current_task())->cpu << 28) |
      (event_index & 0x00FFFFFF);

  events.perf_submit(args, &event_identifier, sizeof(event_identifier));
  return 0;
}