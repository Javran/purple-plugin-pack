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

"""Usage: plugin_pack.py [options]

  -a, --abusive-plugins     Outputs a comma-separated list of the abusive
                            plugins
  -D, --dist-dirs           Outputs the list of all directories that have
                            plugins and should be included in the distribution
  -d, --default-plugins     Outputs a comma-separated list of the default
                            plugins
      --dependency-graph    Outputs a graphviz diagram showing dependencies
  -h, --help                Shows usage information
  -i, --incomplete-plugins  Outputs a comma-separated list of the incomplete
                            plugins
  -p, --plugin=<name>       Outputs all info about <name>
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
		output += 'authors: %s\n' % string.join(self.authors, ',')
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
	plugins = {}

	def load_plugins(self):
		for file in glob.glob('*/plugins.cfg'):
			parser = ConfigParser.ConfigParser()

			try:
				parser.read(file)
			except ConfigParser.ParsingError,  msg:
				printerr('Failed to parse \'%s\':\n%s' % (file, msg))
				continue

			for plugin in parser.sections():
				p = Plugin(os.path.dirname(file), plugin, parser)

				self.plugins[p.name] = p

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

	def dist_dirs(self):
		dirs = {}
		for name in self.plugins.keys():
			dirs[name] = 1

		dirs = dirs.keys()
		dirs.sort()
		for dir in dirs:
			print dir

	def default_plugins(self):
		return self.list_type('default')

	def abusive_plugins(self):
		return self.list_type('abusive')

	def incomplete_plugins(self):
		return self.list_type('incomplete')

	def dependency_graph(self):
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
		print '\toverlap=scale;'
		print
		print '\tnode[fontname="sans", style="filled", shape="box"];'
		print

		# run through the default plugins
		print '\t/* default plugins */'
		print '\tnode[fillcolor="palegreen"];'
		print_plugins(self.default_plugins())
		print

		# run through the incomplete plugins
		print '\t/* incomplete plugins */'
		print '\tnode[fillcolor="lightyellow1"];'
		print_plugins(self.incomplete_plugins())
		print

		# run through the abusive plugins
		print '\t/* abusive plugins */'
		print '\tnode[fillcolor="lightpink"];'
		print_plugins(self.abusive_plugins())
		print

		# run through again, this time showing the relations
		print '\t/* dependencies'
		print '\t * exteranl ones that don\'t have nodes get colored to the following'
		print '\t */'
		print '\tnode[fillcolor="powderblue"];'

		for name in self.plugins.keys():
			plugin = self.plugins[name]

			node, label = node_label(plugin)

			for dep in plugin.depends:
				dep = dep.replace('-', '_')
				print '\t%s -> %s;' % (node, dep)

		print '}'

def main():
	# create our main instance
	pp = PluginPack()

	# load all our plugin data
	pp.load_plugins()

	try:
		shortopts = 'aDdhip:'
		longopts = [
			'abusive-plugins',
			'dependency-graph',
			'default-plugins',
			'dist-dirs',
			'help',
			'incomplete-plugins',
			'plugin',
		]

		opts, args = getopt.getopt(sys.argv[1:], shortopts, longopts)
	except getopt.error, msg:
		print msg
		print __doc__
		sys.exit(1)

	if not opts:
		print __doc__
		sys.exit(1)

	for o, a in opts:
		if o in ('-a', '--abusive-plugins'):
			pp.print_names(pp.abusive_plugins())
		elif o == '--dependency-graph':
			pp.dependency_graph()
		elif o in ('-D', '--dist-dirs'):
			pp.dist_dirs()
		elif o in ('-d', '--default-plugins'):
			pp.print_names(pp.default_plugins())
		elif o in ('-h', '--help'):
			print __doc__
			sys.exit(0)
		elif o in ('-i', '--incomplete-plugins'):
			pp.print_names(pp.incomplete_plugins())
		elif o in ('-p', '--plugin'):
			print pp.plugins[a]

if __name__ == "__main__":
	main()
