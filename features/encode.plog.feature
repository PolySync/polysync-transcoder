Feature: Encode PLog files
    Scenario: First record; it is an ibeo.vehicle_state
        Given the command line: polysync-transcode --slice 0 artifacts/ibeo.25.plog plog
        Then the return value is 0
        And the size of stdout is 1356
        And stdout is identical to the first 1356 bytes of artifacts/ibeo.25.plog

    Scenario: First two records; the second is an ibeo.scan_data
        Given the command line: polysync-transcode --slice :2 artifacts/ibeo.25.plog plog
        Then the return value is 0
        And the size of stdout is 28216
        And stdout is identical to the first 28216 bytes of artifacts/ibeo.25.plog

    Scenario: First three records; the third is undescribed
        Given the command line: polysync-transcode --slice :3 artifacts/ibeo.25.plog plog
        Then the return value is 0
        And the size of stdout is 33746
        And stdout is identical to the first 33746 bytes of artifacts/ibeo.25.plog

