project(
    'textmate-parser',
    ['cpp'],
    version : '0.1',
    meson_version: '>=0.43.0',
    default_options : [
        'warning_level=3', 
        'cpp_std=c++14'
    ]
)

add_project_arguments(
    [
        '-Wno-unused-parameter',
        '-Wno-pedantic',
        '-Wno-sign-compare',
        '-Wno-write-strings'
    ],
    language: ['cpp']
)

jsoncpp_proj = subproject('jsoncpp')
jsoncpp_dep = jsoncpp_proj.get_variable('jsoncpp_dep')

cmplr = meson.get_compiler('cpp')

pwd = meson.current_source_dir()

onigmo_inc = include_directories('Onigmo')
onigmo_lib = cmplr.find_library('libonigmo', dirs:[pwd + '/Onigmo/.libs'])
onigmo_dep = declare_dependency(
    dependencies: onigmo_lib,
    include_directories : onigmo_inc
)

textmate_inc = include_directories('textmate')

subdir('textmate')
subdir('tests')
subdir('tests-c')