#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "llvm-runtime.h"

#ifdef GC_USE_BOEHM
#include <gc/gc.h>

#ifdef OPEN_DYLAN_PLATFORM_DARWIN
enum dyld_tlv_states {
  dyld_tlv_state_allocated = 10,
  dyld_tlv_state_deallocated = 20,
};
typedef struct {
  size_t info_size;
  void *tlv_addr;
  size_t tlv_size;
} dyld_tlv_info;
typedef void (^dyld_tlv_state_change_handler)(enum dyld_tlv_states state, const dyld_tlv_info *info);
extern void dyld_enumerate_tlv_storage(dyld_tlv_state_change_handler handler);
extern void dyld_register_tlv_state_change_handler(enum dyld_tlv_states state, dyld_tlv_state_change_handler handler);
#endif

#endif

void _Init_Run_Time(void)
{
  static int initp = 0;
  if (!initp) {
    initp = 1;

    // register our dylan-level atexit mechanism
    atexit(call_application_exit_functions_internal);

    // set up signal handlers
#ifdef OPEN_DYLAN_PLATFORM_UNIX
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
#endif

#ifdef GC_USE_BOEHM
    GC_INIT();

#ifdef OPEN_DYLAN_PLATFORM_DARWIN
    dyld_tlv_state_change_handler handler
      = ^(enum dyld_tlv_states state, const dyld_tlv_info *info) {
       char *start = info->tlv_addr;
       char *end = start + info->tlv_size;
       if (state == dyld_tlv_state_allocated) {
         GC_add_roots(start, end);
       }
       else if (state == dyld_tlv_state_deallocated) {
         GC_remove_roots(start, end);
       }
    };
    dyld_enumerate_tlv_storage(handler);
    dyld_register_tlv_state_change_handler(dyld_tlv_state_allocated, handler);
    dyld_register_tlv_state_change_handler(dyld_tlv_state_deallocated, handler);
#endif
#endif
  }
}
