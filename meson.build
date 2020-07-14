project('tashlar', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

tashlar_files = [
    'main.cpp',
    'app.cpp',
    'command.cpp',
    'cursor.cpp',
    'document.cpp',
    'dots.cpp',
    'editor.cpp',
    'explorer.cpp',
    'extension.cpp',
    'gutter.cpp',
    'history.cpp',
    'keybinding.cpp',
    'minimap.cpp',
    'popup.cpp',
    'search.cpp',
    'statusbar.cpp',
    'tabbar.cpp',
    'util.cpp',
    'window.cpp'
]

curses_dep = dependency('ncurses', required: true, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('curses', dirs : curses_root, required : false, disabler: true)
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

textmate_inc = include_directories('tm-parser/textmate')
subdir('tm-parser/textmate')
  
executable('tashlar',
    tashlar_files,
    include_directories: [ parser_inc, scopes_inc, theme_inc ],
    dependencies: [ curses_dep, onigmo_dep, jsoncpp_dep, textmate_dep ],
    install : true
)
