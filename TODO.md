# TODO

## High priority
- fixed time step
    - https://www.gamasutra.com/blogs/BramStolk/20160408/269988/Fixing_your_time_step_the_easy_way_with_the_golden_48537_ms.php
    - https://web.archive.org/web/20190506122532/http://gafferongames.com/post/fix_your_timestep/
    - http://lspiroengine.com/?p=378

## Low hanging fruit
- update readme with dependencies (zig 0.4.0+ [or use build-cc-*], raylib)
- how to run (make run)

## Medium priority
- more planets
- asteroids? generate randomly and distribute in a ring (use total mass of asteroid belt; ~3.0e+21)
- zoom in / out
- time speed up / slown
- use physac? https://github.com/victorfisac/Physac

## Low priority
- TODO only use scientific notation if number is `>= 1000` or `< 0.001`
- TODO numeric separators
- static builds via build.zig
- c segfault handler (once macos is supported)
