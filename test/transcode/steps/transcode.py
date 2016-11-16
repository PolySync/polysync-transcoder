from behave import *
from compare import expect
import subprocess, shlex

path = '../../build/transcode/'

@given('command line')
def step_impl(context):
    args = shlex.split(context.text)
    context.response = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)    
    print ('status',context.response.returncode)

@then('return value is {retval}')
def step_impl(context, retval):
    # annoyingly, subprocess.run or the shell has made the exit status unsigned char
    expect(context.response.returncode).to_equal(int(retval) & 0xFF)

@then('stdout contains "{spew}"')
def step_impl(context, spew):
    expect(context.response.stdout.decode('ascii')).to_contain(spew)

@then('stderr contains "{spew}"')
def step_impl(context, spew):
    expect(context.response.stderr.decode('ascii')).to_contain(spew)

