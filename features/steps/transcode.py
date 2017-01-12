from behave import *

from hamcrest import *
import subprocess, shlex, ctypes, os

@given('the command line: {cmdline}')
def step_impl(context, cmdline):

    # Turn off console coloring; it just interferes in the behavioral tests
    args = shlex.split(cmdline)
    args.insert(1, '--plain')  # turns of color escape codes
    if 'loglevel' in context.config.userdata:
        args.insert(1, '--loglevel=' + context.config.userdata['loglevel'])

    context.response = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    context.response.wait()
    context.stdout = context.response.stdout.read().decode('ascii')
    context.stderr = context.response.stderr.read().decode('ascii')

@then('the return value is {retval}')
def step_impl(context, retval):
    # annoyingly, subprocess.run or the shell has made the exit status unsigned char
    assert_that(ctypes.c_int8(context.response.returncode).value, equal_to(int(retval)))

@then('stdout contains: {stdout}')
def step_impl(context, stdout):
    assert_that(context.stdout, contains_string(stdout))

@then('stderr contains: {stderr}')
def step_impl(context, stderr):
    assert_that(context.stderr, contains_string(stderr))

@then('stdout is empty')
def step_impl(context):
    assert_that(context.stdout, empty())

@then('stderr is empty')
def step_impl(context):
    assert_that(context.stderr, empty())

@then('stdout does not contain: {stdout}')
def step_impl(context, stdout):
    assert_that(context.stdout, is_not(contains_string(stdout)))

@then('the size of stdout is {size}')
def step_impl(context, size):
    assert_that(context.stdout, has_length(int(size)))

@then('stdout is identical to the first {size} bytes of {path}')
def step_impl(context, size, path):
    binary_truth = open(path, 'rb').read(int(size))
    assert_that(binary_truth, equal_to(context.stdout))

