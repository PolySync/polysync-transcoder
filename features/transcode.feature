Feature: Invalid parameters are handled gracefully
    Scenario: No arguments provided
        Given the command line: transcode
        Then the return value is -2
        And stdout is empty
        And stderr contains: no input files

    Scenario: Help is requested
        Given the command line: transcode -h
        Then the return value is 0
        And stdout contains: Usage
        And stderr is empty

    Scenario: Invalid argument
        Given the command line: transcode --verschwinden
        Then the return value is -2
        And stdout is empty
        And stderr contains: invalid argument

    Scenario: Input file does not exist
        Given the command line: transcode missing.plog dump
        Then the return value is -2
        And stdout is empty
        And stderr contains: cannot open file

Feature: Datamodel queries
    Scenario: 
