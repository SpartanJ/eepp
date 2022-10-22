import os
import sys
import datetime

project = 'eepp'
copyright = datetime.date.today().strftime("%Y") + ', Martín Lucas Golini'
author = 'Martín Lucas Golini'

sys.path.insert(1, os.path.abspath('./sphinx'))

extensions = ['doxyrest', 'cpplexer']

templates_path = ['_templates']

exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme = 'sphinx_rtd_theme'

doxyrest_tab_width = 4
