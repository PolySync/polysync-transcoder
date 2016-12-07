from behave import *
from compare import expect
import subprocess, shlex, ctypes, os

use_step_matcher("re")
path = '../build/cmdline'

@given('the command line: (?P<cmdline>.+)')
def step_impl(context, cmdline):

    # Turn off console coloring; it just interferes in the behavioral tests
    args = shlex.split(cmdline)
    args[0] = path + '/' + args[0] 
    args.insert(1, '--plain')

    # set up the runtime environment
    os.environ["POLYSYNC_TRANSCODER_LIB"] = ".."

    context.response = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

@then('the return value is (?P<retval>.+)')
def step_impl(context, retval):
    # annoyingly, subprocess.run or the shell has made the exit status unsigned char
    expect(ctypes.c_int8(context.response.returncode).value).to_equal(int(retval))

@then('stdout contains: (?P<stdout>.*)')
def step_impl(context, stdout):
    print (context.response.stdout)
    expect(context.response.stdout.decode('ascii')).to_contain(stdout)

@then('stderr contains: (?P<stderr>.*)')
def step_impl(context, stderr):
    expect(context.response.stderr.decode('ascii')).to_contain(stderr)

@then('stdout is empty')
def step_impl(context):
    expect(not context.response.stdout)

@then('stderr is empty')
def step_impl(context):
    expect(not context.response.stderr)
