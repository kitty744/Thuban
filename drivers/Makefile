drivers_srcs := $(shell find drivers -name '*.c' -o -name '*.s')
objs-y += $(patsubst %.c,%.o,$(patsubst %.s,%.o,$(drivers_srcs)))