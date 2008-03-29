#!/usr/bin/python

import ConfigParser
import glob
import os.path
import sys

plugins = []

class Plugin:
	name = ''
	summary = ''
	description = ''
	directory = ''
	dependencies = []
	type = ''

	def __init__(self, directory, name, parser):
		self.directory = directory

		self.name = parser.get(name, 'name')
		self.summary = parser.get(name, 'summary')
		self.description = parser.get(name, 'description')
		self.dependencies = parser.get(name, 'dependencies').split()
		self.type = parser.get(name, 'type')

	def __str__(self):
		return '[name=\'%s\', directory=\'%s\']' % (self.name, self.directory)

def printerr(msg):
	print >> sys.stderr, msg

def load_plugins():
	for file in glob.glob('*/plugins.ini'):
		parser = ConfigParser.ConfigParser()

		try:
			parser.read(file)
		except ConfigParser.ParsingError,  msg:
			printerr('Failed to parse \'%s\':\n%s' % (file, msg))
			continue

		if not parser.has_section('plugins'):
			printerr('\'%s\' does not have a plugins section!' % file)
			continue

		if not parser.has_option('plugins', 'sections'):
			printerr('\'%s\' does not have a sections option under the plugins section' % file)
			continue

		for plugin in parser.get('plugins', 'sections').split():
			if not parser.has_section(plugin):
				printerr('\'%s\' does not have a section named \'%s\'!' % (file, plugin))
				continue

			p = Plugin(os.path.dirname(file), plugin, parser)

			plugins.append(p)

def main():
	load_plugins()

	for p in plugins:
		print p

if __name__ == "__main__":
	main()
