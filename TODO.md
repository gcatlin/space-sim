# TODO

## High priority
- keys to pan up/down/left/right
- support zoom in / out on sun using mouse wheel
    - fn for drawing an array of ints/floats as a chart
- create a chart_t struct and call DrawChart(chart, data);
    - pos, width, height, border, padding, labels?, max_x, max_y, color_callback?
    - different types via fn names; e.g. DrawBarChart, DrawLineChart
    - rbuf helper fn to facilitate memcpy'ing the data out?

## Low hanging fruit

## Medium priority
- moons!
- press key to select next/prev body
    - show info for selected body
    - body outline (square?, cirlce?) differs from selection indicator (corners of square? different color?)
- click mouse to select body
- zoom in/out on selected body
- solar system bar
    - planets (and moons?) are selectable?
- pan via keys
- pan via mouse
- support speeding up / slowing down time
- read about the Orbit equation https://en.wikipedia.org/wiki/Orbit_equation
- add asteroids? generate randomly and distribute in a ring (use total mass of asteroid belt; ~3.0e+21)
- better game loop / fixed time step
    - https://gameprogrammingpatterns.com/game-loop.html
    - https://web.archive.org/web/20190506122532/http://gafferongames.com/post/fix_your_timestep/
    - https://dewitters.com/dewitters-gameloop/
    - https://www.gamasutra.com/blogs/BramStolk/20160408/269988/Fixing_your_time_step_the_easy_way_with_the_golden_48537_ms.php
    - http://lspiroengine.com/?p=378

## Low priority
- use scientific notation if number is `>= 1000` or `< 0.001`
- display numeric thousands separators
- use physac? https://github.com/victorfisac/Physac
- c segfault handler (once macos is supported)
- static builds via build.zig
