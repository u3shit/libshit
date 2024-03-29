# -*- mode: python -*-

# idx map:
# 500xx libshit
# 510xx boost
# 511xx ljx (host)
# 513xx lua53
# 514xx libc++
# 515xx tracy
# 516xx capnproto

import subprocess
try:
    VERSION = subprocess.check_output(
        ['git', 'describe', '--tags', '--always'],
        stderr = subprocess.PIPE,
        universal_newlines = True).strip('\n').lstrip('v')
except:
    with open('VERSION', 'r') as f:
        VERSION = f.readline().strip('\n')

from waflib.TaskGen import before_method, after_method, feature, extension
def fixup_rc():
    # ignore .rc files when not on windows/no resource compiler
    @extension('.rc')
    def rc_override(self, node):
        if self.env.WINRC:
            from waflib.Tools.winres import rc_file
            rc_file(self, node)

from waflib import Context
try:
    with open('wscript_user.py', 'r') as f:
        exec(compile(f.read(), 'wscript_user.py', 'exec'), Context.g_module.__dict__)
except IOError:
    pass

if not Context.g_module:
    APPNAME = 'libshit'
    try: did_I_mention_that_python_is_a_horrible_language()
    except:
        class AttributeDict(dict):
            def __init__(self, *args, **kwargs):
                super(AttributeDict, self).__init__(*args, **kwargs)
                self.__dict__ = self
        import sys
        Context.g_module = AttributeDict(sys.exc_info()[2].tb_frame.f_globals)

from waflib.Tools import c_config
c_config.MACRO_TO_DESTOS['__vita__'] = 'vita'

app = Context.g_module.APPNAME.upper()
libshit_cross = getattr(Context.g_module, 'LIBSHIT_CROSS', False)

def options(opt):
    opt.load('with_selector', tooldir='.')
    opt.load('compiler_c compiler_cxx clang_compilation_database md5_tstamp')
    grp = opt.get_option_group('configure options')

    grp.add_option('--optimize', action='store_true', default=False,
                   help='Enable some default optimizations')
    grp.add_option('--optimize-ext', action='store_true', default=False,
                   help='Optimize ext libs even if %s is in debug mode' % app.title())
    grp.add_option('--release', action='store_true', default=False,
                   help='Release mode (NDEBUG + optimize)')
    grp.add_option('--with-tests', action='store_true', default=False,
                   help='Enable tests')

    opt.recurse('ext', name='options')

def configure(cfg):
    from waflib import Logs
    if cfg.options.release:
        cfg.options.optimize = True

    cfg.load('with_selector', tooldir='.')

    variant = cfg.variant
    environ = cfg.environ
    # setup host compiler
    cfg.setenv(variant + '_host')
    Logs.pprint('NORMAL', 'Configuring host compiler '+variant)

    cross = False
    # replace xxx with HOST_xxx env vars
    cfg.environ = environ.copy()
    for k in list(cfg.environ):
        if k[0:5] == 'HOST_':
            cross = True
            cfg.environ[k[5:]] = cfg.environ[k]
            del cfg.environ[k]

    if cross:
        configure_variant(cfg)

        if cfg.options.optimize or cfg.options.optimize_ext:
            # host executables on cross compile, so -march=native is ok
            cfg.filter_flags(['CFLAGS', 'CXXFLAGS'], ['-O1', '-march=native'])

    # ----------------------------------------------------------------------
    # setup target
    cfg.setenv(variant)
    Logs.pprint('NORMAL', 'Configuring target compiler '+variant)
    cfg.env.CROSS = cross
    cfg.environ = environ

    configure_variant(cfg)

    if cfg.env.DEST_OS == 'win32':
        cfg.env.append_value('CFLAGS', '-gcodeview')
        cfg.env.append_value('CXXFLAGS', '-gcodeview')
        cfg.env.append_value('LINKFLAGS', '-g')
    else:
        cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], ['-ggdb3'])

    if cfg.options.optimize:
        cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], [
            '-O3', '-flto', '-fno-fat-lto-objects',
             '-fomit-frame-pointer'])

        if cfg.env.DEST_OS == 'win32':
            cfg.env.append_value('LINKFLAGS', '-Wl,-opt:ref,-opt:icf')
        else:
            cfg.env.append_value('LINKFLAGS', '-Wl,-O1')
    elif cfg.options.optimize_ext:
        cfg.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], ['-O3'])

    if cfg.options.release:
        cfg.define('NDEBUG', 1)
    cfg.env.WITH_TESTS = cfg.options.with_tests

    Logs.pprint('NORMAL', 'Configuring ext '+variant)
    cfg.recurse('ext', name='configure', once=False)

def configure_variant(ctx):
    ctx.add_os_flags('ASFLAGS', dup=False)
    # override flags specific to app/bundled libraries
    for v in [app, 'EXT']:
        ctx.add_os_flags('ASFLAGS_'+v, dup=False)
        ctx.add_os_flags('CPPFLAGS_'+v, dup=False)
        ctx.add_os_flags('CFLAGS_'+v, dup=False)
        ctx.add_os_flags('CXXFLAGS_'+v, dup=False)
        ctx.add_os_flags('LINKFLAGS_'+v, dup=False)
        ctx.add_os_flags('LDFLAGS_'+v, dup=False)

    ctx.load('compiler_c compiler_cxx')

    ctx.filter_flags(['CFLAGS', 'CXXFLAGS'], [
        # error on unknown arguments, including unknown options that turns
        # unknown argument warnings into error. どうして？
        '-Werror=unknown-warning-option',
        '-Werror=ignored-optimization-argument',
        '-Werror=unknown-argument',
    ], seq=False)
    ctx.filter_flags(['CFLAGS', 'CXXFLAGS'], [
        '-fdiagnostics-color', '-fdiagnostics-show-option',
        '-fdata-sections', '-ffunction-sections',
    ])
    if ctx.env.DEST_OS != 'win32' and \
       ctx.check_cxx(linkflags='-Wl,--gc-sections', mandatory=False):
        ctx.env.append_value('LINKFLAGS', ['-Wl,--gc-sections'])

    ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], [
        '-Wall', '-Wextra', '-pedantic', '-Wdouble-promotion',
        '-Wno-parentheses', '-Wno-assume', '-Wno-attributes',
        '-Wimplicit-fallthrough', '-Wno-dangling-else', '-Wno-unused-parameter',
        # I don't even know what this warning supposed to mean, gcc. how can you
        # not set a parameter?
        '-Wno-unused-but-set-parameter',
        # __try in lua exception handler
        '-Wno-language-extension-token',
    ])
    # c++ only warnings, shut up gcc
    ctx.filter_flags(['CXXFLAGS_'+app], [
        '-Wold-style-cast', '-Woverloaded-virtual',
        '-Wno-undefined-var-template', # TYPE_NAME usage
        # -Wsubobject-linkage: warning message is criminally bad, basically it
        # complains about defining/using something from anonymous namespace in a
        # header which normally makes sense, except in "*.binding.hpp"s.
        # unfortunately pragma disabling this warning in the binding files does
        # not work. see also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51440
        '-Wno-subobject-linkage', '-Wno-sign-compare',
        '-Wno-extra-semi', # warns on valid ; in structs
    ])

    # gcc is a piece of crap: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
    # missing-prototypes/missing-declarations: of course, clang and gcc flags
    # mean different things, and gcc yells when -Wmissing-prototypes is used in
    # c++
    if ctx.env.COMPILER_CXX == 'clang++':
        ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], [
            '-Werror=undef', '-Wmissing-prototypes',
        ])
    elif ctx.env.COMPILER_CXX == 'g++':
        ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], [
            '-Wno-pedantic', # empty semicolons outside functions are valid since 2011...
            '-Wno-unknown-pragmas' # do not choke on #pragma clang
        ])
        ctx.filter_flags(['CXXFLAGS_'+app], ['-Wmissing-declarations'])

    ctx.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], [
        '-Wno-parentheses-equality', # boost fs, windows build
        '-Wno-assume', # boost fs
        '-Wno-microsoft-enum-value', '-Wno-shift-count-overflow', # ljx
        '-Wno-varargs',
        '-Wno-string-plus-int', # lua53
        '-Wno-deprecated-declarations', '-Wno-ignored-qualifiers', # capnp
    ])

    if ctx.check_cxx(cxxflags=['-isystem', '.'],
                     features='cxx', mandatory=False,
                     msg='Checking for compiler flag -isystem'):
        ctx.env.CPPSYSPATH_ST = ['-isystem']
    else:
        ctx.env.CPPSYSPATH_ST = ctx.env.CPPPATH_ST

    ctx.check_cxx(cxxflags='-std=c++17')
    ctx.env.append_value('CXXFLAGS', ['-std=c++17'])

    if ctx.options.release:
        # only hide symbols in release builds, this creates better stacktraces
        # in tools that doesn't support DWARF debug info
        ctx.filter_flags(['CFLAGS', 'CXXFLAGS'], ['-fvisibility=hidden'])

    if ctx.env.DEST_OS == 'win32':
        # fixup: waf expects mingw, not clang in half-msvc-emulation-mode
        while '-Wl,--enable-auto-import' in ctx.env.LINKFLAGS:
            ctx.env.LINKFLAGS.remove('-Wl,--enable-auto-import')
        ctx.env.IMPLIB_ST = '-Wl,-implib:%s'
        ctx.env.implib_PATTERN = '%s.lib'
        ctx.env.cstlib_PATTERN = ctx.env.cxxstlib_PATTERN = '%s.lib'
        ctx.env.def_PATTERN = '-Wl,-def:%s'
        ctx.env.STLIB_MARKER = ''
        ctx.env.SHLIB_MARKER = ''

        for x in ['CXXFLAGS', 'CFLAGS_EXT']:
            ctx.env.append_value(x, ['-fexceptions']) # -EHsa
        ctx.env.append_value('LINKFLAGS', [
            '-nostdlib', '-Wl,-defaultlib:msvcrt', # -MD cont
            '-Wl,-defaultlib:oldnames', # lua isatty
        ])
        ctx.define('_MT', 1)
        ctx.define('_DLL', 1)

        ctx.load('winres')
        ctx.add_os_flags('WINRCFLAGS')
        ctx.define('_CRT_SECURE_NO_WARNINGS', 1)
        ctx.define('UNICODE', 1)
        ctx.define('_UNICODE', 1)

        ctx.check_cxx(lib='advapi32')
        ctx.check_cxx(lib='kernel32')
        ctx.check_cxx(lib='shell32')
        ctx.check_cxx(lib='user32')

        m = ctx.get_defs('msvc lib version', '#include <yvals.h>', cxx=True)
        cpp_ver = m['_CPPLIB_VER']
        if cpp_ver == '610':
            ctx.end_msg('610, activating include patching', color='YELLOW')
            inc = [ctx.path.find_node('msvc_include').abspath()]
            ctx.env.prepend_value('SYSTEM_INCLUDES', inc)
        else:
            ctx.end_msg(cpp_ver)
    elif ctx.env.DEST_OS == 'vita':
        inc = [ ctx.path.find_node('vita_include').abspath() ]
        # type-limits: char is unsigned, thank you very much
        ctx.env.prepend_value('CXXFLAGS', ['-Wno-type-limits'])
        ctx.env.prepend_value('SYSTEM_INCLUDES', inc)
        ctx.env.append_value('DEFINES_BOOST', ['BOOST_HAS_STDINT_H'])
        ldflags = [
            '-Wl,-q,-z,nocopyreloc','-nostdlib', '-lsupc++',
            '-Wl,--start-group', '-lgcc', '-lSceLibc_stub', '-lpthread',
            # linked automatically by vitasdk gcc when not using -nostdlib
            # we don't need some of them, but unused libraries don't end
            # up in the final executable, so it doesn't hurt
            '-lSceRtc_stub', '-lSceSysmem_stub', '-lSceKernelThreadMgr_stub',
            '-lSceKernelModulemgr_stub', '-lSceIofilemgr_stub',
            '-lSceProcessmgr_stub', '-lSceLibKernel_stub', '-lSceNet_stub',
            '-Wl,--end-group'
        ]
        ctx.env.append_value('LDFLAGS', ldflags)
    else:
        ctx.env.append_value('LINKFLAGS', '-rdynamic')
        # needed by debug stacktrace
        ctx.check_cxx(lib='dl')

def build(bld):
    fixup_rc()
    fixup_fail_cxx()
    bld.recurse('ext', name='build', once=False)

    build_libshit(bld, '')
    if libshit_cross: bld.only_host_env(build_libshit)

def build_libshit(ctx, pref):
    ctx.objects(idx      = 50004 + (len(pref)>0),
                source   = ['src/libshit/except.cpp'],
                uselib   = ['LIBBACKTRACE', app],
                use      = ['BOOST', 'DOCTEST', 'DL', 'BACKTRACE', 'LUA',
                            pref+'lua'],
                includes = 'src',
                export_includes = 'src',
                target   = pref+'libshit-except')

    src = [
        'src/libshit/logger.cpp',
        'src/libshit/low_io.cpp',
        'src/libshit/options.cpp',
        'src/libshit/random.cpp',
        'src/libshit/string_utils.cpp',
        'src/libshit/wtf8.cpp',
    ]
    if ctx.env.DEST_OS == 'vita':
        src += ['src/libshit/vita_fixup.c']
    if ctx.env.WITH_LUA != 'none':
        src += [
            'src/libshit/logger.lua',
            'src/libshit/lua/base.cpp',
            'src/libshit/lua/base_funcs.lua',
            'src/libshit/lua/lua53_polyfill.lua',
            'src/libshit/lua/user_type.cpp',
            'src/libshit/lua/userdata.cpp',
        ]
    if ctx.env.WITH_TRACY != 'none':
        src += [ 'src/libshit/tracy_alloc.cpp' ]

    if ctx.env.WITH_TESTS:
        src += [
            'test/abomination.cpp',
            'test/container/ordered_map.cpp',
            'test/container/parent_list.cpp',
            'test/container/simple_vector.cpp',
            'test/nonowning_string.cpp',
            'test/test_helper.cpp',
        ]
        if ctx.env.WITH_LUA != 'none':
            src += [
                'test/lua/function_call.cpp',
                'test/lua/function_ref.cpp',
                'test/lua/type_traits.cpp',
                'test/lua/user_type.cpp',
            ]


    ctx.objects(idx      = 50000 + (len(pref)>0),
                source   = src,
                uselib   = app,
                use      = ['TRACY',
                            'ADVAPI32', # windows random
                            pref+'tracy', pref+'libshit-except'],
                target   = pref+'libshit')

    if ctx.env.WITH_TESTS and app == 'LIBSHIT':
        ctx.program(idx    = 50002 + (len(pref)>0),
                    source = 'test/libshit_standalone.cpp',
                    use    = pref+'libshit',
                    target = pref+'libshit-tests')


################################################################################
# random utilities

def fixup_fail_cxx():
    # feature fail_cxx: expect compilation failure
    import sys
    from waflib import Logs
    from waflib.Task import Task
    from waflib.Tools import c_preproc
    @extension('.cpp')
    def fail_cxx_ext(self, node):
        if 'fail_cxx' in self.features:
            return self.create_compiled_task('fail_cxx', node)
        else:
            return self.create_compiled_task('cxx', node)

    class fail_cxx(Task):
        #run_str = '${CXX} ${ARCH_ST:ARCH} ${CXXFLAGS} ${CPPFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ${CXX_SRC_F}${SRC} ${CXX_TGT_F}${TGT[0].abspath()}'
        def run(tsk):
            env = tsk.env
            gen = tsk.generator
            bld = gen.bld
            cwdx = getattr(bld, 'cwdx', bld.bldnode) # TODO single cwd value in waf 1.9
            wd = getattr(tsk, 'cwd', None)
            def to_list(xx):
                if isinstance(xx, str): return [xx]
                return xx
            tsk.last_cmd = lst = []
            lst.extend(to_list(env['CXX']))
            lst.extend(tsk.colon('ARCH_ST', 'ARCH'))
            lst.extend(to_list(env['CXXFLAGS']))
            lst.extend(to_list(env['CPPFLAGS']))
            lst.extend(tsk.colon('FRAMEWORKPATH_ST', 'FRAMEWORKPATH'))
            lst.extend(tsk.colon('CPPPATH_ST', 'INCPATHS'))
            lst.extend(tsk.colon('DEFINES_ST', 'DEFINES'))
            lst.extend(to_list(env['CXX_SRC_F']))
            lst.extend([a.path_from(cwdx) for a in tsk.inputs])
            lst.extend(to_list(env['CXX_TGT_F']))
            lst.append(tsk.outputs[0].abspath())
            lst = [x for x in lst if x]
            try:
                (out,err) = bld.cmd_and_log(
                    lst, cwd=bld.variant_dir, env=env.env or None,
                    quiet=0, output=0)
                Logs.error("%s compiled successfully, but it shouldn't" % tsk.inputs[0])
                Logs.info(out, extra={'stream':sys.stdout, 'c1': ''})
                Logs.info(err, extra={'stream':sys.stderr, 'c1': ''})
                return -1
            except Exception as e:
                # create output to silence missing file errors
                open(tsk.outputs[0].abspath(), 'w').close()
                return 0

        vars    = ['CXXDEPS'] # unused variable to depend on, just in case
        ext_in  = ['.h'] # set the build order easily by using ext_out=['.h']
        scan    = c_preproc.scan


from waflib.Configure import conf
def filter_single_flag(ctx, vars, flag, feat, ret, **kw):
    testflag=flag
    if flag[0:5] == '-Wno-':
        testflag = '-W'+flag[5:]
    ctx.check_cxx(cflags=[testflag], cxxflags=[testflag], features=feat,
                  msg='Checking for compiler flag '+flag, **kw)

    ret.append(flag)

def generic_filter_flags(ctx, vars, flags, feat, seq=False):
    ret = []

    if len(flags) == 1:
        filter_single_flag(ctx, vars, flags[0], feat, ret, mandatory=False)
    elif seq:
        for flag in flags:
            filter_single_flag(ctx, vars, flag, feat, ret, mandatory=False)
    elif len(flags) > 1:
        args = []
        for flag in flags:
            def x(ctx, flag=flag):
                ctx.in_msg = True
                return filter_single_flag(ctx, vars, flag, feat, ret)
            args.append({'func': x, 'msg': 'Checking for compiler flag '+flag,
                         'mandatory': False})
        ctx.multicheck(*args)

    for flag in flags:
        if flag in ret:
            for var in vars:
                ctx.env.append_value(var, flag)
    return ret

@conf
def filter_flags(ctx, vars, flags, **kw):
    return generic_filter_flags(ctx, vars, flags, 'cxx', **kw)

@conf
def filter_flags_c(ctx, vars, flags):
    return generic_filter_flags(ctx, vars, flags, 'c', **kw)

def change_var(ctx, to):
    if getattr(ctx, 'setenv', False):
        ctx.setenv(to)
    else:
        ctx.variant = to
        ctx.all_envs[to].derive()

@conf
def only_host_env(ctx, f):
    if not ctx.env.CROSS: return
    variant = ctx.variant
    change_var(ctx, variant + '_host')
    f(ctx, 'host/')
    change_var(ctx, variant)

@conf
def in_host_env(ctx, f):
    if ctx.env.CROSS:
        variant = ctx.variant
        change_var(ctx, variant + '_host')
        f(ctx, 'host/')
        change_var(ctx, variant)
    else:
        f(ctx, '')

@conf
def both_host_env(ctx, f):
    f(ctx, '')
    if ctx.env.CROSS:
        variant = ctx.variant
        change_var(ctx, variant + '_host')
        f(ctx, 'host/')
        change_var(ctx, variant)

# waf 1.7 style logging: c/cxx instead of vague shit like 'processing',
# 'compiling', etc.
from waflib.Task import Task
def getname(self):
    return self.__class__.__name__ + ':'
Task.keyword = getname


@feature('c', 'cxx', 'includes')
@after_method('propagate_uselib_vars', 'process_source', 'apply_incpaths')
def apply_sysincpaths(self):
    lst = self.to_incnodes(self.to_list(getattr(self, 'system_includes', [])) +
                           self.env['SYSTEM_INCLUDES'])
    self.includes_nodes += lst
    self.env['SYSINCPATHS'] = [x.abspath() for x in lst]

from waflib.Tools import c, cxx
from waflib.Task import compile_fun
from waflib import Utils
from waflib.Tools.ccroot import USELIB_VARS
for cls in [c.c, cxx.cxx]:
    run_str = cls.orig_run_str.replace('INCPATHS', 'INCPATHS} ${CPPSYSPATH_ST:SYSINCPATHS')
    (f, dvars) = compile_fun(run_str, cls.shell)
    cls.hcode = Utils.h_cmd(run_str)
    cls.run = f
    cls.vars = list(set(cls.vars + dvars))
    cls.vars.sort()
USELIB_VARS['c'].add('SYSTEM_INCLUDES')
USELIB_VARS['cxx'].add('SYSTEM_INCLUDES')
USELIB_VARS['includes'].add('SYSTEM_INCLUDES')

# reusable rule-like tasks
@conf
def rule_like(ctx, name):
    def x(self):
        self.meths.remove('process_source')
        tsk = self.create_task(name)
        self.task = tsk
        tsk.inputs = self.to_nodes(self.source)

        # from TaskGen.process_rule
        if isinstance(self.target, str):
            self.target = self.target.split()
        if not isinstance(self.target, list):
            self.target = [self.target]
        for x in self.target:
            if isinstance(x, str):
                tsk.outputs.append(self.path.find_or_declare(x))
            else:
                x.parent.mkdir() # if a node was given, create the required folders
                tsk.outputs.append(x)

    x.__name__ = 'rule_like_%s' % name
    feature(name)(x)
    before_method('process_source')(x)

# get preprocessor defs
import re
defs_re = re.compile('^#define ([^ ]+) (.*)$')
@conf
def get_defs(ctx, msg, file, cxx=False):
    ctx.start_msg('Checking for '+msg)
    cmd = [] # python, why u no have Array#flatten?
    cmd += cxx and ctx.env.CXX or ctx.env.CC
    cmd += ctx.env.CPPFLAGS
    cmd += cxx and ctx.env.CXXFLAGS or ctx.env.CFLAGS
    if isinstance(file, str):
        node = ctx.bldnode.make_node(cxx and 'test.cpp' or 'test.c')
        node.write(file)
        file = node

    cmd += ['-E', file.abspath(), '-dM' ]
    try:
        out = ctx.cmd_and_log(cmd)
    except Exception:
        ctx.end_msg(False)
        raise

    return dict(map(lambda l: defs_re.match(l).groups(), out.splitlines()))

# random build helpers
@conf
def gen_version_hpp(bld, name):
    import re
    lst = list(filter(lambda x: re.match('^[0-9]+$', x), re.split('[\.-]', VERSION)))
    # prevent parsing a git shortrev as a number
    if len(lst) < 2: lst = []
    rc_ver = ','.join((lst + ['0']*4)[0:4])
    bld(features   = 'subst',
        source     = name + '.in',
        target     = name,
        ext_out    = ['.shit'], # otherwise every fucking c and cpp file will
                                # depend on this task...
        VERSION    = VERSION,
        RC_VERSION = rc_ver)
