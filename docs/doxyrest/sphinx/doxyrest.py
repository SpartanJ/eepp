#...............................................................................
#
#  This file is part of the Doxyrest toolkit.
#
#  Doxyrest is distributed under the MIT license.
#  For details see accompanying license.txt file,
#  the public copy of which is also available at:
#  http://tibbo.com/downloads/archive/doxyrest/license.txt
#
#...............................................................................

import os
import re
import warnings
from docutils import nodes
from docutils.parsers.rst import Directive, directives
from docutils.transforms import Transform
from docutils.statemachine import StringList, string2lines
from sphinx import roles, addnodes, config
from sphinx.io import SphinxBaseFileInput, SphinxRSTFileInput
from sphinx.directives.other import Include

#...............................................................................
#
#  utils
#

this_dir = os.path.dirname(os.path.realpath(__file__))
crefdb = {}

def get_cref_target(text):
    if text in crefdb:
        return crefdb[text]

    warnings.warn('target not found for cref: ' + text, Warning, 2)
    return None


#...............................................................................
#
#  Sphinx nodes
#

class HighlightedText(nodes.General, nodes.TextElement):
    def __init__(self, *args, **kwargs):
        nodes.TextElement.__init__(self, *args, **kwargs)


def visit_highlighted_text_node(self, node):
    text_node = node.children[0]
    language = node['language']

    if language == 'none':
        self.body.append(text_node)
    else:
        options = {}
        options['stripnl'] = False
        options['ensurenl'] = False

        text = text_node

        # pygments always ensures a newline at the end and there is no way to tell
        # if the newline is originating from source or was it inserted by pygments
        # as a workaround, duplicate trailing newline and then strip it

        if text.endswith('\n'):
            text = text + '\n'

        highlighted = self.highlighter.highlight_block(text, language, options)

        match = re.search('<pre>(.*)\n</pre>', highlighted, re.DOTALL)
        if not match:
            self.body.append(text_node)
        else:
            self.body.append(match.group(1))

    raise nodes.SkipNode

def create_xref_node(raw_text, text, target):
    if not target:
        return nodes.Text(text, text)

    node = addnodes.pending_xref(raw_text)
    node['reftype'] = 'ref'
    node['refdomain'] = 'std'
    node['reftarget'] = target
    node['refwarn'] = True
    node['refexplicit'] = True
    node += nodes.Text(text, text)
    return node

def create_target_node(raw_text, text, target, highlight_language, lineno, document, extra_classes=[]):
    node = nodes.target(raw_text, '')
    node['names'] += [target]
    node['classes'] += extra_classes
    node.line = lineno

    if text:
        if highlight_language:
            node += HighlightedText(text, text, language=highlight_language)
        else:
            node += nodes.Text(text, text)

    document.note_explicit_target(node)
    return node


#...............................................................................
#
#  Sphinx directives
#

class RefCodeBlock(Directive):
    has_content = True
    required_arguments = 0
    optional_arguments = 1
    final_argument_whitespace = False

    option_spec = {
        'linenos': directives.flag,
        'dedent': int,
        'lineno-start': int,
        'emphasize-lines': directives.unchanged_required,
        'caption': directives.unchanged_required,
        'class': directives.class_option,
        'name': directives.unchanged,
    }

    def __init__(self, *args, **kwargs):
        Directive.__init__(self, *args, **kwargs)

        role_re_src = '(:ref:|:cref:|:target:)'
        if self.state.document.settings.env.config.default_role == 'cref':
            role_re_src += '?' # explicit role is optional

        role_re_src += '`(.+?)(\s*<([^<>]*)>)?`'
        self.role_re_prog = re.compile(role_re_src)
        self.ws_re_prog = re.compile('\s+')
        self.url_re_prog = re.compile('(ftp|https?)://')

    def run(self):
        config = self.state.document.settings.env.config
        code = u'\n'.join(self.content)
        pos = 0
        node = nodes.literal_block('.', '') # single char to prevent sphinx from trying to highlight it
        node['classes'] += ['highlight']    # we are stripping pyments-generated <div>
        node['classes'] += self.options.get('class', [])

        if len(self.arguments) >= 1:
            language = self.arguments[0]
        else:
            language = config.highlight_language

        while True:
            match = self.role_re_prog.search(code, pos)
            if match is None:
                plain_text = code[pos:]
                if plain_text != "":
                    node += HighlightedText(plain_text, plain_text, language=language)
                break

            plain_text = code[pos:match.start()]
            if plain_text != "":
                node += HighlightedText(plain_text, plain_text, language=language)

            raw_text = match.group(0)
            role = match.group(1)
            text = match.group(2)
            target = match.group(4)

            pos = match.end()

            if text:
                text = text.replace('\\<', '<') # restore escaped left-chevron

            if role == ':target:':
                if not target:
                    target = text
                    text = None
                    ws_match = self.ws_re_prog.match(code, pos)
                    if ws_match: # drop whitespace right after empty target
                        pos = ws_match.end()

                new_node = create_target_node(
                    raw_text,
                    text,
                    target,
                    language,
                    None,
                    self.state.document,
                    ['doxyrest-code-target']
                    )

            elif not role or role == ':cref:':
                target = get_cref_target(target if target else text)
                new_node = create_xref_node(raw_text, text, target)

            else: # :ref:
                if not target:
                    target = text

                if self.url_re_prog.match(target):
                    new_node = nodes.reference(raw_text, text, refuri=target)
                else:
                    new_node = create_xref_node(raw_text, text, target)

            node += new_node

        self.add_name(node)
        return [node]


class TabAwareInclude(Include):
    def run(self):
        # update tab_width setting
        self.state.document.settings.tab_width = \
        self.state.document.settings.env.config.doxyrest_tab_width
        return Include.run(self)


#...............................................................................
#
#  Sphinx transforms
#

class RefTransform(Transform):
    default_priority = 100

    node_classes = {
        nodes.literal,
        nodes.strong,
        nodes.emphasis
    }

    def __init__(self, document, startnode=None):
        Transform.__init__(self, document, startnode)

        re_src = '(:c?ref:)'
        if document.settings.env.config.default_role == 'cref':
            re_src += '?' # explicit role is optional

        re_src += '`(.+?)(\s*<([^<>]*)>)?`'
        self.re_prog = re.compile(re_src)

    @staticmethod
    def node_filter(node):
        for node_class in RefTransform.node_classes:
            if isinstance (node, node_class):
                return node['classes'] == []

        return False

    def apply(self):
        for node in self.document.traverse(RefTransform.node_filter):
            code = node.astext()
            node.children = []
            pos = 0

            while True:
                match = self.re_prog.search(code, pos)
                if match is None:
                    plain_text = code[pos:]
                    if plain_text != "":
                        node += nodes.Text(plain_text, plain_text)
                    break

                plain_text = code[pos:match.start()]
                if plain_text != "":
                    node += nodes.Text(plain_text, plain_text)

                raw_text = match.group(0)
                role = match.group(1)
                text = match.group(2)
                target = match.group(4)

                if not role or role == ':cref:':
                    target = get_cref_target(text)

                node += create_xref_node(raw_text, text, target)
                pos = match.end()


#...............................................................................
#
#  Sphinx roles
#

def cref_role(typ, raw_text, text, lineno, inliner, options={}, content=[]):
    target = get_cref_target(text)

    if text.find(' ') == -1:
        node = nodes.literal(raw_text, '')
    else:
        node = nodes.inline(raw_text, '')

    node['classes'] += ['doxyrest-cref']
    node += create_xref_node(raw_text, text, target)
    return [node], []

def target_role(typ, raw_text, text, lineno, inliner, options={}, content=[]):
    node = create_target_node(raw_text, None, text, None, lineno, inliner.document)
    return [node], []


#...............................................................................
#
#  Sphinx source inputs
#

class TabAwareSphinxRSTFileInput(SphinxRSTFileInput):
    def read(self):
        # type: () -> StringList
        inputstring = SphinxBaseFileInput.read(self)
        tab_width = self.env.config.doxyrest_tab_width
        lines = string2lines(inputstring, convert_whitespace=True, tab_width=tab_width)

        content = StringList()
        for lineno, line in enumerate(lines):
            content.append(line, self.source_path, lineno)

        if self.env.config.rst_prolog:
            self.prepend_prolog(content, self.env.config.rst_prolog)
        if self.env.config.rst_epilog:
            self.append_epilog(content, self.env.config.rst_epilog)

        return content


#...............................................................................
#
#  Sphinx events
#

def on_builder_inited(app):
    app.config.html_static_path += [
        this_dir + '/css/doxyrest-pygments.css',
        this_dir + '/js/target-highlight.js'
    ]

    app.add_stylesheet('doxyrest-pygments.css')
    app.add_javascript('target-highlight.js')

    supported_themes = {
        'sphinx_rtd_theme',
        'sphinxdoc'
    }

    if app.config.html_theme in supported_themes:
        css_file = 'doxyrest-' + app.config.html_theme + '.css'
        app.config.html_static_path += [this_dir + '/css/' + css_file];
        app.add_stylesheet(css_file);

    for basedir, dirnames, filenames in os.walk(app.srcdir):
        if 'crefdb.py' in filenames:
            crefdb_path = os.path.join(basedir, 'crefdb.py')
            src = open(crefdb_path).read()
            ns = {}
            exec(src, ns)
            new_crefdb = ns['crefdb']
            if isinstance(new_crefdb, dict):
                global crefdb
                crefdb.update(new_crefdb)

#...............................................................................
#
#  Doxyrest extenstion setup
#

def setup(app):
    app.add_node(
        HighlightedText,
        html=(visit_highlighted_text_node, None),
        latex=(visit_highlighted_text_node, None)
        )

    app.add_role('cref', cref_role)
    app.add_role('target', target_role)
    app.add_config_value('doxyrest_tab_width', default=4, rebuild=True)
    app.add_config_value('doxyrest_cref_file', default=None, rebuild=True)
    app.registry.source_inputs['restructuredtext'] = TabAwareSphinxRSTFileInput
    directives.register_directive('ref-code-block', RefCodeBlock)
    directives.register_directive('include', TabAwareInclude)
    app.add_transform(RefTransform)
    app.connect('builder-inited', on_builder_inited)

    # register our docutils.conf

    prevConfig = None

    if 'DOCUTILSCONFIG' in os.environ:
        prevConfig = os.environ['DOCUTILSCONFIG']

    os.environ['DOCUTILSCONFIG'] = this_dir + '/conf/docutils.conf'

    if prevConfig:
        os.environ['DOCUTILSCONFIG'] += os.pathsep
        os.environ['DOCUTILSCONFIG'] += prevConfig
