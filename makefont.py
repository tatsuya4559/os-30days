class Compiler:
    def __init__(self, lines):
        self.lines = lines
        self.line_len = len(lines)
        self.linenr = 0

    def _advance(self):
        if self.linenr < self.line_len - 1:
            self.linenr += 1
            return True
        return False

    @property
    def current_line(self):
        return self.lines[self.linenr]

    def compile(self):
        results = []
        while True:
            if self.current_line.startswith('char'):
                results.append(self.compile_char())
            if not self._advance():
                break
        return results

    def compile_char(self):
        _, idx = self.current_line.split(' ')
        results = []
        for _ in range(16):
            self._advance()
            bin_str = self.current_line.replace('.', '0').replace('*', '1')
            hex_str = hex(int(bin_str, 2))
            results.append(hex_str)
        return '    {%s},\n' % ','.join(results)


def main():
    with open('hankaku.txt', 'r') as fp:
        src = fp.readlines()

    compiler = Compiler(src)
    results = compiler.compile()

    with open('font.h', 'w') as fp:
        fp.write('unsigned char fonts[256][16] = {\n')
        fp.writelines(results)
        fp.write('};')


if __name__ == "__main__":
    main()
