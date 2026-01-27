mm_srcs := $(shell find mm -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(mm_srcs)))