.PHONY: all clean ping_pong

# Use native compilation with HiPE optimization for best performance
ERLC = erlc
ERLC_FLAGS = +native +'{hipe, [o3]}'
ERL = erl
ERL_FLAGS = -noshell

all: ping_pong

ping_pong: ping_pong.erl
	$(ERLC) $(ERLC_FLAGS) $<

run_ping_pong: ping_pong
	$(ERL) $(ERL_FLAGS) -s ping_pong start

clean:
	rm -f *.beam

.DEFAULT_GOAL := all
