project('tashlar', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

add_global_arguments('-D_XOPEN_SOURCE_EXTENDED=1', language: 'cpp')

tashlar_files = [
    'src/main.cpp',
    'src/app.cpp',
    'src/block.cpp',
    'src/command.cpp',
    'src/cursor.cpp',
    'src/document.cpp',
    'src/dots.cpp',
    'src/editor.cpp',
    'src/explorer.cpp',
    'src/extension.cpp',
    'src/gutter.cpp',
    'src/history.cpp',
    'src/keybinding.cpp',
    'src/minimap.cpp',
    'src/popup.cpp',
    'src/search.cpp',
    'src/statusbar.cpp',
    'src/scripting.cpp',
    'src/tabbar.cpp',
    'src/util.cpp',
    'src/window.cpp',
    'src/renderer/curses.cpp'
]

curses_dep = dependency('ncursesw', required: true, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

jsoncpp_proj = subproject('jsoncpp')
jsoncpp_dep = jsoncpp_proj.get_variable('jsoncpp_dep')
  
onigmo_inc = include_directories('tm-parser/Onigmo')
onigmo_lib = cc.find_library('libonigmo', dirs:[pwd + '/libs'])
onigmo_dep = declare_dependency(
    dependencies: onigmo_lib,
    include_directories : onigmo_inc
)

lua_inc = include_directories('lua-5.4.0/src')
lua_lib = cc.find_library('liblua', dirs:[pwd + '/libs'])
lua_dep = declare_dependency(
    dependencies: lua_lib,
    include_directories : lua_inc
)

qjs_inc = include_directories('quickjs-2020-07-05')
qjs_lib = cc.find_library('libquickjs', dirs:[pwd + '/libs'])
qjs_dep = declare_dependency(
    dependencies: qjs_lib,
    include_directories : qjs_inc
)

dl_dep = cc.find_library('dl', required : true) 
pthread_dep = cc.find_library('pthread', required : true) 

textmate_inc = include_directories('tm-parser/textmate')
subdir('tm-parser/textmate')
  
executable('tashlar',
    tashlar_files,
    include_directories: [ './src', parser_inc, scopes_inc, theme_inc ],
    dependencies: [ qjs_dep, dl_dep, pthread_dep, curses_dep, onigmo_dep, jsoncpp_dep, textmate_dep ],
    install : true
)
