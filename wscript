# -*- mode: python -*-

# idx map:
# 500xx libshit
# 510xx boost
# 511xx ljx (host)
# 513xx lua53

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
def fixup_msvc():
    @after_method('apply_link')
    @feature('c', 'cxx')
    def apply_flags_msvc(self):
        pass

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

app = Context.g_module.APPNAME.upper()
libshit_cross = getattr(Context.g_module, 'LIBSHIT_CROSS', False)

all_system = set()
all_optional = set()
def options(opt):
    opt.load('compiler_c compiler_cxx')
    grp = opt.get_option_group('configure options')
    grp.add_option('--clang-hack', action='store_true', default=False,
                   help='Read COMPILE.md...')
    grp.add_option('--clang-hack-host', action='store_true', default=False,
                   help='Read COMPILE.md...')
    grp.add_option('--optimize', action='store_true', default=False,
                   help='Enable some default optimizations')
    grp.add_option('--optimize-ext', action='store_true', default=False,
                   help='Optimize ext libs even if %s is in debug mode' % app.title())
    grp.add_option('--release', action='store_true', default=False,
                   help='Enable some flags for release builds')
    bnd = opt.add_option_group('Bundling options')
    bnd.add_option('--all-system', dest='all_system',
                   action='store_const', const='system',
                   help="Disable bundled libs where it's generally ok")
    bnd.add_option('--all-bundled', dest='all_system',
                   action='store_const', const='bundle',
                   help="Use bundled libs where it's generally ok")

    opt.add_option('--skip-run-tests', action='store_true', default=False,
                   help="Skip actually running tests with `test'")
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
        configure_variant(cfg, cfg.options.clang_hack_host)

        if cfg.options.optimize or cfg.options.optimize_ext:
            if cfg.env['COMPILER_CXX'] == 'msvc':
                cfg.filter_flags(['CFLAGS', 'CXXFLAGS'], ['-O1'])
            else:
                cfg.filter_flags(
                    ['CFLAGS', 'CXXFLAGS'], ['-O1', '-march=native'])

    # ----------------------------------------------------------------------
    # setup target
    cfg.setenv(variant)
    Logs.pprint('NORMAL', 'Configuring target compiler '+variant)
    cfg.env.CROSS = cross
    cfg.environ = environ

    cfg.load('clang_compilation_database')
    configure_variant(cfg, cfg.options.clang_hack)

    if cfg.env['COMPILER_CXX'] == 'msvc':
        if cfg.options.optimize:
            for f in ['CFLAGS', 'CXXFLAGS']:
                cfg.env.prepend_value(f, ['-O2', '-flto'])
            cfg.env.prepend_value('LINKFLAGS', ['/OPT:REF', '/OPT:ICF'])
        elif cfg.options.optimize_ext:
            cfg.env.prepend_value('CXXFLAGS_EXT', '-O2')
    else:
        if cfg.options.optimize:
            cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], [
                '-Ofast', '-flto', '-fno-fat-lto-objects',
                 '-fomit-frame-pointer'])

            cfg.env.append_value('LINKFLAGS', '-Wl,-O1')
        elif cfg.options.optimize_ext:
            cfg.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], ['-g0', '-Ofast'])

    if cfg.options.release:
        cfg.define('NDEBUG', 1)

    Logs.pprint('NORMAL', 'Configuring ext '+variant)
    cfg.recurse('ext', name='configure', once=False)

def configure_variant(ctx, clang_hack):
    # override flags specific to app/bundled libraries
    for v in [app, 'EXT']:
        ctx.add_os_flags('CPPFLAGS_'+v, dup=False)
        ctx.add_os_flags('CFLAGS_'+v, dup=False)
        ctx.add_os_flags('CXXFLAGS_'+v, dup=False)
        ctx.add_os_flags('LINKFLAGS_'+v, dup=False)
        ctx.add_os_flags('LDFLAGS_'+v, dup=False)

    if clang_hack:
        ctx.find_program('clang-cl', var='CC')
        ctx.find_program('clang-cl', var='CXX')
        ctx.find_program('lld-link', var='LINK_CXX')
        ctx.find_program('llvm-lib', var='AR')
        ctx.find_program(ctx.path.abspath()+'/rc.sh', var='WINRC')

        ctx.add_os_flags('WINRCFLAGS', dup=False)
        rcflags_save = ctx.env.WINRCFLAGS

        ctx.load('msvc', funs='no_autodetect')
        fixup_msvc()
        from waflib.Tools.compiler_cxx import cxx_compiler
        from waflib.Tools.compiler_c import c_compiler
        from waflib import Utils

        plat = Utils.unversioned_sys_platform()
        cxx_save = cxx_compiler[plat]
        c_save = c_compiler[plat]
        cxx_compiler[plat] = ['msvc']
        c_compiler[plat] = ['msvc']

        ctx.env.WINRCFLAGS = rcflags_save

    ctx.load('compiler_c compiler_cxx')

    if clang_hack:
        cxx_compiler[plat] = cxx_save
        c_compiler[plat] = c_save

    ctx.filter_flags(['CFLAGS', 'CXXFLAGS'], [
        # error on unknown arguments, including unknown options that turns
        # unknown argument warnings into error. どうして？
        '-Werror=unknown-warning-option',
        '-Werror=ignored-optimization-argument',
        '-Werror=unknown-argument',

        '-fdiagnostics-color', '-fdiagnostics-show-option',
    ])
    ctx.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], [
        '-Wall', '-Wextra', '-pedantic',
        '-Wno-parentheses', '-Wno-assume', '-Wno-attributes',
        '-Wold-style-cast', '-Woverloaded-virtual', '-Wimplicit-fallthrough',
        '-Wno-undefined-var-template', # TYPE_NAME usage
    ])
    ctx.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], [
        '-Wno-parentheses-equality', # boost fs, windows build
        '-Wno-microsoft-enum-value', '-Wno-shift-count-overflow', # ljx
        '-Wno-varargs',
    ])

    if ctx.check_cxx(cxxflags=['-isystem', '.'],
                     features='cxx', mandatory=False,
                     msg='Checking for compiler flag -isystem'):
        ctx.env.CPPSYSPATH_ST = ['-isystem']
    elif ctx.check_cxx(cxxflags=['-imsvc', '.'],
                       features='cxx', mandatory=False,
                       msg='Checking for compiler flag -imsvc'):
        ctx.env.CPPSYSPATH_ST = ['-imsvc']
    else:
        ctx.env.CPPSYSPATH_ST = ctx.env.CPPPATH_ST

    if ctx.env['COMPILER_CXX'] == 'msvc':
        ctx.define('_CRT_SECURE_NO_WARNINGS', 1)
        ctx.env.append_value('CXXFLAGS', [
            '-Xclang', '-std=c++1z',
            '-Xclang', '-fdiagnostics-format', '-Xclang', 'clang',
            '-EHsa', '-MD'])
        ctx.env.append_value('CFLAGS_EXT', ['-EHsa', '-MD'])

        m = ctx.get_defs('msvc lib version', '#include <yvals.h>', cxx=True)
        cpp_ver = m['_CPPLIB_VER']
        if cpp_ver == '610':
            ctx.end_msg('610, activating include patching', color='YELLOW')
            inc = '-I' + ctx.path.find_node('msvc_include').abspath()
            ctx.env.prepend_value('CFLAGS', inc)
            ctx.env.prepend_value('CXXFLAGS', inc)
        else:
            ctx.end_msg(cpp_ver)
    else:
        ctx.check_cxx(cxxflags='-std=c++1z')
        ctx.env.append_value('CXXFLAGS', ['-std=c++1z'])

        ctx.filter_flags(['CFLAGS', 'CXXFLAGS'], ['-fvisibility=hidden'])
        ctx.env.append_value('LINKFLAGS', '-rdynamic')

    m = ctx.get_defs('destination OS', '')
    if '_WIN64' in m:
        ctx.env.DEST_OS = 'win64'
    elif '_WIN32' in m:
        ctx.env.DEST_OS = 'win32'
    ctx.end_msg(ctx.env.DEST_OS)

    if ctx.env.DEST_OS == 'win32' or ctx.env.DEST_OS == 'win64':
        ctx.define('WINDOWS', 1)
        ctx.define('UNICODE', 1)
        ctx.define('_UNICODE', 1)

        ctx.check_cxx(lib='kernel32')
        ctx.check_cxx(lib='shell32')
        ctx.check_cxx(lib='user32')

def build(bld):
    fixup_msvc()
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
    if not ctx.env.WITHOUT_LUA:
        src += [
            'src/libshit/logger.lua',
            'src/libshit/lua/base.cpp',
            'src/libshit/lua/base_funcs.lua',
            'src/libshit/lua/userdata.cpp',
            'src/libshit/lua/user_type.cpp',
        ]

    ctx.stlib(idx      = 50000 + (len(pref)>0),
              source   = src,
              uselib   = app,
              use      = 'BOOST BRIGAND lua',
              includes = 'src',
              export_includes = 'src',
              target   = pref+'libshit')


from waflib.Build import BuildContext
class TestContext(BuildContext):
    cmd = 'test'
    fun = 'test'
Context.g_module.TestContext = TestContext

def test(bld):
    build(bld)

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

    src = [
        #'test/main.cpp', workaround static lib linking problems, add it in main
        'test/char_utils.cpp',
        'test/options.cpp',
        'test/container/ordered_map.cpp',
        'test/container/parent_list.cpp',
    ]
    if not bld.env.WITHOUT_LUA:
        src += [
            'test/lua/base.cpp',
            'test/lua/function_call.cpp',
            'test/lua/function_ref.cpp',
            'test/lua/user_type.cpp',
        ]
    bld.objects(idx      = 50002,
                source   = src,
                includes = 'src',
                uselib   = app,
                use      = 'CATCH libshit',
                target   = 'libshit-tests')

    if not bld.options.skip_run_tests:
        bld.add_post_fun(lambda ctx:
            ctx.exec_command([
                bld.get_tgen_by_name('run-tests').link_task.outputs[0].abspath(),
                '--use-colour', 'yes'], cwd=bld.variant_dir) == 0 or ctx.fatal('Test failure')
        )


################################################################################
# random utilities
from waflib.Configure import conf
@conf
def filter_flags(cfg, vars, flags):
    ret = []

    for flag in flags:
        try:
            # gcc ignores unknown -Wno-foo flags but not -Wfoo, but warns if
            # there are other warnings. with clang, just depend on
            # -Werror=ignored-*-option, -Werror=unknown-*-option
            testflag = flag
            if flag[0:5] == '-Wno-':
                testflag = '-W'+flag[5:]
            cfg.check_cxx(cxxflags=[testflag], features='cxx',
                          msg='Checking for compiler flags '+testflag)
            ret.append(flag)
            for var in vars:
                cfg.env.append_value(var, flag)
        except:
            pass

    return ret

@conf
def filter_flags_c(cfg, vars, flags):
    ret = []

    for flag in flags:
        try:
            # gcc ignores unknown -Wno-foo flags but not -Wfoo, but warns if
            # there are other warnings. with clang, just depend on
            # -Werror=ignored-*-option, -Werror=unknown-*-option
            testflag = flag
            if flag[0:5] == '-Wno-':
                testflag = '-W'+flag[5:]
            cfg.check_cc(cflags=[testflag],
                         msg='Checking for compiler flags '+testflag)
            ret.append(flag)
            for var in vars:
                cfg.env.append_value(var, flag)
        except:
            pass

    return ret

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
def system_chk(ctx, name, default, system_chk, bundle_chk, cross=False):
    envname = 'BUILD_' + Utils.quote_define_name(name)
    hasname = 'HAS_'  + Utils.quote_define_name(name)

    if bundle_chk == None:
        def fun(ctx):
            pass
        bundle_chk = fun
    if isinstance(bundle_chk, str):
        incl_dir = ctx.path.find_dir(bundle_chk).abspath()
        def fun(ctx):
            ctx.env['SYSTEM_INCLUDES_'+name.upper()] = incl_dir
        bundle_chk = fun

    def x(name):
        opt = getattr(ctx.options, 'system_'+name) or default
        if opt == 'auto' or opt == 'system':
            try:
                system_chk(ctx)
                ctx.env[envname] = False
                ctx.env[hasname] = True
                ctx.msg('Using '+name, 'system')
                return
            except ConfigurationError:
                if opt == 'system': raise

        if opt == 'auto' or opt == 'bundle':
            try:
                bundle_chk(ctx)
                ctx.msg('Using '+name, 'bundled')
                ctx.env[envname] = True
                ctx.env[hasname] = True
                return
            except ConfigurationError:
                if opt == 'bundle': raise

        ctx.msg('Using '+name, False)
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

    cmd += ['-E', file.abspath() ]
    if ctx.env['COMPILER_CXX'] == 'msvc': cmd += ['-Xclang', '-dM']
    else: cmd += ['-dM']
    try:
        out = ctx.cmd_and_log(cmd)
    except Exception:
        ctx.end_msg(False)
        raise

    return dict(map(lambda l: defs_re.match(l).groups(), out.splitlines()))
