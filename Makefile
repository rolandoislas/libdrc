-include Makefile.config

LIBDRC_SRCS:=$(wildcard src/*.cpp)
LIBDRC_OBJS:=$(LIBDRC_SRCS:.cpp=.o)

ifeq ($(DEMOS),y)
DEMOS_NAMES:=3dtest
DEMOS_SRCS:=demos/framework/framework.cpp \
            $(foreach d,$(DEMOS_NAMES),demos/$(d)/main.cpp)
DEMOS_BINS:=$(foreach d,$(DEMOS_NAMES),demos/$(d)/$(d))
endif

ALL_SRCS:=$(LIBDRC_SRCS) $(DEMOS_SRCS)
ALL_OBJS:=$(ALL_SRCS:.cpp=.o)

all: libdrc demos

libdrc: libdrc.so libdrc.a

demos: $(DEMOS_BINS)

libdrc.a: $(LIBDRC_OBJS)
	rm -f $@
	ar rcs $@ $^

libdrc.so: $(LIBDRC_OBJS) Makefile.config
	$(CXX) $(LDFLAGS) -shared -o $@ $(LIBDRC_OBJS)

define build_demo =
demos/$(1)/$(1): demos/$(1)/main.o demos/framework/framework.o libdrc.a
	$$(CXX) -o $$@ $$^ $$(LDFLAGS) $$(LDFLAGS_DEMOS)
endef
$(foreach d,$(DEMOS_NAMES),$(eval $(call build_demo,$(d))))

%.o: %.cpp Makefile.config
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(ALL_OBJS)

distclean: clean
	rm -f .depend Makefile.config libdrc.a libdrc.so $(DEMOS_BINS)

install:
	@echo TODO

uninstall:
	@echo TODO

Makefile.config:
	$(error You must run ./configure first)

-include .depend
.depend: Makefile.config
	$(CXX) -MM $(CXXFLAGS) $(ALL_SRCS) > $@

.PHONY: all libdrc demos clean distclean install uninstall
