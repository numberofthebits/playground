cl.exe ^
 main.c^
 vec.c^
 ecs.c^
 component.c^
 log.c^
 render_system.c^
 game.c^
 math.c^
 assetstore.c^
 os_win.c^
 util.c^
 hashmap.c^
 arena.c^
 animation_system.c^
 .\libs\glad\src\glad.c ^
 Shlwapi.lib ^
 "./libs/GLFW/lib/glfw3dll.lib"^
 /Zi^ /W3 /DEBUG /std:c17 ^
 /I "./libs/GLFW/include" /I "./libs/glad/include" /I "./libs/stb/" ^
 /D_CRT_SECURE_NO_WARNINGS
