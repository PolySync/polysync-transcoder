from behave import *

from hamcrest import *
import subprocess, shlex, ctypes, os

path = '../build'

@given('the command line: {cmdline}')
def step_impl(context, cmdline):

    # Turn off console coloring; it just interferes in the behavioral tests
    args = shlex.split(cmdline)
    args[0] = path + '/' + args[0] 
    args.insert(1, '--plain')  # turns of color escape codes
    args.insert(1, '--plugdir=../build/plugin') # configure runtime environment
    args.insert(1, '--plugdir=..')
    if 'loglevel' in context.config.userdata:
        args.insert(1, '--loglevel=' + context.config.userdata['loglevel'])

    context.response = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

@then('the return value is {retval}')
def step_impl(context, retval):
    # annoyingly, subprocess.run or the shell has made the exit status unsigned char
    assert_that(ctypes.c_int8(context.response.returncode).value, equal_to(int(retval)))

@then('stdout contains: {stdout}')
def step_impl(context, stdout):
    assert_that(context.response.stdout.decode('ascii'), contains_string(stdout))

@then('stderr contains: {stderr}')
def step_impl(context, stderr):
    assert_that(context.response.stderr.decode('ascii'), contains_string(stderr))

@then('stdout is empty')
def step_impl(context):
    assert_that(context.response.stdout, empty())

@then('stderr is empty')
def step_impl(context):
    assert_that(context.response.stderr, empty())

@then('stdout does not contain: {stdout}')
def step_impl(context, stdout):
    assert_that(context.response.stdout.decode('ascii'), is_not(contains_string(stdout)))

@then('the size of stdout is {size}')
def step_impl(context, size):
    assert_that(context.response.stdout, has_length(int(size)))

@then('stdout is identical to the first {size} bytes of {path}')
def step_impl(context, size, path):
    binary_truth = open(path, 'rb').read(int(size))
    assert_that(binary_truth, equal_to(context.response.stdout))

