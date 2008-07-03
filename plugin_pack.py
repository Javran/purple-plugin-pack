#!/usr/bin/python

# plugin_pack.py - Helper script for obtaining info about the plugin pack
# Copyright (C) 2008 Gary Kramlich <grim@reaperworld.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.

"""Usage: plugin_pack.py [OPTION...] command

Flags:

  -a  Load abusive plugins
  -d  Load default plugins
  -f  Load finch plugins
  -i  Load incomplate plugins
  -p  Load purple plugins
  -P  Load pidgin plugins

Commands:
"""

import ConfigParser
import getopt
import glob
import os.path
import string
import sys

webpage = 'http://plugins.guifications.org/'

def printerr(msg):
	print >> sys.stderr, msg

class Plugin:
	name = ''
	directory = ''
	type = ''
	depends = []
	provides = ''
	summary = ''
	description = ''
	authors = []
	introduced = ''
	notes = ''

	def __init__(self, directory, name, parser):
		self.name = name

		self.directory = directory

		self.type = parser.get(name, 'type')
		self.depends = parser.get(name, 'depends').split()
		self.provides = parser.get(name, 'provides')
		self.summary = parser.get(name, 'summary')
		self.description = parser.get(name, 'description')
		self.authors = parser.get(name, 'authors').split(',')
		self.introduced = parser.get(name, 'introduced')

		if parser.has_option(name, 'notes'):
			self.notes = parser.get(name, 'notes')

		if self.type != 'default' and self.type != 'incomplete' and self.type != 'abusive':
			printerr('\'%s\' has an unknown type of \'%s\'!' % (self.name, self.type))

	def __str__(self):
		output  = 'name: %s\n' % self.name
		output += 'authors: %s\n' % string.join(self.authors, ', ')
		output += 'type: %s\n' % self.type
		output += 'depends: %s\n' % string.join(self.depends, ' ')
		output += 'provides: %s\n' % self.provides
		output += 'directory: %s\n' % self.directory
		output += 'summary: %s\n' % self.summary
		output += 'description: %s\n' % self.description

		if self.notes:
			output += 'notes: %s\n' % self.notes

		return output

class PluginPack:
	commands = {}
	plugins = {}

	def load_plugins(self, types, depends):
		if len(types) == 0:
			types = None

		if len(depends) == 0:
			depends = None

		for file in glob.glob('*/plugins.cfg'):
			parser = ConfigParser.ConfigParser()

			try:
				parser.read(file)
			except ConfigParser.ParsingError,  msg:
				printerr('Failed to parse \'%s\':\n%s' % (file, msg))
				continue

			for plugin in parser.sections():
				p = Plugin(os.path.dirname(file), plugin, parser)

				# this is kind of hacky, but if we have types, we check to see
				# if the type is in list of types to load.
				if types and not p.type in types:
					continue
				
				# now we check if the give plugins depends match the search
				# depends
				if depends:
					if len(set(depends).intersection(set(p.depends))) == 0:
						continue

				self.plugins[p.provides] = p

	def list_type(self, type):
		list = []

		for name in self.plugins.keys():
			plugin = self.plugins[name]
			if plugin.type == type:
				list.append(plugin)

		list.sort()

		return list

	def list_dep(self, dep):
		list = []

		for name in self.plugins.keys():
			plugin = self.plugins[name]

			if dep in plugin.depends:
				list.append(plugin)

		list.sort()

		return list

	def print_names(self, list):
		names = []

		for plugin in list:
			names.append(plugin.name)

		print string.join(names, ',')

	def default_plugins(self):
		return self.list_type('default')

	def abusive_plugins(self):
		return self.list_type('abusive')

	def incomplete_plugins(self):
		return self.list_type('incomplete')

	def purple_plugins(self):
		return self.list_dep('purple')

	def finch_plugins(self):
		return self.list_dep('finch')

	def pidgin_plugins(self):
		return self.list_dep('pidgin')

	def unique_dirs(self):
		dirs = {}
		for name in self.plugins.keys():
			dirs[self.plugins[name].directory] = 1

		dirs = dirs.keys()
		dirs.sort()

		return dirs

	def help(self, args):
		"""Displays information about other commands"""
		try:
			cmd = self.commands[args[0]]
			print cmd.__doc__
		except KeyError:
			print 'command \'%s\' was not found' % args[0]
		except IndexError:
			print self.help.__doc__
			print
			print 'help usage:'
			print '  help <command>'
			print
			print 'Available commands:'

			cmds = self.commands.keys()
			cmds.remove('help')
			cmds.sort()
			print '  %s' % (string.join(cmds, ' '))
	commands['help'] = help

	def dist_dirs(self, args):
		"""Displays a list of all plugin directories to included in the distribution"""
		print string.join(self.unique_dirs(), ' ')
	commands['dist_dirs'] = dist_dirs

	def build_dirs(self, args):
		"""Displays a list of the plugins that can be built"""
		if len(args) != 2:
			printerr('build_dirs expects 2 arguments:')
			printerr('\ta comma separated list of dependencies')
			printerr('\ta comma separated list of plugins to build')
			sys.exit(1)

		# store the external depedencies
		externals = args[0].split(',')

		deps = {}

		# run through the provided dependencies, setting their dependencies to
		# nothing since we know we already have them
		for d in externals:
			deps[d] = []

		# now run through the plugins adding their deps to the dictionary
		for name in self.plugins.keys():
			plugin = self.plugins[name]

			deps[plugin.provides] = plugin.depends

		# run through the requested plugins and store their plugin instance in check
		check = []
		for provides in args[1].split(','):
			try:
				if provides == 'all':
					defaults = []
					for p in self.default_plugins():
						defaults.append(p.provides)

					check += defaults

					continue

				plugin = self.plugins[provides]
				check.append(plugin.provides)
			except KeyError:
				continue

		# convert our list of plugins to check into a set to remove dupes
		#check = set(check)

		# create our list of plugins to build
		build = []

		# now define a function to check our deps
		def has_deps(provides):
			# don't add anything to build more than once
			if provides in build:
				return True

			try:
				dep_list = deps[provides]
			except KeyError:
				return False

			# now check the dependencies
			for dep in dep_list:
				if not has_deps(dep):
					return False

			# make sure the provides isn't an external
			if not provides in externals:
				build.append(provides)

			# everything checks out!
			return True

		# check all the plugins we were told to for their dependencies
		for c in check:
			has_deps(c)

		# now create a list of all directories to build
		output = []

		for provides in build:
			plugin = self.plugins[provides]

			output.append(plugin.directory)

		output.sort()

		print "%s" % (string.join(output, ','))
	commands['build_dirs'] = build_dirs
	
	def list_plugins(self, args):
		"""Displays a list similiar to 'dpkg -l' about the plugin pack"""

		data = {}

		# create an array for the widths, we initialize it to the lengths of
		# the title strings.  We ignore summary, since that one shouldn't
		# matter.
		widths = [4, 8, 0]

		for p in self.plugins.keys():
			plugin = self.plugins[p]

			if plugin.type == 'abusive':
				type = 'a'
			elif plugin.type == 'incomplete':
				type = 'i'
			else:
				type = 'd'

			if 'finch' in plugin.depends:
				ui = 'f'
			elif 'pidgin' in plugin.depends:
				ui = 'p'
			elif 'purple' in plugin.depends:
				ui = 'r'
			else:
				ui = 'u'

			widths[0] = max(widths[0], len(plugin.name))
			widths[1] = max(widths[1], len(plugin.provides))
			widths[2] = max(widths[2], len(plugin.summary))

			data[plugin.provides] = [type, ui, plugin.name, plugin.provides, plugin.summary]

		print 'Type=Default/Incomplete/Abusive'
		print '| UI=Finch/Pidgin/puRple/Unknown'
		print '|/ Name%s Provides%s Summary' % (' ' * (widths[0] - 4), ' ' * (widths[1] - 8))
		print '++-%s-%s-%s' % ('=' * (widths[0]), '=' * (widths[1]), '=' * (widths[2]))

		# create the format var
		fmt = '%%s%%s %%-%ds %%-%ds %%s' % (widths[0], widths[1]) #, widths[2])

		# now loop through the list again, with everything formatted
		list = data.keys()
		list.sort()

		for p in list:
			d = data[p]
			print fmt % (d[0], d[1], d[2], d[3], d[4])
	commands['list'] = list_plugins

	def config_file(self, args):
		"""Outputs the contents for the file to be m4_include()'d from configure"""
		uniqdirs = self.unique_dirs()

		# add our --with-plugins option
		print 'AC_ARG_WITH(plugins,'
		print '            AC_HELP_STRING([--with-plugins], [what plugins to build]),'
		print '            ,WITH_PLUGINS=all)'

		# determine and add our output files
		print 'PP_DIST_DIRS="%s"' % (string.join(uniqdirs, ' '))
		print 'AC_SUBST(PP_DIST_DIRS)'
		print
		print 'AC_CONFIG_FILES(['
		for dir in uniqdirs:
			print '\t%s/Makefile' % (dir)
		print '])'
		print

		# setup a second call to determine the plugins to be built
		print 'PP_BUILD=`$PYTHON $srcdir/plugin_pack.py build_dirs $DEPENDENCIES $WITH_PLUGINS`'
		print
		print 'PP_BUILD_DIRS=`echo $PP_BUILD | sed \'s/,/\ /g\'`'
		print 'AC_SUBST(PP_BUILD_DIRS)'
		print
		print 'PP_PURPLE_BUILD="$PYTHON $srcdir/plugin_pack.py -p show_names $PP_BUILD"'
		print 'PP_PIDGIN_BUILD="$PYTHON $srcdir/plugin_pack.py -P show_names $PP_BUILD"'
		print 'PP_FINCH_BUILD="$PYTHON $srcdir/plugin_pack.py -f show_names $PP_BUILD"'
	commands['config_file'] = config_file

	def dependency_graph(self, args):
		"""Outputs a graphviz script showing plugin dependencies"""
		def node_label(plugin):
			node = plugin.provides.replace('-', '_')
			label = plugin.name

			return node, label

		def print_plugins(list):
			for plugin in list:
				node, label = node_label(plugin)

				print '\t%s[label="%s"];' % (node, label)

		print 'digraph {'
		print '\tlabel="Dependency Graph";'
		print '\tlabelloc="t";'
		print '\tsplines=TRUE;'
		print '\toverlap=FALSE;'
		print
		print '\tnode[fontname="sans", fontsize="8", style="filled"];'
		print

		# run through the default plugins
		print '\t/* default plugins */'
		print '\tnode[fillcolor="palegreen",shape="tab"];'
		print_plugins(self.default_plugins())
		print

		# run through the incomplete plugins
		print '\t/* incomplete plugins */'
		print '\tnode[fillcolor="lightyellow1",shape="note"];'
		print_plugins(self.incomplete_plugins())
		print

		# run through the abusive plugins
		print '\t/* abusive plugins */'
		print '\tnode[fillcolor="lightpink",shape="octagon"];'
		print_plugins(self.abusive_plugins())
		print

		# run through again, this time showing the relations
		print '\t/* dependencies'
		print '\t * exteranl ones that don\'t have nodes get colored to the following'
		print '\t */'
		print '\tnode[fillcolor="powderblue", shape="egg"];'

		for name in self.plugins.keys():
			plugin = self.plugins[name]

			node, label = node_label(plugin)

			for dep in plugin.depends:
				dep = dep.replace('-', '_')
				print '\t%s -> %s;' % (node, dep)

		print '}'
	commands['dependency_graph'] = dependency_graph

	def debian_description(self, args):
		"""Outputs the description for the Debian packages"""
		print 'Description: %d useful plugins for Pidgin, Finch, and Purple' % len(self.plugins)
		print ' The Plugin Pack is a collection of many simple-yet-useful plugins for Pidgin,'
		print ' Finch, and Purple.  You will find a summary of each plugin below.  For more'
		print ' about an individual plugin, please see %s' % webpage
		print ' .'
		print ' Note: not all %d of these plugins are currently usable' % len(self.plugins)
		
		list = self.plugins.keys()
		list.sort()
		for key in list:
			plugin = self.plugins[key]
			print ' .'
			print ' %s: %s' % (plugin.name, plugin.summary)

		print ' .'
		print ' .'
		print ' Homepage: %s' % webpage
	commands['debian_description'] = debian_description

	def show_names(self, args):
		"""Displays the names of the given comma separated list of provides"""

		if len(args) == 0 or len(args[0]) == 0:
			printerr('show_names expects a comma separated list of provides')
			sys.exit(1)

		provides = args[0].split(',')
		if len(provides) == 0:
			print "none"

		line = " "

		for provide in provides:
			if not provide in self.plugins:
				continue

			name = self.plugins[provide].name

			if len(line) + len(name) + 2 > 75:
				print line.rstrip(',')
				line = ' '
			
			line += ' %s,' % name 

		if len(line) > 1:
			print line.rstrip(',')
	commands['show_names'] = show_names

	def info(self, args):
		"""Displays all information about the given plugins"""
		for p in args:
			try:
				print self.plugins[p].__str__().strip()
			except KeyError:
				print 'Failed to find a plugin that provides \'%s\'' % (p)

			print
	commands['info'] = info

	def stats(self, args):
		"""Displays stats about the plugin pack"""
		counts = {}
		
		counts['total'] = len(self.plugins)
		counts['default'] = len(self.default_plugins())
		counts['incomplete'] = len(self.incomplete_plugins())
		counts['abusive'] = len(self.abusive_plugins())
		counts['purple'] = len(self.purple_plugins())
		counts['finch'] = len(self.finch_plugins())
		counts['pidgin'] = len(self.pidgin_plugins())

		def value(val):
			return "%3d (%6.2f%%)" % (val, (float(val) / float(counts['total'])) * 100.0)

		print "Purple Plugin Pack Stats"
		print ""
		print "%d plugins in total" % (counts['total'])
		print
		print "Status:"
		print "  complete:   %s" % (value(counts['default']))
		print "  incomplete: %s" % (value(counts['incomplete']))
		print "  abusive:    %s" % (value(counts['abusive']))
		print ""
		print "Type:"
		print "  purple: %s" % (value(counts['purple']))
		print "  finch:  %s" % (value(counts['finch']))
		print "  pidgin: %s" % (value(counts['pidgin']))
	commands['stats'] = stats

def show_usage(pp, exitcode):
	print __doc__

	cmds = pp.commands.keys()
	cmds.sort()
	
	for cmd in cmds:
		print "  %-20s %s" % (cmd, pp.commands[cmd].__doc__)

	print ""

	sys.exit(exitcode)

def main():
	# create our main instance
	pp = PluginPack()

	types = []
	depends = []

	try:
		shortopts = 'adfiPp'

		opts, args = getopt.getopt(sys.argv[1:], shortopts)
	except getopt.error, msg:
		print msg
		show_usage(pp, 1)

	for o, a in opts:
		if o == '-a':
			types.append('abusive')
		elif o == '-d':
			types.append('default')
		elif o == '-i':
			types.append('incomplete')
		elif o == '-f':
			depends.append('finch')
		elif o == '-P':
			depends.append('pidgin')
		elif o == '-p':
			depends.append('purple')

	# load the plugins that have been requested, if both lists are empty, all
	# plugins are loaded
	pp.load_plugins(types, depends)

	if(len(args) == 0):
		show_usage(pp, 1)

	cmd = args[0]
	args = args[1:]

	try:
		pp.commands[cmd](pp, args)
	except KeyError:
		printerr('\'%s\' command not found' % (cmd))

if __name__ == '__main__':
	main()
