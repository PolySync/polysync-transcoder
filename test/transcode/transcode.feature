Feature: Invalid parameters are handled gracefully
    Scenario: No arguments provided
        Given command line 
        """
        ./transcode
        """
        Then return value is -2
        Then stderr contains "no input files"

    Scenario: Help is requested
        Given command line
        """
        ./transcode -h
        """
        Then return value is 0
        Then stdout contains "Usage"

    Scenario: Input file does not exist
        Given command line
        """
        ./transcode missing.plog dump
        """
        Then return value is -2
        Then stderr contains "cannot open file"
