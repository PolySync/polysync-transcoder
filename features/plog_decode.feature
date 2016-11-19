Feature: Decode PLog records

    Background:
        Given an empty descriptor catalog

    Scenario: Decode a simple, valid blob
        Given the hex blob 0100000000000000 02000000 03000000
        And the descriptor catalog contains ps_byte_array_msg
        When decoding the type ps_byte_array_msg
        Then the result should be a ps_byte_array_msg:
            | name      | type       | value |
            | dest_guid | plog::guid | 1     |
            | data_type | uint32     | 2     |
            | payload   | uint32     | 3     |

    Scenario: Support the "skip" directive
        Given the hex blob 
        """
        01000000 
        02000000 
        03 
        00000000 # skip 4 bytes
        04 
        0500
        0600000000000000 
        """
        And the descriptor catalog contains ibeo.header
        When decoding the type ibeo.header 
        Then the result should be an ibeo.header:
            | name      | type   | value |
            | magic     | uint32 | 1     |
            | prev_size | uint32 | 2     |
            | size      | uint8  | 3     |
            | device_id | uint8  | 4     |
            | data_type | uint16 | 5     |
            | time      | uint64 | 6     |

    Scenario: Throw on unknown type
        Given the hex blob 0100000000000000 02000000 0300000000000000
        Then decoding the type ps_byte_array_msg should throw "no decoder for type ps_byte_array_msg"

    Scenario: nested type
        Given the hex blob 0100 02 03
        And the descriptor catalog contains nested_scanner
        And the descriptor catalog contains scanner_info
        When decoding the type nested_scanner
        Then the result should be a nested_scanner:
            | name          | type      | value |
            | start_time    | uint16    | 1     |

