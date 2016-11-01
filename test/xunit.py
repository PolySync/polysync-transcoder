import subprocess, shlex, os, sys, re
from bencodepy import decode
# import xml.etree.ElementTree as ET
from lxml import etree

class XUnitWriter:

    def __init__(self, name):
        self._name = name

    def create_suite(self, suites):
        self.close_suite()

        name = (b''.join(suites).decode('us-ascii'))
        name = re.sub(r'\((.*)\)', r'\1', name)
        name = re.sub(' ', '_', name)
        e = etree.Element('testsuite')
        e.set('name', name)

        self.errors = 0
        self.failures = 0
        self.tests = 0
        self.time = 0.0 

        return e

    def create_testcase(self, msg):
        if self._suite is None:
            self._suite = self.create_suite(msg[b'test'][b'suites'])

        self.close_testcase()
        testname = msg[b'test'][b'test'].decode('us-ascii')
        name = re.sub(r'\((.*)\)', r'\1', testname)
        name = re.sub(' ', '_', name)
        e = etree.Element('testcase')
        e.set('name', name)
        return e

    def started_run(self, msg):
        self._suites = etree.Element('testsuites')
        self._suite = None
        self._testcase = None

    def ended_run(self, msg):
        pass

    def close_suite(self):
        self.close_testcase()
        if self._suite is None:
            return
        self._suite.set('tests', '{}'.format(self.tests))
        self._suite.set('failures', '{}'.format(self.failures))
        self._suite.set('errors', '{}'.format(self.errors))
        self._suite.set('time', '{}'.format(self.time))
        self._suites.append(self._suite)
        self._suite = None

    def close_testcase(self):
        if self._testcase is None:
            return
        self._suite.append(self._testcase)
        self._testcase = None

    def started_suite(self, msg):
        self._suite = self.create_suite(msg[b'suites'])

    def ended_suite(self, msg):
        self.close_suite()
        
    def __enter__(self):
        return self

    def __exit__(self, type, value, tb):
        self.close_suite()

        tree = etree.ElementTree(self._suites)
        with open('{}.xml'.format(self._name), 'w') as xml:
            xml.write(etree.tostring(self._suites, pretty_print=True).decode('us-ascii'))

    def started_test(self, msg):
        # This signal from mettle appears unreliable.  Sometimes it happens, normally not.
        # name = msg[b'test'][b'test']
        # print ('start', name)

        # self._testcase = self.create_testcase(msg)
        # self.tests += 1
        pass

    def passed_test(self, msg):
        self._testcase = self.create_testcase(msg)
        self.tests += 1
        self.close_testcase()

        self.time += float(msg[b'duration'])
        output = msg[b'output']
        stderr = output[b'stderr_log']
        stdout = output[b'stdout_log']
        name = msg[b'test'][b'test']
        print('passed', name)

    def failed_test(self, msg):
        self._testcase = self.create_testcase(msg)
        self.failures += 1
        self.time += float(msg[b'duration'])

        e = etree.Element('error')
        e.set('message', msg[b'message'])
        e.text = msg[b'message']
        self._testcase.append(e)
        self.close_testcase()

        output = msg[b'output']
        stderr = output[b'stderr_log']
        stdout = output[b'stdout_log']
        name = msg[b'test'][b'test']
        print('failed', name)


    def __call__(self, msg):
        # visit the member function named by the event type.
        method = msg[b'event'].decode('us-ascii')
        print(method)
        XUnitWriter.__dict__[method](self, msg)

for executable in ['decode', 'description']:
    with XUnitWriter(executable) as xunit:
        read_fd, write_fd = os.pipe()
        args = shlex.split('./test_{} --no-subproc'.format(executable))
        args.append('--output-fd={}'.format(write_fd))
        subproc = subprocess.Popen(args, pass_fds=(write_fd,))
        os.close(write_fd)

        msg = os.read(read_fd, 16384)
        while msg: 
            xunit(decode(msg));
            msg = os.read(read_fd, 16384)

        subproc.wait()
