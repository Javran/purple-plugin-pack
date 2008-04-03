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

  -a  Load abusive plugins
  -d  Load default plugins
  -f  Load finch plugins
  -i  Load incomplate plugins
  -p  Load purple plugins
  -P  Load pidgin plugins

Commands:

  dependency-graph  Outputs a graphviz script of the plugin's dependencies
  info <plugin>     Display information about plugin.
"""

import ConfigParser
import getopt
import glob
import os.path
import string
import sys

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
				if types:
					try:
						types.index(p.type)
					except ValueError:
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

	def help(self, args):
		try:
			cmd = self.commands[args[0]]
			print cmd.__doc__
		except KeyError:
			print 'command \'%s\' was not found' % args[0]
		except IndexError:
			pass
	commands['help'] = help

	def dist_dirs(self, args):
		"""Outputs a list of all plugin directories to included in the distribution"""
		dirs = {}
		for name in self.plugins.keys():
			dirs[self.plugins[name].directory] = 1

		dirs = dirs.keys()
		dirs.sort()
		print string.join(dirs, ' ')
	commands['dist_dirs'] = dist_dirs

	def dependency_graph(self, args):
		"""Outputs a graphviz script of dependencies"""
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

	def info(self, args):
		"""Outputs all information about the given plugins"""
		for p in args:
			try:
				print self.plugins[p].__str__().strip()
			except KeyError:
				print 'Failed to find a plugin that provides \'%s\'' % (p)

			print
	commands['info'] = info

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
		print __doc__
		sys.exit(1)

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
		print __doc__
		sys.exit(1)

	cmd = args[0]
	args = args[1:]

	try:
		pp.commands[cmd](pp, args)
	except KeyError:
		printerr('\'%s\' command not found' % (cmd))


if __name__ == '__main__':
	main()
