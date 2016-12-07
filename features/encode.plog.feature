Feature: Encode PLog files
    Scenario: First record
        Given the command line: polysync-transcode --slice 0 artifacts/ibeo.25.plog plog -n tmp.1.plog

