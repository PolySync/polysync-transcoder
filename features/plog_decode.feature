Feature: Decode PLog records

    Background:
        Given a descriptor catalog

    Scenario:
        Given the hex blob 0100000000000000 02000000 03000000
        When decoding the type ps_byte_array_msg
        Then the result should be a ps_byte_array_msg:
            | name      | type       | value |
            | dest_guid | plog::guid | 1     |
            | data_type | uint32     | 2     |
            | payload   | uint32     | 4     |
