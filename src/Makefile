# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
SDL_CFLAGS=$(shell sdl2-config --cflags)
SDL_LIBS=$(shell sdl2-config --libs) -lSDL2_mixer
GLES2_CFLAGS=
GLES2_LIBS=-lGLESv2
READLINE_LIBS=-lreadline

SHAREDFLAGS=-Werror=implicit-function-declaration -Wmissing-declarations -Wall \
       -Wextra -Wno-unused-parameter \
       -Og -Wall -ggdb3 -g3 -I. \
       -DFLIP_ENDIAN_HACK -DPSYS_DEBUGGER -DGAME_CHEATS -DENABLE_DEBUGUI \
       $(SDL_CFLAGS) $(GLES2_CFLAGS)
CFLAGS=$(SHAREDFLAGS) -Wmissing-prototypes
CFLAGS += -std=c99
CFLAGS += -D_GNU_SOURCE
CXXFLAGS=$(SHAREDFLAGS) -Iimgui
LDFLAGS=$(SDL_LIBS) $(GLES2_LIBS) $(READLINE_LIBS) -lm

# CXX is only used for debug gui
LINK=$(CXX)

PSYS_OBJS= \
	psys/psys_bootstrap.o \
	psys/psys_debug.o \
	psys/psys_interpreter.o \
	psys/psys_opcodes.o \
	psys/psys_registers.o \
	psys/psys_rsp.o \
	psys/psys_save_state.o \
	psys/psys_set.o \
	psys/psys_task.o

GAME_OBJS= \
	game/game_gembind.o \
	game/game_screen.o \
	game/game_shiplib.o \
	game/game_sound.o \
	game/game_debug.o \
	util/debugger.o \
	util/util_img.o \
	util/util_time.o

TEST_OBJS= \
	test/test_util.o

TOOL_OBJS= \
	util/write_bmp.o

DEBUGUI_OBJS= \
	debugui/debugui.o \
	imgui/imgui.o \
	imgui/imgui_demo.o \
	imgui/imgui_draw.o \
	imgui/imgui_impl_sdl_gles2.o

HEADERS= \
	psys/psys_bootstrap.h \
	psys/psys_constants.h \
	psys/psys_debug.h \
	psys/psys_helpers.h \
	psys/psys_interpreter.h \
	psys/psys_opcodes.h \
	psys/psys_registers.h \
	psys/psys_rsp.h \
	psys/psys_set.h \
	psys/psys_save_state.h \
	psys/psys_state.h \
	psys/psys_task.h \
	psys/psys_types.h \
	psys/psys_types.h \
	util/memutil.h \
	util/util_img.h \
	util/util_minmax.h \
	util/util_save_state.h \
	util/debugger.h \
	game/game_debug.h \
	game/game_gembind.h \
	game/game_screen.h \
	game/game_shiplib.h \
	game/game_sound.h \
	game/sundog_soundfx.h \
	test/test_util.h

MAIN_OBJS= \
	sundog_compare_trace.o \
	sundog.o \
        test/img_tests.o \
	test/inst_tests.o \
	test/set_tests.o \
	rip_images.o

EXES= \
	sundog \
	sundog_compare_trace \
	test/img_tests \
	test/inst_tests \
	test/set_tests \
	rip_images

all: $(EXES)

sundog: sundog.o $(PSYS_OBJS) $(GAME_OBJS) $(DEBUGUI_OBJS)
	$(LINK) $^ $(LDFLAGS) -o $@

sundog_compare_trace: sundog_compare_trace.o $(PSYS_OBJS) $(GAME_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

rip_images: rip_images.o $(PSYS_OBJS) $(GAME_OBJS) $(TOOL_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

test/set_tests: test/set_tests.o $(PSYS_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

test/inst_tests: test/inst_tests.o $(PSYS_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

test/img_tests: test/img_tests.o $(PSYS_OBJS) $(GAME_OBJS) $(TEST_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

game/game_debuginfo.h: ../tools/gen_debug_info.py ../tools/libcalls_list.py ../tools/appcalls_list.py
	../tools/gen_debug_info.py $@

$(PSYS_OBJS) $(MAIN_OBJS) $(GAME_OBJS): $(HEADERS)

game/game_debug.o: game/game_debuginfo.h

check: $(EXES)
	test/set_tests
	test/inst_tests
	test/img_tests

clean:
	rm -f $(PSYS_OBJS) $(EXES) $(MAIN_OBJS) $(GAME_OBJS) $(TEST_OBJS) $(TOOL_OBJS)
