- build
    - libraylib-2.5.0.a only used for static builds
- zig
    - https://ziglang.org/#Zig-Build-System
    - https://ziglang.org/#Zig-is-also-a-C-compiler
- raylib
    - https://www.raylib.com/cheatsheet/cheatsheet.html
    - https://github.com/raysan5/raylib/wiki/Working-on-macOS
- safe types
    ```
    // get
    #define unit(x) (x)
    #define unit(x) ((x).unit)
    // set
    #define set_unit(x) (x)
    #define set_unit(x) {.unit=(x)}

    typedef struct { float kg; } kg;
    typedef struct { float _kg; } _kg; // inverse mass
    typedef struct { float m; } m;
    typedef struct { float s; } s;
    typedef struct { float m_s; } m_s;
    typedef struct { float m_s2; } m_s2;
    typedef struct { float m3_kgs2; } m3_kgs2;
    ```

