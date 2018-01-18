#!/usr/bin/env python
import argparse, os, sys
import pprint
import subprocess
import json

SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(SCRIPT_PATH, '../../binding_generator/scripts/'))
from starfish_idl_reader import gen_ir_from_file, merge_irs, apply_types

try:
  from jinja2 import Environment, FileSystemLoader
except ImportError:
  print 'Error: Jinja2 not found'
  print 'Exiting...'
  os._exit(0)

def prerun_all(dir_path):
  result = {}
  for (root, dirs, files) in os.walk(dir_path):
    for f in files:
      if os.path.splitext(f)[-1] != '.idl':
        continue
      file_path = os.path.join(root, f)
      if 'unimpl_' in f:
        continue
      ir = gen_ir_from_file(file_path)
      merge_irs(result, ir)
  apply_types(result, result)
  return result

def has_unimplemented_in_interface(interface):
  if interface.get('constructor') and interface['constructor'].get('unimplemented'):
    return True
  if interface.get('name_constructor') and interface['name_constructor'].get('unimplemented'):
    return True
  for attribute in interface.get('attributes'):
    if attribute.get('unimplemented'):
      return True
  for function in interface.get('functions'):
    if function.get('unimplemented'):
      return True

def generate_html(all_irs):
  json_out = {
    'interfaces': []
  }
  starfish_interfaces = all_irs['interfaces']
  for key in starfish_interfaces:
    item = starfish_interfaces[key]
    if item.get('unimplemented'):
      continue
    if item.get('partial_interface'):
      continue
    newitem = {}
    newitem['name'] = item['name']
    if has_unimplemented_in_interface(item):
      newitem['has_unimplemented'] = True
      item['has_unimplemented'] = True
    if item.get('flags'):
      newitem['flags'] = list(item['flags'])
    json_out['interfaces'].append(newitem)
    generate_code_with_template(item, os.path.join(SCRIPT_PATH, '../webpages/webapi/'), 'autogen_' + item['name'] + '.html', 'template_webapi_item.html')

  # Generate main html
  generate_code_with_template(json_out, os.path.join(SCRIPT_PATH, '../webpages/webapi/'), 'autogen_webapi_main.html', 'template_webapi_main.html')
  print 'Done!'
  print 'Check ' + os.path.join(SCRIPT_PATH, '../webpages/webapi/autogen_webapi_main.html')

def generate_code_with_template(ir, out_path, out_name, template):
  template = env.get_template(template)
  if not os.path.exists(out_path):
    raise Exception('Out path \'' + out_path + '\' doesn\'t exist')
  with open(os.path.join(out_path, out_name), 'w') as w:
    ret = template.render(**ir)
    w.write(ret)
    # print('> Generated Code \'{}\''.format(out_name))

def index_to_capital(index):
  return chr(ord('A') + index)

if __name__ == '__main__':
  argparser = argparse.ArgumentParser()
  argparser.add_argument('root_path', help='root directiory to start')
  args = argparser.parse_args()
  # Argument validation
  if not os.path.isdir(args.root_path):
    print 'ERR: Invalid root path \'' + args.root_path + '\''
    sys.exit(1)
  # Jinja setting
  env = Environment(loader=FileSystemLoader(SCRIPT_PATH), trim_blocks=True,
                    lstrip_blocks=True)
  env.filters['index_to_capital'] = index_to_capital
  env.filters['jsonify'] = json.dumps

  all_irs = prerun_all(args.root_path)
  generate_html(all_irs)

