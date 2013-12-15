Video streaming protocol
========================

+--------------+----------+---------------+
| Console Port | Pad Port | Direction     |
+==============+==========+===============+
| 50020        | 50120    | Console ↔ Pad |
+--------------+----------+---------------+

The video streaming protocol, also known as ``vstrm``, is used to stream
compressed game video data from the Wii U to a GamePad, or to stream camera
data from a GamePad to the Wii U. It is using H.264 with a custom wrapper
around the VCL instead of NAL.

Each packet has an 8 byte header (4 is also possible but never seen), followed
by an 8 byte extended header, followed by the compressed video data.

Chunking and stream reconstruction
----------------------------------
TODO

Protocol header
---------------

::

    struct VstrmHeader {
        u32 magic : 4;
        u32 packet_type : 2;
        u32 seq_id : 10;
        u32 init : 1;
        u32 frame_begin : 1;
        u32 chunk_end : 1;
        u32 frame_end : 1;
        u32 has_timestamp : 1;
        u32 payload_size : 11;

        // If has_timestamp is set (almost always)
        u32 timestamp : 32;
    };

.. image:: vstrm-header.png

TODO

Extended header
---------------

TODO

Decoding frames
---------------

TODO
