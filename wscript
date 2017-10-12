# -*- mode: python -*-

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

all_system = []
def options(opt):
    opt.load('compiler_c compiler_cxx')
    grp = opt.get_option_group('configure options')
    grp.add_option('--clang-hack', action='store_true', default=False,
                   help='Read COMPILE.md...')
    grp.add_option('--optimize', action='store_true', default=False,
                   help='Enable some default optimizations')
    grp.add_option('--optimize-ext', action='store_true', default=False,
                   help='Optimize ext libs even if %s is in debug mode' % app.title())
    grp.add_option('--release', action='store_true', default=False,
                   help='Enable some flags for release builds')
    grp.add_option('--all-system', dest='all_system',
                   action='store_const', const='system',
                   help="Disable bundled libs where it's generally ok")
    grp.add_option('--all-bundled', dest='all_system',
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

    cfg.load('compiler_c')

    if cfg.options.optimize or cfg.options.optimize_ext:
        cfg.filter_flags_c(['CFLAGS'], ['-O1', '-march=native'])

    # ----------------------------------------------------------------------
    # setup target
    cfg.setenv(variant)
    Logs.pprint('NORMAL', 'Configuring target compiler '+variant)
    cfg.env.CROSS = cross
    cfg.environ = environ

    # override flags specific to app/bundled libraries
    for v in [app, 'EXT']:
        cfg.add_os_flags('CPPFLAGS_'+v, dup=False)
        cfg.add_os_flags('CFLAGS_'+v, dup=False)
        cfg.add_os_flags('CXXFLAGS_'+v, dup=False)
        cfg.add_os_flags('LINKFLAGS_'+v, dup=False)
        cfg.add_os_flags('LDFLAGS_'+v, dup=False)

    if cfg.options.clang_hack:
        cfg.find_program('clang-cl', var='CC')
        cfg.find_program('clang-cl', var='CXX')
        cfg.find_program('lld-link', var='LINK_CXX')
        cfg.find_program('llvm-lib', var='AR')
        cfg.find_program(cfg.path.abspath()+'/rc.sh', var='WINRC')

        cfg.add_os_flags('WINRCFLAGS', dup=False)
        rcflags_save = cfg.env.WINRCFLAGS

        cfg.load('msvc', funs='no_autodetect')
        fixup_msvc()
        from waflib.Tools.compiler_cxx import cxx_compiler
        from waflib.Tools.compiler_c import c_compiler
        from waflib import Utils

        plat = Utils.unversioned_sys_platform()
        cxx_save = cxx_compiler[plat]
        c_save = c_compiler[plat]
        cxx_compiler[plat] = ['msvc']
        c_compiler[plat] = ['msvc']

        cfg.env.WINRCFLAGS = rcflags_save

    cfg.load('compiler_c compiler_cxx clang_compilation_database')

    if cfg.options.clang_hack:
        cxx_compiler[plat] = cxx_save
        c_compiler[plat] = c_save

    cfg.filter_flags(['CFLAGS', 'CXXFLAGS'], [
        # error on unknown arguments, including unknown options that turns
        # unknown argument warnings into error. どうして？
        '-Werror=unknown-warning-option',
        '-Werror=ignored-optimization-argument',
        '-Werror=unknown-argument',

        '-fdiagnostics-color', '-fdiagnostics-show-option',
    ])
    cfg.filter_flags(['CFLAGS_'+app, 'CXXFLAGS_'+app], [
        '-Wall', '-Wextra', '-pedantic',
        '-Wno-parentheses', '-Wno-assume', '-Wno-attributes',
        '-Wold-style-cast', '-Woverloaded-virtual', '-Wimplicit-fallthrough',
        '-Wno-undefined-var-template', # TYPE_NAME usage
    ])
    cfg.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], [
        '-Wno-parentheses-equality', # boost fs, windows build
        '-Wno-microsoft-enum-value', '-Wno-shift-count-overflow', # ljx
        '-Wno-varargs',
    ])

    if cfg.check_cxx(cxxflags=['-isystem', '.'],
                     features='cxx', mandatory=False,
                     msg='Checking for compiler flag -isystem'):
        cfg.env.CPPSYSPATH_ST = ['-isystem']
    elif cfg.check_cxx(cxxflags=['-imsvc', '.'],
                       features='cxx', mandatory=False,
                       msg='Checking for compiler flag -imsvc'):
        cfg.env.CPPSYSPATH_ST = ['-imsvc']
    else:
        cfg.env.CPPSYSPATH_ST = cfg.env.CPPPATH_ST

    if cfg.env['COMPILER_CXX'] == 'msvc':
        cfg.define('_CRT_SECURE_NO_WARNINGS', 1)
        cfg.env.append_value('CXXFLAGS', [
            '-Xclang', '-std=c++1z',
            '-Xclang', '-fdiagnostics-format', '-Xclang', 'clang',
            '-EHsa', '-MD'])
        cfg.env.append_value('CFLAGS_EXT', ['-EHsa', '-MD'])
        inc = '-I' + cfg.path.find_node('msvc_include').abspath()
        cfg.env.prepend_value('CFLAGS', inc)
        cfg.env.prepend_value('CXXFLAGS', inc)

        if cfg.options.optimize:
            #cfg.env.prepend_value('CFLAGS', ['/O1', '/GS-'])
            for f in ['CFLAGS_EXT', 'CXXFLAGS_EXT', 'CXXFLAGS_'+app]:
                cfg.env.prepend_value(f, [
                    '-O2', '-Xclang', '-emit-llvm-bc'])
            cfg.env.prepend_value('LINKFLAGS', ['/OPT:REF', '/OPT:ICF'])
        elif cfg.options.optimize_ext:
            cfg.env.prepend_value('CXXFLAGS_EXT', '-O2')
    else:
        cfg.check_cxx(cxxflags='-std=c++1z')
        cfg.env.append_value('CXXFLAGS', ['-std=c++1z'])

        cfg.filter_flags(['CFLAGS', 'CXXFLAGS'], ['-fvisibility=hidden'])
        cfg.env.append_value('LINKFLAGS', '-rdynamic')
        if cfg.options.optimize:
            cfg.filter_flags(['CFLAGS', 'CXXFLAGS', 'LINKFLAGS'], [
                '-Ofast', '-flto', '-fno-fat-lto-objects',
                 '-fomit-frame-pointer'])

            cfg.env.append_value('LINKFLAGS', '-Wl,-O1')
        elif cfg.options.optimize_ext:
            cfg.filter_flags(['CFLAGS_EXT', 'CXXFLAGS_EXT'], ['-g0', '-Ofast'])

    def chkdef(cfg, defn):
        return cfg.check_cxx(fragment='''
#ifndef %s
#error err
#endif
int main() { return 0; }
''' % defn,
                             msg='Checking for '+defn,
                             features='cxx',
                             mandatory=False)

    if chkdef(cfg, '_WIN64'):
        cfg.env.DEST_OS = 'win64'
    elif chkdef(cfg, '_WIN32'):
        cfg.env.DEST_OS = 'win32'

    if cfg.options.release:
        cfg.define('NDEBUG', 1)
    if cfg.env.DEST_OS == 'win32' or cfg.env.DEST_OS == 'win64':
        cfg.define('WINDOWS', 1)
        cfg.define('UNICODE', 1)
        cfg.define('_UNICODE', 1)

        cfg.check_cxx(lib='kernel32')
        cfg.check_cxx(lib='shell32')
        cfg.check_cxx(lib='user32')

    Logs.pprint('NORMAL', 'Configuring ext '+variant)
    cfg.recurse('ext', name='configure', once=False)

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

    src = [
        'src/libshit/char_utils.cpp',
        'src/libshit/options.cpp',
        'src/libshit/except.cpp',
        'src/libshit/logger.cpp',
    ]
    if not bld.env.WITHOUT_LUA:
        src += [
            'src/libshit/logger.lua',
            'src/libshit/lua/base.cpp',
            'src/libshit/lua/base_funcs.lua',
            'src/libshit/lua/userdata.cpp',
            'src/libshit/lua/user_type.cpp',
        ]

    bld.stlib(source   = src,
              uselib   = app,
              use      = 'BOOST BRIGAND lua',
              includes = 'src',
              export_includes = 'src',
              target   = 'libshit')


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
    ]
    if not bld.env.WITHOUT_LUA:
        src += [
            'test/lua/base.cpp',
            'test/lua/function_call.cpp',
            'test/lua/function_ref.cpp',
            'test/lua/user_type.cpp',
        ]
    bld.objects(source   = src,
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

# waf 1.7 style logging: c/cxx instead of vague shit like 'processing',
# 'compiling', etc.
from waflib.Task import Task
def getname(self):
    return self.__class__.__name__ + ':'
Task.keyword = getname

# system building helpers
from waflib.Options import OptionsContext
def system_opt(ctx, name, include_all=True):
    grp = ctx.get_option_group('configure options')
    sname = 'system_' + name
    grp.add_option('--system-'+name, dest=sname,
                   action='store_const', const='system',
                   help='Use system '+name)
    grp.add_option('--bundled-'+name, dest=sname,
                   action='store_const', const='bundle',
                   help='Use bundled '+name)
    if include_all:
        global all_system
        all_system += [name]
OptionsContext.system_opt = system_opt

from waflib.Errors import ConfigurationError
from waflib import Utils
@conf
# bundle_chk: if string, header only lib's include path
def system_chk(ctx, name, default, system_chk, bundle_chk):
    opt = getattr(ctx.options, 'system_'+name) or default
    envname = 'BUILD_' + Utils.quote_define_name(name)

    if bundle_chk == None:
        def fun(ctx):
            pass
        bundle_chk = fun
    if isinstance(bundle_chk, str):
        incl_dir = ctx.path.find_dir(bundle_chk).abspath()
        def fun(ctx):
            ctx.env['SYSTEM_INCLUDES_'+name.upper()] = incl_dir
        bundle_chk = fun

    if opt == 'auto':
        try:
            system_chk(ctx)
            ctx.env[envname] = False
            ctx.msg('Using '+name, 'system')
            return
        except ConfigurationError:
            opt = 'bundle'

    if opt == 'system':
        system_chk(ctx)
        ctx.msg('Using '+name, 'system')
        ctx.env[envname] = False
    elif opt == 'bundle':
        bundle_chk(ctx)
        ctx.msg('Using '+name, 'bundled')
        ctx.env[envname] = True
    else:
        ctx.fatal('Invalid opt')


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
