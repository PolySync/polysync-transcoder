Feature: Invalid parameters are handled gracefully
    Scenario: No arguments provided
        Given the command line: polysync-transcode
        Then the return value is -2
        And stdout is empty
        And stderr contains: no input file

    Scenario: Help is requested
        Given the command line: polysync-transcode -h
        Then the return value is 0
        And stdout contains: Usage
        And stderr is empty

    Scenario: Invalid argument
        Given the command line: polysync-transcode --verschwinden ibeo.25.plog 
        Then the return value is -1
        And stdout is empty
        And stderr contains: unrecognised option

    Scenario: Input file does not exist
        Given the command line: polysync-transcode missing.plog dump
        Then the return value is -2
        And stdout is empty
        And stderr contains: cannot open file

    Scenario: Input file is not plog
        Given the command line: polysync-transcode /proc/cpuinfo
        Then the return value is -2
        And stdout is empty
        And stderr contains: not a plog
