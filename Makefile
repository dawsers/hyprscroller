.PHONY: all debug release clean install dev

CXXFLAGS = -fPIC -fno-gnu-unique -std=c++2b

SRCS = src/main.cpp \
	src/dispatchers.cpp \
	src/scroller.cpp

OBJS = $(SRCS:%.cpp=%.o)

debug: CXXFLAGS += -g
debug: hyprscroller.so

release: CXXFLAGS += -O3
release: hyprscroller.so

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -DWLR_USE_UNSTABLE \
	    -MT $@ -MMD -MP -MF $*.d \
	    `pkg-config --cflags pixman-1 libdrm hyprland` \
	    -c $< -o $@

all: clean release

hyprscroller.so: $(OBJS)
	$(CXX) $(CXXFLAGS) -shared \
	    -DWLR_USE_UNSTABLE \
	    `pkg-config --cflags pixman-1 libdrm hyprland` \
	    $(OBJS) \
	    -o hyprscroller.so

clean:
	rm -f $(OBJS)
	rm -f $(SRCS:%.cpp=%.d)
	rm -f ./hyprscroller.so
	rm -f compile_commands.json

install: release
	mkdir -p `xdg-user-dir`/.config/hypr/plugins
	cp hyprscroller.so `xdg-user-dir`/.config/hypr/plugins

dev: clean compile_commands.json

compile_commands.json: Makefile $(SRCS)
	bear -- make debug

-include $(SRCS:%.cpp=%.d)
