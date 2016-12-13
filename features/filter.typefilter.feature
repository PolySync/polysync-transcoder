Feature: Typefilter plugin
    Scenario: No filter enabled
        Given the command line: polysync-transcode artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.scan_data x8
        And stdout contains: ibeo.vehicle_state x9
        And stdout contains: ibeo.header x25

    Scenario: One type enabled
        Given the command line: polysync-transcode --type ibeo.scan_data artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.scan_data x8
        And stdout does not contain: ibeo.vehicle_state 
        And stdout does not contain: ibeo.header

    Scenario: Two types enabled
        Given the command line: polysync-transcode --type ibeo.scan_data --type ibeo.vehicle_state artifacts/ibeo.25.plog
        Then the return value is 0
        And stdout contains: ibeo.scan_data x8
        And stdout contains: ibeo.vehicle_state x9
        And stdout does not contain: ibeo.header



