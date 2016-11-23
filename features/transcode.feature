Feature: Invalid parameters are handled gracefully
    Scenario: No arguments provided
        Given the command line: transcode
        Then the return value is -2
        And stderr contains: no input files

    Scenario: Help is requested
        Given the command line: transcode -h
        Then the return value is 0
        And stdout contains: Usage

    Scenario: Input file does not exist
        Given the command line: transcode missing.plog dump
        Then the return value is -2
        And stderr contains: cannot open file

    Scenario Outline: Simple queries
        Given the command line: <cmdline>
        Then the return value is <retval>
        And stdout contains: <stdout>
        And stderr contains: <stderr>

        Examples:
            | cmdline                       | retval    | stdout    | stderr            |
            | transcode                     | -2        |           | no input files    |
            | transcode -h                  | 0         | Usage     |                   |
            | transcode missing.plog dump   | -2        |           | cannot open file  |
