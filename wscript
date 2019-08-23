# -*- mode: python -*-

# idx map:
# 500xx libshit
# 510xx boost
# 511xx ljx (host)
# 513xx lua53
# 514xx libc++

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

all_system = set()
all_optional = set()
def options(opt):
    opt.load('compiler_c compiler_cxx')
    grp = opt.get_option_group('configure options')

    grp.add_option('--optimize', action='store_true', default=False,
                   help='Enable some default optimizations')
    grp.add_option('--optimize-ext', action='store_true', default=False,
                   help='Optimize ext libs even if %s is in debug mode' % app.title())
    grp.add_option('--release', action='store_true', default=False,
                   help='Release mode (NDEBUG + optimize)')
    grp.add_option('--with-tests', action='store_true', default=False,
                   help='Enable tests')

    bnd = opt.add_option_group('Bundling options')
    bnd.add_option('--all-system', dest='all_system',
                   action='store_const', const='system',
                   help="Disable bundled libs where it's generally ok")
    bnd.add_option('--all-bundled', dest='all_system',
                   action='store_const', const='bundle',
                   help="Use bundled libs where it's generally ok")

    opt.recurse('ext', name='options')

def configure(cfg):
    from waflib import Logs
    if cfg.options.release:
        cfg.options.optimize = True

    if cfg.options.all_system:
        global all_system
        for s in all_system:
            if getattr(cfg.options, 'system_'+s, None) == None:
                setattr(cfg.options, 'system_'+s, cfg.options.all_system)

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

    cfg.load('clang_compilation_database')
    configure_variant(cfg)

    if cfg.env.DEST_OS == 'win32':
        cfg.env.append_value('CFLAGS', '-gcodeview')
        cfg.env.append_value('CXXFLAGS', '-gcodeview')
        cfg.env.append_value('LINKFLAGS', '-g')
    else:
        cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], ['-ggdb3'])

    if cfg.options.optimize:
        cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], [
            '-Ofast', '-flto', '-fno-fat-lto-objects',
             '-fomit-frame-pointer'])

        if cfg.env.DEST_OS == 'win32':
            cfg.env.append_value('LINKFLAGS', '-Wl,-opt:ref,-opt:icf')
        else:
            cfg.env.append_value('LINKFLAGS', '-Wl,-O1')
    elif cfg.options.optimize_ext:
        cfg.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], ['-Ofast'])

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
        '-Wall', '-Wextra', '-pedantic',
        '-Wno-parentheses', '-Wno-assume', '-Wno-attributes',
        '-Wimplicit-fallthrough', '-Wno-dangling-else', '-Wno-unused-parameter',
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
    ])

    # gcc is a piece of crap: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53431
    if ctx.env.COMPILER_CXX == 'clang++':
        ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], ['-Werror=undef'])
    elif ctx.env.COMPILER_CXX == 'g++':
        # no pedantic: empty semicolons outside functions are valid since 2011...
        ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], ['-Wno-pedantic'])
    ctx.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], [
        '-Wno-parentheses-equality', # boost fs, windows build
        '-Wno-assume', # boost fs
        '-Wno-microsoft-enum-value', '-Wno-shift-count-overflow', # ljx
        '-Wno-varargs',
    ])

    if ctx.check_cxx(cxxflags=['-isystem', '.'],
                     features='cxx', mandatory=False,
                     msg='Checking for compiler flag -isystem'):
        ctx.env.CPPSYSPATH_ST = ['-isystem']
    else:
        ctx.env.CPPSYSPATH_ST = ctx.env.CPPPATH_ST

    ctx.check_cxx(cxxflags='-std=c++17')
    ctx.env.append_value('CXXFLAGS', ['-std=c++17'])

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
        inc = [
            ctx.path.find_node('vita_include').abspath(),
            ctx.path.find_node('ext/libcxx/include').abspath(),
        ]
        # type-limits: char is unsigned, thank you very much
        ctx.env.prepend_value('CXXFLAGS', ['-Wno-type-limits', '-nostdinc++'])
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

def build(bld):
    fixup_rc()
    fixup_fail_cxx()
    bld.recurse('ext', name='build', once=False)

    import re
    rc_ver = re.sub('-.*', '', re.sub('\.', ',', VERSION))
    bld(features   = 'subst',
        source     = '../src/version.hpp.in',
        target     = '../src/version.hpp',
        ext_out    = ['.shit'], # otherwise every fucking c and cpp file will
                                # depend on this task...
        VERSION    = VERSION,
        RC_VERSION = rc_ver)

    build_libshit(bld, '')
    if libshit_cross: bld.only_host_env(build_libshit)

def build_libshit(ctx, pref):
    src = [
        'src/libshit/char_utils.cpp',
        'src/libshit/options.cpp',
        'src/libshit/except.cpp',
        'src/libshit/logger.cpp',
    ]
    if ctx.env.DEST_OS == 'vita':
        src += ['src/libshit/vita_fixup.c']
    if ctx.env.WITH_LUA:
        src += [
            'src/libshit/logger.lua',
            'src/libshit/lua/base.cpp',
            'src/libshit/lua/base_funcs.lua',
            'src/libshit/lua/userdata.cpp',
            'src/libshit/lua/user_type.cpp',
        ]

    if ctx.env.WITH_TESTS:
        src += [
            'test/main.cpp',
            'test/container/ordered_map.cpp',
            'test/container/parent_list.cpp',
        ]
        if ctx.env.WITH_LUA:
            src += [
                'test/lua/function_call.cpp',
                'test/lua/function_ref.cpp',
                'test/lua/type_traits.cpp',
                'test/lua/user_type.cpp',
            ]


    ctx.objects(idx      = 50000 + (len(pref)>0),
                source   = src,
                uselib   = app,
                use      = 'BOOST BRIGAND DOCTEST lua libcxx',
                includes = 'src',
                export_includes = 'src',
                target   = pref+'libshit')


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

# system building helpers
from waflib.Options import OptionsContext
def system_opt(ctx, name, include_all=True, cross=False, has_bundle=True,
               optional=False):
    grp = ctx.get_option_group('Bundling options')
    def x(name):
        sname = 'system_' + name
        grp.add_option('--system-'+name, dest=sname,
                       action='store_const', const='system',
                       help='Use system '+name)
        if has_bundle:
            grp.add_option('--bundled-'+name, dest=sname,
                           action='store_const', const='bundle',
                           help='Use bundled '+name)
        if optional:
            grp.add_option('--disable-'+name, dest=sname,
                           action='store_const', const='disable',
                           help='Disable '+name)
        if include_all:
            global all_system
            all_system.add(name)
        if optional:
            global all_optional
            all_optional.add(name)
    x(name)
    if cross: x(name + '-host')
OptionsContext.system_opt = system_opt

from waflib.Errors import ConfigurationError
from waflib import Utils
@conf
# bundle_chk: if string, header only lib's include path
def system_chk(ctx, name, default, system_chk, bundle_chk, cross=False,
               post_chk=None, define=None):
    def_name = Utils.quote_define_name(name)
    envname = 'BUILD_' + def_name
    hasname = 'HAS_'  + def_name
    defines = 'DEFINES_' + def_name

    global all_optional
    if define == None and name in all_optional:
        define = app + '_WITH_' + def_name

    if bundle_chk == None:
        def fun(ctx):
            pass
        bundle_chk = fun
    if isinstance(bundle_chk, str):
        incl_dir = ctx.path.find_dir(bundle_chk)
        if not incl_dir:
            ctx.fatal('%s/%s not found. Are git submodules missing?' %
                      (ctx.path.abspath(), bundle_chk))
        def fun(ctx):
            ctx.env['SYSTEM_INCLUDES_' + def_name] = incl_dir.abspath()
        bundle_chk = fun

    def x(name):
        opt = getattr(ctx.options, 'system_'+name) or default
        if opt == 'auto' or opt == 'system':
            try:
                system_chk(ctx)
                ctx.env[envname] = False
                ctx.env[hasname] = True
                if define: ctx.env.append_value(defines, define + '=1')
                if post_chk: post_chk(ctx)
                ctx.msg('Using '+name, 'system')
                return
            except ConfigurationError:
                if opt == 'system': raise

        if opt == 'auto' or opt == 'bundle':
            try:
                bundle_chk(ctx)
                ctx.env[envname] = True
                ctx.env[hasname] = True
                if define: ctx.env.append_value(defines, define + '=1')
                if post_chk: post_chk(ctx)
                ctx.msg('Using '+name, 'bundled')
                return
            except ConfigurationError:
                if opt == 'bundle': raise

        ctx.msg('Using '+name, False)
        if define: ctx.env.append_value(defines, define + '=0')
        global all_optional
        if opt == 'disable' or name in all_optional: return
        ctx.fatal('Module '+name+' not found')

    x(name)
    if cross: ctx.only_host_env(lambda bull,shit: x(name+'-host'))


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
