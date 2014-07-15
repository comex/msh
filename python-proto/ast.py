import collections, re, curses.ascii

class Node(object): pass

# *expr - only valid inside Exec or List
class Expand(collections.namedtuple('Expand',
    'sub'
    ), Node):
    def repr(self):
        return '*' + self.sub.repr()

class Lit(collections.namedtuple('Lit', [
    'str',
    ]), Node):
    def repr(self):
        fancy = False

        out = ''
        for char in self.str:
            # real version should be more unicode-respectful
            if not curses.ascii.isprint(char) or (curses.ascii.isspace(char) and char != ' '):
                fancy = True
                out += repr(char)[1:-1]
            elif char in '"\\$':
                fancy = True
                out += '\\' + char
            elif char in ' `()[]&<>~*?|':
                fancy = True
                out += char
            else:
                out += char
        if fancy:
            out = '"' + out + '"'
        return out

class Var(collections.namedtuple('Var', [
    'is_ref', 'name',
    ]), Node):
    def repr(self):
        return ('&' if self.is_ref else '') + '$' + self.name

# &foo`bar```
# keys
class Path(collections.namedtuple('Path', [
    'is_ref', 'base', 'keys',
    ]), Node):
    def repr(self):
        out = ''
        if self.is_ref:
            out += '&'
        out += self.base.repr()
        print(self)
        for key in self.keys:
            out += '`'
            if isinstance(key, Path):
                out += '{' + key.repr() + '}'
            else:
                out += key.repr()
        return out

# <file
# 2>file
# ->var  /  ->$&var
# <-text
# 2>&1
# {
IN, OUT = 'IN', 'OUT'

class Redir(collections.namedtuple('Redir', [
    'fd', 'dir', 'endpoint',
    ])): # not a Node
    def repr(self):
        out = ''
        if self.fd != (0, 1)[self.dir is OUT]:
            out += '%d' % self.fd
        out += ('<-', '->')[self.dir is OUT]
        out += self.endpoint.repr()
        return out

class RedirOtherFd(collections.namedtuple('RedirOtherFd', [
    'fd', 'fd2',
    ])):
    def repr(self):
        return ('' if self.fd == 1 else str(self.fd)) + \
            '>&' + \
            str(self.fd2)
# }

# [foo expr expr... redirs... &?]
class Exec(collections.namedtuple('Exec', [
    'cmd', 'args', 'redirs', 'is_async',
    ]), Node):
    def repr(self):
        return '[' + \
            ' '.join(x.repr() for x in [self.cmd] + self.args + self.redirs) + \
            (' &' if self.is_async else '') + \
            ']'

# (expr expr...)
class List(collections.namedtuple('List',
    'elems',
    ), Node):
    def repr(self):
        return '(' + ' '.join(elem.repr() for elem in self.elems) + ')'

# {cmds...}
class Block(collections.namedtuple('Block',
    'exec',
    ), Node):
    def repr(self):
        return '{' + self.exec.repr()[1:-1] + '}'

# [cmd | cmd2 | cmd3]
# cmds must be Execs
class Pipe(collections.namedtuple('Pipe',
    'cmds',
    ), Node):
    def repr(self):
        assert all(isinstance(cmd, Exec) for cmd in self.cmds)
        return '[' + ' | '.join(cmd.repr()[1:-1] for cmd in self.cmds) + ']'

# [cmd; cmd2; cmd3]
# lower precedence than pipes, obviously; also must be execs
class Sequence(collections.namedtuple('Sequence',
    'cmds',
    ), Node):
    def repr(self):
        assert all(isinstance(cmd, Exec) for cmd in self.cmds)
        return '[' + '; '.join(cmd.repr()[1:-1] for cmd in self.cmds) + ']'

if __name__ == '__main__':
    x = List([
        Lit('foo'),
        Expand(Lit('bar')),
        Var(False, 'var'),
        Var(True, 'ref'),
        Path(True, Var(False, 'var'), [
            Path(False, Lit('foo'), [Lit('bar')]),
            Lit('hi there'),
        ]),
        Sequence([
            Exec(
                Lit('some cmd'),
                [Lit('foo'), Lit('->'), Lit('&')],
                [
                    Redir(0, IN, Pipe([
                        Exec(Lit('cat'), [Lit('filename')], [], False),
                        Exec(Lit('cat'), [Lit('filename2')], [], False),
                    ])),
                    Redir(2, OUT, Var(True, 'err')),
                    RedirOtherFd(3, 2),
                ],
                is_async=True,
            ),
            Exec(Lit('next cmd'), [], [], is_async=True),
        ]),
    ])
    print(x.repr())
