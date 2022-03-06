#!/usr/bin/env plask

import sys, unittest, traceback
import os.path

import plask


class PlaskTestResult(unittest.TextTestResult):
    '''Format test failures/exceptions if IDE-friendly form'''

    STDOUT_LINE = '\nStdout:\n%s'
    STDERR_LINE = '\nStderr:\n%s'

    def _exc_info_to_string(self, err, test):
        """Converts a sys.exc_info()-style tuple of values into a string."""
        exctype, value, tb = err
        # Skip test runner traceback levels
        while tb and self._is_relevant_tb_level(tb):
            tb = tb.tb_next

        if exctype is test.failureException:
            # Skip assert*() traceback levels
            length = self._count_relevant_tb_levels(tb)
            tb_lines = traceback.extract_tb(tb, length)
        else:
            tb_lines = traceback.extract_tb(tb)
            #msg_lines = traceback.format_exception(exctype, value, tb)

        msg_lines = []
        for filename, lineno, function, text in tb_lines[:-1]:
            msg_lines.append('%s:%d\n: in "%s": %s\n' % (filename, lineno, function, text))
        filename, lineno, function, text = tb_lines[-1]
        msg_lines.append('%s:%d: %s in "%s": %s\n>   %s\n' % (filename, lineno, exctype.__name__, function, value, text))

        if self.buffer:
            output = sys.stdout.getvalue()
            error = sys.stderr.getvalue()
            if output:
                if not output.endswith('\n'):
                    output += '\n'
                msg_lines.append(STDOUT_LINE % output)
            if error:
                if not error.endswith('\n'):
                    error += '\n'
                msg_lines.append(STDERR_LINE % error)

        return ''.join(msg_lines)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: ", sys.executable, "-m runtest test_file.(py|xpl) [tests...]", file=sys.stderr)
        sys.exit(127)

    sys.path.insert(0, os.path.dirname(sys.argv[1]))

    __name__, ext = os.path.splitext(os.path.basename(sys.argv[1]))

    if ext == '.py':

        exec(compile(open(sys.argv[1], encoding='utf8').read(), sys.argv[1], 'exec'), globals(), locals())

        tests = sys.argv[2:]
        if not tests:
            for m in dict(globals()).values():
                try:
                    if issubclass(m, unittest.TestCase):
                        tests.append(m)
                except TypeError:
                    pass
        else:
            tests = [globals()[m] for m in tests]

    elif ext == '.xpl':

        __manager__ = plask.Manager()
        __manager__.load(sys.argv[1])
        __globals = globals().copy()
        __manager__.export(__globals)
        __globals.update(__manager__.defs)
        __locals = dict()
        script = "#coding: utf8\n" + "\n"*(__manager__._scriptline-1) + __manager__.script

        exec(compile(script, sys.argv[1], 'exec'), __globals, __locals)

        tests = sys.argv[2:]
        if not tests:
            for m in __locals.values():
                try:
                    if issubclass(m, unittest.TestCase):
                        tests.append(m)
                except TypeError:
                    pass
        else:
            tests = [__locals[m] for m in tests]

    else:
        print("Unrecognized file", sys.argv[1], file=sys.stderr)
        sys.exit(127)

    errors = 0

    for m in tests:
        suite = unittest.TestLoader().loadTestsFromTestCase(m)
        runner = unittest.TextTestRunner()
        runner.resultclass = PlaskTestResult
        result = runner.run(suite)
        errors += len(result.errors) + len(result.failures)

    sys.exit(errors)
