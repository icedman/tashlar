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
    'src/item.cpp',
    'src/minimap.cpp',
    'src/popup.cpp',
    'src/search.cpp',
    'src/scripting.cpp',
    'src/scrollbar.cpp',
    'src/snapshot.cpp',
    'src/tabbar.cpp',
    'src/statusbar.cpp',
    'src/util.cpp',
    'src/view.cpp',
    'src/keybinding.cpp',
    'src/render/editor.cpp',
    'src/render/explorer.cpp',
    'src/render/gutter.cpp',
    'src/render/minimap.cpp',
    'src/render/popup.cpp',
    'src/render/tabbar.cpp',
    'src/render/statusbar.cpp',
]

tashlar_sdl_files = [
    'src/render/render_sdl.cpp',
    'src/render/keyinput_sdl.cpp',
    'libs/lite/rencache.cpp',
    'libs/lite/renderer.cpp',
    'libs/lite/stb_truetype.cpp'
]

tashlar_curses_files = [
    'src/render/render.cpp',
    'src/render/keyinput.cpp'
]

textmate_inc = include_directories(
    'libs/tm-parser/textmate/parser',
    'libs/tm-parser/textmate/theme',
    'libs/tm-parser/textmate/scopes'
)

textmate_files = [
    'libs/tm-parser/textmate/parser/grammar.cpp',
    'libs/tm-parser/textmate/parser/reader.cpp',
    'libs/tm-parser/textmate/parser/pattern.cpp',
    'libs/tm-parser/textmate/parser/parser.cpp',
    'libs/tm-parser/textmate/scopes/scope.cpp',
    'libs/tm-parser/textmate/scopes/types.cpp',
    'libs/tm-parser/textmate/scopes/parse.cpp',
    'libs/tm-parser/textmate/scopes/match.cpp',
    'libs/tm-parser/textmate/theme/theme.cpp',
    'libs/tm-parser/textmate/theme/util.cpp'
]
 
curses_dep = dependency('ncursesw', required: true, disabler: true)
if not curses_dep.found()
  curses_root = get_option('curses_root')
  curses_lib = cc.find_library('cursesw', dirs : curses_root, required : false, disabler: true)
  curses_dep = declare_dependency(include_directories: curses_root, dependencies: curses_lib)
endif

dl_dep = cc.find_library('dl', required : true) 
pthread_dep = cc.find_library('pthread', required : true) 

sdl2_lib = cc.find_library('SDL2', required: true)
sdl2_dep = declare_dependency(dependencies: [ sdl2_lib ])

sdl2ttf_lib = cc.find_library('SDL2_ttf', required: true)
sdl2ttf_dep = declare_dependency(dependencies: [ sdl2ttf_lib ])
   
jsoncpp_lib = cc.find_library('jsoncpp', required: true)
jsoncpp_dep = declare_dependency(dependencies: [ jsoncpp_lib ])

onigmo_lib = cc.find_library('onigmo', required: true)
onigmo_dep = declare_dependency(dependencies: [ onigmo_lib ])

quickjs_lib = cc.find_library('quickjs', required: true, dirs:['/usr/lib/quickjs', '/usr/local/lib/quickjs'])
if build_machine.system() == 'darwin'
    quickjs_dep = declare_dependency(dependencies: [ quickjs_lib ], include_directories: ['/usr/local/include/quickjs'])
else
    quickjs_dep = declare_dependency(dependencies: [ quickjs_lib ], include_directories: ['/usr/include/quickjs'])
endif
  
executable('ashlr',
    tashlar_files,
    tashlar_curses_files,
    textmate_files,
    include_directories: [ './src', './src/render', textmate_inc ],
    dependencies: [ quickjs_dep, dl_dep, pthread_dep, curses_dep, onigmo_dep, jsoncpp_dep ],
    install : true
)

executable('ashlar',
    tashlar_files,
    tashlar_sdl_files,
    textmate_files,
    include_directories: [ './src', './src/render', './libs/lite', textmate_inc ],
    dependencies: [ quickjs_dep, dl_dep, pthread_dep, sdl2_dep, sdl2ttf_dep, curses_dep, onigmo_dep, jsoncpp_dep ],
    install : true
)
