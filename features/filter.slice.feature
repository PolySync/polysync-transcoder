Feature: Slice filter plugin
    Scenario: Single record
        Given the command line: polysync-transcode --slice 1 -d verbose artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x1
        And stderr contains: index: 1

    Scenario: First five records
        Given the command line: polysync-transcode -d verbose --slice :5 artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x5
        And stderr contains: index: 0
        And stderr contains: index: 4

    Scenario: Middle five records
        Given the command line: polysync-transcode -d verbose --slice 10:15 artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x5
        And stderr contains: index: 10
        And stderr contains: index: 14

    Scenario: Last five records
        Given the command line: polysync-transcode -d verbose --slice 20: artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x5
        And stderr contains: index: 20
        And stderr contains: index: 24

    Scenario: Every other record from beginning
        Given the command line: polysync-transcode -d verbose --slice :2: artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x13
        And stderr contains: index: 0
        And stderr contains: index: 24 

    Scenario: Every third record from second
        Given the command line: polysync-transcode -d verbose --slice 2:3: artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x8
        And stderr contains: index: 2
        And stderr contains: index: 23 

    Scenario: Every third record from first through the 20th
        Given the command line: polysync-transcode -d verbose --slice 1:3:20 artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.header x7
        And stderr contains: index: 1
        And stderr contains: index: 19 


