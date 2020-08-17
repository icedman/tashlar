project('tashlar', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

add_global_arguments('-D_XOPEN_SOURCE_EXTENDED=1', language: 'cpp')
add_global_arguments('-DENABLE_UTF8=1', language: 'cpp')

tashlar_files = [
    'src/main.cpp',
    'src/app.cpp',
    'src/block.cpp',
    'src/cursor.cpp',
    'src/completer.cpp',
    'src/document.cpp',
    'src/dots.cpp',
    'src/editor.cpp',
    'src/explorer.cpp',
    'src/extension.cpp',
    'src/gem.cpp',
    'src/gutter.cpp',
    'src/highlighter.cpp',
    'src/minimap.cpp',
    'src/popup.cpp',
    'src/search.cpp',
    'src/scripting.cpp',
    'src/snapshot.cpp',
    'src/tabbar.cpp',
    'src/statusbar.cpp',
    'src/util.cpp',
    'src/view.cpp',
    'src/keybinding.cpp',
    'src/keyinput.cpp',
    'src/render/editor.cpp',
    'src/render/explorer.cpp',
    'src/render/gutter.cpp',
    'src/render/minimap.cpp',
    'src/render/popup.cpp',
    'src/render/tabbar.cpp',
    'src/render/statusbar.cpp',
    'src/render/render_sdl.cpp',
    'libs/lite/rencache.cpp',
    'libs/lite/renderer.cpp',
    'libs/lite/stb_truetype.cpp',
]

curses_dep = dependency('ncursesw', required: true, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

onigmo_inc = include_directories('libs/Onigmo')
onigmo_lib = cc.find_library('libonigmo', dirs:[pwd + '/libs'])
onigmo_dep = declare_dependency(
    dependencies: onigmo_lib,
    include_directories : onigmo_inc
)

qjs_inc = include_directories('libs/quickjs-2020-07-05')
qjs_lib = cc.find_library('libquickjs', dirs:[pwd + '/libs'])
qjs_dep = declare_dependency(
    dependencies: qjs_lib,
    include_directories : qjs_inc
)

dl_dep = cc.find_library('dl', required : true) 
pthread_dep = cc.find_library('pthread', required : true) 

sdl2_lib = cc.find_library('SDL2')
sdl2_dep = declare_dependency(dependencies: [ sdl2_lib ])
# sdl2gfx_lib = cc.find_library('SDL2_gfx')
# sdl2gfx_dep = declare_dependency(dependencies: [ sdl2gfx_lib ])
# sdl2ttf_lib = cc.find_library('SDL2_ttf')
# sdl2ttf_dep = declare_dependency(dependencies: [ sdl2ttf_lib ])
# sdl2image_lib  = cc.find_library('SDL2_image')
# sdl2image_dep  = declare_dependency(dependencies: [ sdl2image_lib ])
    
jsoncpp_proj = subproject('jsoncpp-1.8.4')
jsoncpp_dep = jsoncpp_proj.get_variable('jsoncpp_dep')

textmate_inc = include_directories('libs/tm-parser/textmate')
subdir('libs/tm-parser/textmate')
    
executable('tashlar',
    tashlar_files,
    include_directories: [ './src', './src/render', './libs/lite', parser_inc, scopes_inc, theme_inc ],
    dependencies: [ qjs_dep, dl_dep, pthread_dep, sdl2_dep, curses_dep, onigmo_dep, jsoncpp_dep, textmate_dep ],
    install : true
)
