import ast, string, re

class Peeker:
    def __init__(self, str):
        self.str = str
        self.offset = -1
        self.peek = ''
        self.read()
    def read(self, n=1):
        peek = self.peek
        if peek is None:
            raise Exception('Peeker out of chars')
        self.offset += n
        self.peek = None if self.offset == len(self.str) else self.str[self.offset]
        return peek

    def peek_more(self, n):
        offset = self.offset + n
        return None if offset == len(self.str) else self.str[offset]

    def peek_re(self, re):
        return re.match(self.str, self.offset)

    def rewind(self):
        self.offset -= 1
        self.peek = self.str[self.offset]

AsyncMarker = 'AsyncMarker'

spaces_except_newline = {' ', '\t', '\r'}
spaces = {' ', '\t', '\r', '\n'}
bareword_enders = spaces | {')', ']', '{', '}', '<', '>', '|', '`', '#', ';', '\n', None}
var_enders = bareword_enders | {'(', '[', '$'}
bareword_enders_expr = '($|[ \t\r\n)\]{}<>|`#;\n])'
bareword_enders_re = re.compile(bareword_enders_expr)
escape_chars = {
    'a': '\a', 'b': '\b', 'f': '\f', 'n': '\n', 'r': '\r', 't': '\t', 'v': '\v'
}
letters = set(string.ascii_letters)
digits = set(string.digits)
hexadigits = set(string.digits + 'abcdef')
symbol_cmd_enders = bareword_enders | {'[', '"', "'", '$'} | letters | digits

# ) ] } done
# in C the peek_exec stuff needs to be one big function

always_enders_expr = '\)|\]|}|$|done' + bareword_enders_expr
always_enders_re = re.compile(always_enders_expr)
always_enders_and_semicolon_re = re.compile(';|\n|' + always_enders_expr)
always_enders_and_semicolon_pipe_re = re.compile('\||;|\n|' + always_enders_expr)


def all_numeric(x):
    return bool(re.match('^[0-9]+$', x))

class ParseError(Exception):
    def __init__(self, pkr, msg):
        msg = '%s\nremaining: %r' % (msg, pkr.str[pkr.offset:pkr.offset+20])
        Exception.__init__(self, msg)

class Parser(Peeker):
    def translate_escape(self):
        assert self.read() == '\\'
        c = self.read()
        if c == 'x':
            # hex
            hexstr = ''
            while self.peek in hexadigits:
                hexstr += self.read()
            try:
                hexnum = int(hexstr, 16)
            except ValueError:
                raise ParseError(self, 'bad hex code \\x%s' % hexstr)
            if hexnum > 255:
                raise ParseError(self, 'bad non-unicode escape code for character %d' % hexnum)
            return chr(hexnum)
        elif c in escape_chars:
            return escape_chars[c]
        elif c in digits:
            # octal
            octstr = c
            while self.peek in digits:
                octstr += self.read()
            try:
                octnum = int(octstr, 8)
            except ValueError:
                raise ParseError(self, 'bad octal code \\%s' % octstr)
            if octnum > 255:
                raise ParseError(self, 'bad non-unicode escape code for character %d' % octnum)
            return chr(octnum)
        elif c in letters:
            raise ParseError(self, 'unknown alphabetic escape code \\%s' % c)
        else:
            return c
    def parse_dblquote(self):
        assert self.read() == '"'
        bits = []
        the_str = ''
        while self.peek != '"':
            c = self.read()
            if c == '\\':
                self.rewind()
                the_str += self.translate_escape()
            elif c == '$':
                if the_str:
                    bits.append(ast.Lit(the_str))
                    the_str = ''
                if self.peek == '[':
                    # $[] in quotes == []
                    bits.append(self.parse_exec())
                elif self.peek == '{':
                    # ${foo`bar} in quotes == $foo`bar
                    self.peek = '$'
                    bit = self.parse_var()
                    if self.peek == '`':
                        bit = self.parse_path(bit, quoted_mode=True)
                    if self.peek != '}':
                        raise ParseError(self, '${} not closed')
                    bits.append(bit)
                else:
                    self.rewind()
                    bit = self.parse_var()
                    if self.peek == '`':
                        bit = self.parse_path(bit)
                    bits.append(bit)
            else:
                the_str += c
        assert self.read() == '"'
        if the_str or len(bits) == 0:
            bits.append(ast.Lit(the_str))
        return bits


    def parse_singlequote(self):
        assert self.read() == "'"
        out = ''
        while self.peek != "'":
            out += self.read()
        self.read() # '
        return out

    def parse_bareword_out_expr(self):
        return self.parse_bareword() # XXX

    def parse_bareword(self, is_cmd=False, async_ok=False):
        bit = self.parse_bareword_nopath()
        if self.peek == '`':
            bit = self.parse_path(bit)
        return bit

    def parse_bareword_nopath(self, is_cmd=False, async_ok=False):
        bits = []
        the_str = ''
        is_glob = False
        # special rule to, e.g., allow '=x 1' rather than '= x 1'
        # also allows [* x 2] to not be interpreted as a glob
        # < is still ambiguous though, so must be lt
        if is_cmd and self.peek not in symbol_cmd_enders:
            while self.peek not in symbol_cmd_enders:
                the_str += self.read()
            return ast.Lit(the_str)
        # & at beginning of word (and word is not &) = reference
        if self.peek == '&':
            self.read()
            if self.peek in bareword_enders:
                # it's just &
                if not async_ok:
                    raise ParseError('& not accepted here')
                return AsyncMarker
            elif self.peek == '&':
                raise ParseError(self, 'double &')
            rest = self.parse_bareword()
            if isinstance(rest, (Var, Index)):
                assert not rest.is_ref
                rest.is_ref = True
                return rest
            else:
                raise ParseError(self, '& must be followed by an index or variable expression')
        elif self.peek == '*':
            the_str += '*'
            is_glob = True
            self.read()
            if self.peek in '([{$':
                return ast.Expand(self.parse_bareword())
            # else, it's a glob
        elif self.peek_re(re.compile('{|do' + bareword_enders_expr)):
            # block
            self.read((1, 2)[self.peek == 'd'])
            return ast.Block(self.parse_exec_(re.compile('}|done')))
        elif self.peek == '(':
            return self.parse_list()

        def add_this_bit():
            nonlocal the_str, is_glob
            if the_str:
                if is_glob:
                    bits.append(self.parse_glob(the_str))
                else:
                    bits.append(ast.Lit(the_str))
                the_str = ''
        while self.peek not in bareword_enders:
            if self.peek == '[':
                add_this_bit()
                bits.append(self.parse_exec())
            elif self.peek == '"':
                bits2 = self.parse_dblquote()
                if isinstance(bits2[0], ast.Lit):
                    the_str += bits2.pop(0).str
                if bits2:
                    add_this_bit()
                    bits.extend(bits2)
            elif self.peek == "'":
                the_str += self.parse_singlequote()
            elif self.peek == '$':
                add_this_bit()
                bits.append(self.parse_var())
            elif self.peek == '\\':
                the_str += self.translate_escape()
            else:
                c = self.read()
                if c in '*?':
                    is_glob = True
                the_str += c
        add_this_bit()

        if len(bits) == 0:
            raise ParseError(self, "shouldn't get to parse_bareword() with only a bareword_ender in store")
        elif len(bits) == 1:
            return bits[0]
        else:
            return ast.Exec(ast.Lit('str-concat'), bits, redirs=[], is_async=False)

    def skip_white(self):
        while True:
            while self.peek in spaces_except_newline:
                self.read()
            if self.peek == '#':
                self.parse_comment()
            else:
                break

    def parse_exec(self):
        assert self.read() == '['
        return self.parse_exec_(re.compile(']'))

    def parse_exec_(self, ender):
        bits = []
        while not self.peek_re(always_enders_re):
            bit = self.parse_exec_pipeline()
            if self.peek in {';', '\n'}:
                self.read()
            if bit is None:
                continue
            bits.append(bit)
        m = self.peek_re(ender)
        if not m:
            raise ParseError(self, 'Mismatched brackets - got %s, expected %s' % (self.peek, (ender or '<end of data>')))
        if self.peek is not None:
            self.read(len(m.group(0)))
        return bits[0] if len(bits) == 1 else ast.Sequence(bits)

    def parse_exec_pipeline(self):
        bits = []
        had_empty = False
        while not self.peek_re(always_enders_and_semicolon_re):
            bit = self.parse_exec_single()
            if self.peek == '|':
                self.read()
            if bit is None:
                had_empty = True
                continue
            bits.append(bit)
        if had_empty and len(bits) > 1:
            raise ParseError(self, 'empty command in a pipeline')
        if len(bits) == 0:
            return None
        return bits[0] if len(bits) == 1 else ast.Pipe(bits)

    def parse_exec_single(self):
        cmd = None
        args = []
        redirs = []
        is_async = False
        while True:
            self.skip_white()
            if self.peek_re(always_enders_and_semicolon_re):
                break
            if self.peek_re(re.compile('<|>|->')):
                redirs.extend(self.parse_redir(None))
                continue

            node = self.parse_bareword(is_cmd=(cmd is None), async_ok=True)
            assert node is not None
            if node is AsyncMarker:
                is_async = True
                break # [foo & bar] = [foo &; bar] like bash
            # yucky
            if isinstance(node, ast.Lit) and all_numeric(node.str) and self.peek_re(re.compile('<|>|->')):
                redirs.extend(self.parse_redir(int(node.str)))
                continue
            # ok, it's an arg
            if cmd is None:
                cmd = node
            else:
                args.append(node)

        if cmd is None:
            if not redirs:
                # literally empty
                return None
            cmd = ast.Lit('cat')

        return ast.Exec(cmd, args, redirs, is_async)

    def parse_redir(self, fd):
        out_and_err = False
        c = self.read()
        if c == '<':
            dir = ast.IN
            if fd is None:
                fd = 0
            if self.peek == '-':
                self.read()
                endpoint = self.parse_bareword()
            else:
                filename = self.parse_bareword()
                endpoint = ast.Exec(ast.Lit('read-file'), [filename], [], is_async=False)
        elif c == '>':
            dir = ast.OUT
            if fd is None:
                fd = 1
            if self.peek == '&':
                self.read()
                out_and_err = True
            filename = self.parse_bareword()
            if out_and_err and isinstance(filename, ast.Lit) and all_numeric(filename.str):
                # it's actually a>&b
                return [ast.RedirOtherFd(fd, int(filename.str))]
            endpoint = ast.Block(ast.Exec(ast.Lit('write-to-file'), [filename], [], is_async=False))
        else:
            dir = ast.OUT
            assert c == '-' and self.read() == '>'
            if self.peek == '&':
                self.read()
                out_and_err = True
            endpoint = self.parse_bareword_out_expr()
        bit = ast.Redir(fd, dir, endpoint)
        if out_and_err:
            return [bit, ast.RedirOtherFd(2, 1)]
        else:
            return [bit]

    def parse_list(self):
        assert self.read() == '('
        bits = []
        while True:
            # skip white, *including* newline
            while self.peek in spaces:
                self.read()
            if self.peek == '#':
                self.parse_comment()
                continue
            if self.peek_re(always_enders_re):
                break
            bits.append(self.parse_bareword())
        if self.peek != ')':
            raise ParseError(self, 'Mismatched brackets - got %s, expected )' % self.peek)
        self.read()
        return ast.List(bits)

    def parse_var(self):
        assert self.read() == '$'
        name = ''
        while self.peek not in var_enders:
            # allow unicodey escapes in var names
            if self.peek_re(re.compile('\\[xuU]')):
                name += self.translate_escape()
            elif self.peek == '\\':
                break
            else:
                name += self.read()
        return ast.Var(False, name)

    def parse_braced_key(self):
        assert self.read() == '{'
        bit = self.parse_bareword()
        if self.peek != '}':
            raise ParseError(self, 'Mismatched brackets - got %s, expected }' % self.peek)
        self.read()
        return bit

    def parse_path(self, bit, quoted_mode=False):
        assert self.peek == '`'
        keys = []
        while self.peek == '`':
            self.read()
            if quoted_mode:
                assert False # XXX
            else:
                if self.peek == '{':
                    key = self.parse_braced_key()
                elif self.peek in bareword_enders:
                    key = ast.Lit('')
                else:
                    key = self.parse_bareword_nopath()
            keys.append(key)
        return ast.Path(False, bit, keys)

    def parse_comment(self):
        assert self.read() == '#'
        while self.read() != '\n':
            pass

    def parse_outer(self):
        return self.parse_exec_(re.compile('$'))

if __name__ == '__main__':
    import sys
    if len(sys.argv) >= 2:
        inp = open(sys.argv[1]).read()
    else:
        inp = "foo'bar'\"baz\"` [bar bar >&1]\nhi do = x 6 } {=x 5} (foo bar)`3``4`{$foo`4}"
    parsed = Parser(inp).parse_outer()
    print(parsed)
    print()
    print(parsed.repr())
