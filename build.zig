const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("space-sim", null);
    exe.setBuildMode(mode);
    exe.addCSourceFile("main.c", [_][]const u8{"-std=c99"});
    exe.linkSystemLibrary("c");
    exe.linkSystemLibrary("raylib");

    // LATER: segfault handler
    // exe.addObject(b.addObject("segfault", "segfault.zig"));

    // LATER: statically link raylib (needs platform-dependent logic)
    // exe.addObjectFile("libraylib.a");
    // exe.linkFramework("Cocoa");
    // exe.linkFramework("CoreVideo");
    // exe.linkFramework("GLUT");
    // exe.linkFramework("IOKit");
    // exe.linkFramework("OpenGL");

    const run_cmd = exe.run();

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);

    b.default_step.dependOn(&exe.step);
    b.installArtifact(exe);
}
