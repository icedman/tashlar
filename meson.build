project('ashlar-edit', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14']
)

pwd = meson.current_source_dir()
cc = meson.get_compiler('cpp')

add_global_arguments('-D_XOPEN_SOURCE_EXTENDED=1', language: 'cpp')
add_global_arguments('-DENABLE_UTF8=1', language: 'cpp')
add_global_arguments('-DENABLE_SCRIPTING=1', language: 'cpp')
add_global_arguments('-w', language: 'cpp')

ashlar_files = [
    'src/app.cpp',
    'src/block.cpp',
    'src/completer.cpp',
    'src/cursor.cpp',
    'src/document.cpp',
    'src/dots.cpp',
    'src/editor.cpp',
    'src/explorer.cpp',
    'src/extension.cpp',
    'src/highlighter.cpp',
    'src/indexer.cpp',
    'src/operation.cpp',
    'src/search.cpp',
    'src/snapshot.cpp',
    'src/statusbar.cpp',
    'src/util.cpp'
]

ashlar_view_files = [
    'src/view/view.cpp',
    'src/view/app_view.cpp',
    'src/view/editor_view.cpp',
    'src/view/explorer_view.cpp',
    'src/view/gem_view.cpp',
    'src/view/gutter_view.cpp',
    'src/view/minimap_view.cpp',
    'src/view/popup_view.cpp',
    'src/view/scrollbar_view.cpp',
    'src/view/statusbar_view.cpp',
    'src/view/tabbar_view.cpp',
    'src/view/search_view.cpp',
    'src/view/view_controls.cpp'
]

ashlar_curses_files = [
    'src/view/render.cpp',
    'src/view/keyinput.cpp'
]

ashlar_sdl_files = [
    'src/view/render_sdl.cpp',
    'src/view/keyinput_sdl.cpp',
    'libs/lite/rencache.cpp',
    'libs/lite/renderer.cpp',
    'libs/lite/stb_truetype.cpp'
]

textmate_inc = include_directories(
    'libs/tm-parser/textmate/parser',
    'libs/tm-parser/textmate/theme',
    'libs/tm-parser/textmate/scopes'
)

textmate_files = [
    'libs/tm-parser/textmate/parser/grammar.cpp',
    'libs/tm-parser/textmate/parser/parser.cpp',
    'libs/tm-parser/textmate/parser/pattern.cpp',
    'libs/tm-parser/textmate/parser/reader.cpp',
    'libs/tm-parser/textmate/scopes/match.cpp',
    'libs/tm-parser/textmate/scopes/parse.cpp',
    'libs/tm-parser/textmate/scopes/scope.cpp',
    'libs/tm-parser/textmate/scopes/types.cpp',
    'libs/tm-parser/textmate/theme/theme.cpp',
    'libs/tm-parser/textmate/theme/util.cpp'
]
 
curses_dep = dependency('ncursesw', required: false, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

pthread_dep = cc.find_library('pthread', required : true, disabler: true) 

if get_option('build_sdl')
  sdl2_dep = dependency('SDL2', required: false, disabler: true)
  sdl2ttf_dep = dependency('SDL2_ttf', required: false, disabler: true)  
endif

jsoncpp_dep = dependency('jsoncpp', required: true)
onigmo_dep = dependency('onigmo', required: true)

dl_dep = cc.find_library('dl', required : true) 
quickjs_lib = cc.find_library('quickjs', required: false, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
if build_machine.system() == 'darwin'
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/local/include/quickjs'])
else
    quickjs_dep = declare_dependency(dependencies: [ dl_dep, quickjs_lib ], include_directories: ['/usr/include/quickjs'])
endif

if get_option('build_curses')
executable('ashlr',
    'src/main.cpp',
    ashlar_files,
    ashlar_curses_files,
    ashlar_view_files,
    textmate_files,
    include_directories: [ './src', './src/view', textmate_inc ],
    dependencies: [ quickjs_dep, pthread_dep, onigmo_dep, jsoncpp_dep, curses_dep ],
    install : true
)
endif

if get_option('build_sdl')
executable('ashlr-sdl',
    'src/main.cpp',
    ashlar_files,
    ashlar_sdl_files,
    ashlar_view_files,
    textmate_files,
    include_directories: [ './src', './libs/lite', './src/view', textmate_inc ],
    dependencies: [ quickjs_dep, pthread_dep, onigmo_dep, jsoncpp_dep, sdl2_dep, sdl2ttf_dep ],
    install : true
)
endif

if get_option('build_test')
executable('ashlr-test',
    'src/test.cpp',
    ashlar_files,
    textmate_files,
    include_directories: [ './src', textmate_inc ],
    dependencies: [ quickjs_dep, pthread_dep, onigmo_dep, jsoncpp_dep ],
    install : true
)
endif