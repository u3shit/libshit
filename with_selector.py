from waflib.Configure import conf
from waflib.Errors import ConfigurationError
from waflib.Options import OptionsContext
from waflib import Context, Utils
import sys

all_optional = set()

all_default = {}
all_set = {'system': {}, 'bundle': {}}

def options(opt):
    grp = opt.add_option_group('Dependency selection options')
    grp.add_option('--all-bundled', dest='all_system',
                   action='store_const', const='bundle',
                   help="Use bundled libs where it's generally ok")
    grp.add_option('--all-system', dest='all_system',
                   action='store_const', const='system',
                   help="Disable bundled libs where it's generally ok")

def configure(cfg):
    if cfg.options.all_system:
        x = all_set[cfg.options.all_system]
        for k in x:
            if getattr(cfg.options, k, None) == None:
                setattr(cfg.options, k, x[k])



def with_opt(ctx, name, variants, default, help=None, cross=False,
             bundle_all=None, system_all=None):
    grp = ctx.get_option_group('Dependency selection options')
    help = help or 'Select %s' % name
    help_tail = "\v[default: %r" % default
    if bundle_all: help_tail += ', --all-bundled: %r' % bundle_all
    if system_all: help_tail += ', --all-system: %r' % system_all
    help_tail += ']'

    def x(name, help):
        wname = 'with_' + name
        helpmsg = '%s: %s%s' % (help, ', '.join(variants), help_tail)
        all_default[wname] = default
        if bundle_all: all_set['bundle'][wname] = bundle_all
        if system_all: all_set['system'][wname] = system_all
        if 'none' in variants: all_optional.add(name)

        grp.add_option('--with-'+name, dest=wname, action='store', help=helpmsg)
        if 'none' in variants:
            grp.add_option(
                '--without-'+name, dest=wname,
                action='store_const', const='none',
                help='Disable %s (same as --with-%s=none)' % (name, name))

    x(name, help)
    if cross: x(name + '-host', help + ' (for host)')
OptionsContext.with_opt = with_opt

def system_opt(ctx, name, cross=False, optional=False):
    variants = ['bundle', 'system']
    if optional:
        variants.append('none')
        system = 'system,none'
    else:
        system = 'system'
    with_opt(ctx, name, variants,
             default='system,bundle', bundle_all='bundle', system_all=system)
OptionsContext.system_opt = system_opt


app = Context.g_module.APPNAME.upper()

def run_fun(ctx, fun, def_name):
    if fun == None:
        return

    if isinstance(fun, str):
        incl_dir = ctx.path.find_dir(fun)
        if not incl_dir:
            ctx.fatal('%s/%s not found. Are git submodules missing?' %
                      (ctx.path.abspath(), fun))

        ctx.env['SYSTEM_INCLUDES_' + def_name] = incl_dir.abspath()
        return

    return fun(ctx)

# at least ljx messes around with variants, so stash every variant
def save_envs(ctx):
    stashed = set()
    for k in ctx.all_envs:
        stashed.add(k)
        ctx.all_envs[k].stash()
    return stashed

def revert_envs(ctx, stashed):
    for k in stashed:
        ctx.all_envs[k].revert()

def commit_envs(ctx, stashed):
    for k in stashed:
        ctx.all_envs[k].commit()


@conf
# chk_fun: if string, header only lib's include path
def with_chk(ctx, name, chks, cross=False, post_chk=None, define=None,
             define_name=None):
    def_name = define_name or Utils.quote_define_name(name)
    envname = 'WITH_' + def_name
    defines = 'DEFINES_' + def_name

    if define == None and name in all_optional:
        define = app + '_WITH_' + def_name
        if 'none' not in chks:
            chks['none'] = None

    def x(name):
        wname = 'with_' + name
        opts = (getattr(ctx.options, wname) or all_default[wname]).split(',')
        if opts == []:
            ctx.fatal('No value specified for --with-%s' % name)

        last_except = None
        for opt in opts:
            if not opt in chks:
                ctx.fatal('Unknown option for --with-%s: %s' % (name, opt))

            saved = save_envs(ctx)
            try:
                run_fun(ctx, chks[opt], def_name)
                ctx.env[envname] = opt

                if define:
                    val = '%s=%s' % (define, opt != 'none' and 1 or 0)
                    ctx.env.append_value(defines, val)
                if post_chk: post_chk(ctx)
                ctx.msg('Using ' + name, opt,
                        color = opt != 'none' and 'GREEN' or 'YELLOW')

                commit_envs(ctx, saved)
                return
            except ConfigurationError:
                revert_envs(ctx, saved)
                last_except = sys.exc_info()

        if sys.hexversion < 0x3000000:
            exec('raise last_except[0], last_except[1], last_except[2]')
        else:
            exec('raise last_except[0].with_traceback(last_except[1], last_except[2])')

    x(name)
    if cross: ctx.only_host_env(lambda bull,shit: x(name+'-host'))
