import Options
from os import unlink, symlink, popen
from os.path import exists 

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  fb_config = conf.find_program('fb_config', var='FB_CONFIG', mandatory=True)
  fb_libdir = popen("%s --libs" % fb_config).readline().strip()
  conf.env.append_value("LIBPATH_FB", fb_libdir)
  conf.env.append_value("LIB_FB", "fbclient")
  fb_includedir = popen("%s --cflags" % fb_config).readline().strip()
  conf.env.append_value("CPPPATH_FB", fb_includedir)

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'binding'
  obj.source = "binding.cc"
  obj.uselib = "FB"

