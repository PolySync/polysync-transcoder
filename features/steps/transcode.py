from behave import *

# Started out using compare, which turns out to be old and sucky. Need to finish moving to hamcrest.
from compare import expect
from hamcrest import *
import subprocess, shlex, ctypes, os

path = '../build/cmdline'

@given('the command line: {cmdline}')
def step_impl(context, cmdline):

    # Turn off console coloring; it just interferes in the behavioral tests
    args = shlex.split(cmdline)
    args[0] = path + '/' + args[0] 
    args.insert(1, '--plain')
    if 'loglevel' in context.config.userdata:
        args.insert(1, '--loglevel=' + context.config.userdata['loglevel'])

    # set up the runtime environment
    os.environ["POLYSYNC_TRANSCODER_LIB"] = "..;../build/plugin"

    context.response = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

@then('the return value is {retval}')
def step_impl(context, retval):
    # annoyingly, subprocess.run or the shell has made the exit status unsigned char
    expect(ctypes.c_int8(context.response.returncode).value).to_equal(int(retval))

@then('stdout contains: {stdout}')
def step_impl(context, stdout):
    expect(context.response.stdout.decode('ascii')).to_contain(stdout)

@then('stderr contains: {stderr}')
def step_impl(context, stderr):
    expect(context.response.stderr.decode('ascii')).to_contain(stderr)

@then('stdout is empty')
def step_impl(context):
    expect(not context.response.stdout)

@then('stderr is empty')
def step_impl(context):
    expect(not context.response.stderr)

@then('stdout does not contain: {stdout}')
def step_impl(context, stdout):
    assert_that(context.response.stdout.decode('ascii'), is_not(contains_string(stdout)))

@then('the size of stdout is {size}')
def step_impl(context, size):
    expect(len(context.response.stdout)).to_equal(int(size))

@then('stdout is identical to the first {size} bytes of {path}')
def step_impl(context, size, path):
    ref = open(path, 'rb').read(int(size))
    expect(ref).to_equal(context.response.stdout)

