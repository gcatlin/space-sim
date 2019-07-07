.PHONY = build clean list-deps run FORCE

build: FORCE
	@zig build

# build-dynamic:
# 	@mkdir build
# 	cc main.c `pkg-config --libs --cflags raylib` -o space-sim

# build-static:
# 	cc libraylib-2.5.0.a main.c \
# 	    -o space-sim \
# 	    -framework Cocoa -framework CoreVideo\
# 	    -framework GLUT-framework IOKit -framework OpenGL \
# 	    -Xlinker -w

clean:
	rm -rf zig-cache

list-deps:
	@zig build install
	@otool -L zig-cache/bin/space-sim

run: FORCE
	@zig build run

FORCE:

