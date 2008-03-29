#!/usr/bin/python

import ConfigParser
import glob
import os.path
import sys

def printerr(msg):
	print >> sys.stderr, msg

class Plugin:
	name = ''
	provides = ''
	summary = ''
	description = ''
	directory = ''
	depends = []
	type = ''

	def __init__(self, directory, name, parser):
		self.directory = directory
		self.name = name

		self.provides = parser.get(name, 'provides')
		self.summary = parser.get(name, 'summary')
		self.description = parser.get(name, 'description')
		self.depends = parser.get(name, 'depends').split()
		self.type = parser.get(name, 'type')

		if self.type != 'default' and self.type != 'incomplete' and self.type != 'abusive':
			printerr('\'%s\' has an unknown type of \'%s\'!' % (self.name, self.type))

	def __str__(self):
		return self.name

class PluginPack:
	plugins = []

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

				self.plugins.append(p)

	def list(self, type):
		list = []

		for plugin in self.plugins:
			if plugin.type == type:
				list.append(plugin)

		return list

	def dump_list(self, list):
		for i in list:
			print '\t%s' % i

	def debug(self):
		print 'Default:'
		self.dump_list(self.list('default'))

		print 'Abusive:'
		self.dump_list(self.list('abusive'))

		print 'Incomplete:'
		self.dump_list(self.list('incomplete'))

def main():
	pp = PluginPack()

	pp.load_plugins()

	pp.debug()

if __name__ == "__main__":
	main()
