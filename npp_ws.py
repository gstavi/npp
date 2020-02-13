#!/usr/bin/python3

import os.path
import fnmatch
import re

# Folder within Notepad++ workspace.
class npp_ws_folder:
	def __init__(self, name):
		self.name = name
		self.sub_folders = []
		self.files = []
	pass

# Project within Notepad++ workspace.
class npp_ws_proj:
	def __init__(self, name):
		self.name = name
		# list of tuples (path, proj_folder)
		self.base_dirs = []
		self.folders = []
		self.files = []
		self.file_patterns = []
		self.ignore_dirs = ['.git']
		self.ignore_files = []
		self.no_scan_dirs = []
		self.files = []
	# Add a base directory into project that will later be scanned in order
	# to add its files into the project.
	def add_base_dir(self, path, folder=None):
		self.base_dirs.append((path, folder))
	# Find existing folder or generate a new one.
	def get_folder(folder_list, folder_name):
		for folder in folder_list:
			if (folder.name == folder_name):
				return folder
		folder = npp_ws_folder(folder_name)
		folder_list.append(folder)
		return folder
	# Add file into project. Generates folders within project as needed.
	def insert_file(self, path_list, file_path):
		file_list = self.files
		folder_list = self.folders
		for dir_name in path_list:
			folder = npp_ws_proj.get_folder(folder_list, dir_name)
			folder_list = folder.sub_folders
			file_list = folder.files
		file_list.append(file_path)

# Notepad++ workspace.
class npp_ws:
	name = 'npp_ws'
	path = '.'
	indent = '    '
	def __init__(self):
		self.projects = []
		self.file_patterns = ['*.c', '*.h', '*.cpp']
		self.ignore_dirs = ['.git']
		self.ignore_files = []
		self.no_scan_dirs = []
	def start_proj(self, name):
		new_proj = npp_ws_proj(name)
		self.projects.append(new_proj)
		return new_proj
	# Scan files of a specific project.
	def scan_proj(self, proj):
		proj_filter = scan_filter(self, proj)
		for base_dir in proj.base_dirs:
			scan = dir_scan(base_dir[0], base_dir[1])
			scan.run(proj_filter, proj)
	# Scan files on disk and insert them into projects.
	def scan(self):
		for proj in self.projects:
			self.scan_proj(proj)
	# Provide file path for output. If file resides in the sub tree of the
	# workspace file directory then file path will be relative, otherwise
	# absoulte.
	def file_path(self, file_path):
		if file_path.startswith(self.path):
			return file_path[len(self.path):]
		return file_path
	def generate_file_lines(self, level, file_list):
		prefix = self.indent * level
		for file in file_list:
			print(prefix + '<File name="' + ws.file_path(file) + '" />')
	def generate_folders(self, level, folder_list):
		for folder in folder_list:
			prefix = self.indent * level
			print(prefix + '<Folder name="' + folder.name + '">')
			self.generate_file_lines(level, folder.files)
			self.generate_folders(level + 1, folder.sub_folders)
			print(prefix + '</Folder>')
	def generate(self, ws_path):
		self.name = os.path.basename(ws_path)
		self.path = os.path.abspath(os.path.dirname(ws_path)) + os.sep
		print("<NotepadPlus>")
		for proj in self.projects:
			print(self.indent + '<Project name="' + proj.name + '">')
			level = 2
			self.generate_file_lines(level, proj.files)
			self.generate_folders(level, proj.folders)
			print(self.indent + '</Project>')
		print("</NotepadPlus>")

def pat_match(file_name, pattern_list):
	for pattern in (pattern_list):
		if fnmatch.fnmatch(file_name, pattern):
			return True
	return False

# Control which files and directories are added or not into the workspace.
class scan_filter:
	def __init__(self, ws, proj):
		self.file_patterns = ws.file_patterns + proj.file_patterns
		self.ignore_dirs = ws.ignore_dirs + proj.ignore_dirs
		self.ignore_files = ws.ignore_files + proj.ignore_files
		self.no_scan_dirs = []
		for dir in ws.no_scan_dirs + proj.no_scan_dirs:
			self.no_scan_dirs.append(fixed_path(dir))
	def file_match(self, file_name):
		if not pat_match(file_name, self.file_patterns):
			return False
		if pat_match(file_name, self.ignore_files):
			return False
		return True
	def use_dir(self, scan_path, dir_name):
		if pat_match(dir_name, self.ignore_dirs):
			return False
		dir_path = scan_path + os.sep + dir_name
		if dir_path in self.no_scan_dirs:
			return False
		return True

def path_to_list(path):
	path = os.path.normpath(path)
	path_list = list(filter(None, path.split(os.sep)))
	return path_list

def fixed_path(path):
	path = os.path.expandvars(path)
	path = os.path.expanduser(path)
	path = os.path.normpath(path)
	return path

class dir_scan:
	def __init__(self, base_path, proj_folder):
		self.base_path = fixed_path(base_path)
		self.curr_scan_path = []
		self.proj_path = []
		if isinstance(proj_folder, str):
			self.proj_path += path_to_list(proj_folder)
	def run(self, ws_filter, proj):
		scan_path = self.base_path + os.sep + os.sep.join(self.curr_scan_path)
		scan_path = os.path.normpath(scan_path)
		ent_iter = os.scandir(os.path.abspath(scan_path))
		for ent in ent_iter:
			if ent.is_file():
				if ws_filter.file_match(ent.name):
					proj.insert_file(self.proj_path, ent.path)
				continue
			if not ent.is_dir():
				continue;
			if ws_filter.use_dir(scan_path, ent.name):
				self.curr_scan_path.append(ent.name)
				self.proj_path.append(ent.name)
				self.run(ws_filter, proj)
				self.curr_scan_path.pop()
				self.proj_path.pop()
		ent_iter.close()

ws = npp_ws()

proj = ws.start_proj("plugins")
proj.add_base_dir("/home/gur/work/npp/tagleet", "tagleet")
proj.add_base_dir("/home/gur/work/npp/lexamples", "lexamples")
proj.add_base_dir("/home/gur/work/npp/lexamples", "lexamples")
proj.file_patterns += ['*.def', '*.txt', 'readme*', '*.rc']
proj = ws.start_proj("npp")
proj.add_base_dir("/home/gur/work/npp/trunk")
proj = ws.start_proj("scripts")
proj.add_base_dir("~/work/npp/scripts")

#ws.scan()
#ws.generate("/home/gur/work/my_workspace")

def add_config_line(config_lines, line):
	if len(line) > 0 and line[0] != '#':
		config_lines.append(line)

def load_config_lines(file_name):
	file = open(fixed_path(file_name))
	config_lines = []
	full_line = ''
	for inp_line in file:
		line = inp_line.rstrip()
		length = len(line)
		line = line.lstrip()
		new_length = len(line)
		if new_length > 0 and new_length < length:
			full_line += ' ' + line
			continue
		add_config_line(config_lines, full_line)
		full_line = line
	add_config_line(config_lines, full_line)
	file.close()
	return config_lines

def break_to_sections(config_lines):
	sections = []
	sec_name = None
	sec_lines = []
	for line in config_lines:
		result = re.fullmatch('\[(.*)\]', line)
		if not result:
			sec_lines.append(line)
			continue;
		if len(sec_lines) > 0:
			sections.append((sec_name, sec_lines))
		sec_name = result.group(1)
		sec_lines = []
	if len(sec_lines) > 0:
		sections.append((sec_name, sec_lines))
	return sections
		
def process_ws_section(ws, lines):
	for line in lines:
		result = re.fullmatch('(\w+)\s*=(.*)', line)
		if not result:
			continue
		var = result.group(1).strip()
		val = result.group(2).strip()
		if (var == 'file'):
			ws.name = val
		elif var == 'patterns':
			ws.file_patterns += val.split()
		elif var == 'ignore_files':
			ws.ignore_files += val.split()
		elif var == 'ignore_dirs':
			ws.ignore_dirs += val.split()
		elif var == 'no_scan_dirs':
			ws.no_scan_dirs += val.split()

def process_proj_section(ws, lines):
	proj = None
	for line in lines:
		result = re.fullmatch('(\w+)\s*=(.*)', line)
		if not result:
			continue
		var = result.group(1).strip()
		val = result.group(2).strip()
		if (var == 'name'):
			proj = ws.start_proj(val)
		elif var == 'scan':
			scan_args = val.split()
			path = scan_args[0]
			folder = scan_args[1] if len(scan_args) > 1 else None
			proj.add_base_dir(path, folder)
		elif var == 'patterns':
			proj.file_patterns += val.split()
		elif var == 'ignore_files':
			proj.ignore_files += val.split()
		elif var == 'ignore_dirs':
			proj.ignore_dirs += val.split()
		elif var == 'no_scan_dirs':
			proj.no_scan_dirs += val.split()
		elif var == 'files':
			for file in val.split():
				proj.files.append(fixed_path(file))

ws = npp_ws()
config_lines = load_config_lines("npp_ws.cfg")
sections = break_to_sections(config_lines)
			
for section in sections:
	if section[0] == 'Workspace':
		process_ws_section(ws, section[1])
	elif section[0] == 'Project':
		process_proj_section(ws, section[1])

ws.scan()
ws.generate(ws.name)
