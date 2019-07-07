.PHONY = build build-cc-dynamic build-cc-static clean list-builtins list-deps run FORCE

ZIG=/Users/geoff/Code/lang/zig/build/bin/zig

build: FORCE
	@${ZIG} build

# dynamically linked
build-cc-dynamic:
	cc main.c `pkg-config --libs --cflags raylib` -o space-sim

build-cc-static:
	cc libraylib-2.5.0.a main.c -o space-sim \
		-framework Cocoa -framework CoreVideo\
		-framework GLUT-framework IOKit -framework OpenGL \
		-Xlinker -w

clean:
	rm -rf zig-cache

list-builtins:
	@${ZIG} builtin

list-deps:
	@${ZIG} build install
	otool -L zig-cache/bin/space-sim

run: FORCE
	${ZIG} build run

FORCE:

