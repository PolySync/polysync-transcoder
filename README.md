### Print the first record in the file "input.plog":

The argument '--last 1' stops the program after the first log record.  The plugin 'dump' will print the contents of the file to the console.

`transcode --last 1 input.plog dump`

* log_record:
    * index: 0
    * size: 150
    * prev_size: 0
    * timestamp: 1464470621734607
    * msg_header:
        * type: 16
        * timestamp: 1464470621734607
        * src_guid: 0
    * ps_byte_array_msg:
        * dest_guid: 0
        * data_type: 160
        * payload: 114
    * ibeo.header:
        * magic: 2952708290
        * prev_size: 5458
        * size: 0
        * device_id: 0
        * data_type: 10247
        * time: 15777401601108159419
    * ibeo.vehicle_state:
        * timestamp: 15777401601954560034
        * distance_x: 784592
        * distance_y: -2097145
        * course_angle: 0.623269
        * longitudinal_velocity: 0.01
        * ...

