arch_srcs := $(shell find arch/x86_64 -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(arch_srcs)))