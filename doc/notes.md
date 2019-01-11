RTSP
=====
-   RFC 2326 
-   "a server maintains
    a session labeled by an identifier"
-   Both an RTSP server and client can issue requests
-   RTSP is defined to use ISO 10646 (UTF-8)
-   The Request-URI always contains the absolute URI. Because of
       backward compatibility with a historical blunder, HTTP/1.1 [2]
       carries only the absolute path in th
-   Message:
         The basic unit of RTSP communication, consisting of a
         structu
-   The semantics
       are that the identified resource can be controlled by RTSP at the
       server listening for TCP (scheme "rtsp") connections or UDP (scheme
       "rtspu") packets on that port of host, and the Request-URI for the
       resource is rtsp_URL.
       
-   RFC 2068 / HTTP 1.1 "implied *LWS
                              The grammar described by this specification is word-based. Except
                              where noted otherwise, linear whitespace (LWS) can be included
                              between any two adjacent words (token or quoted-string), and
                              between adjacent tokens and delimiters (tspecials), without
                              changing the interpretation of a field. At least one delimiter
                              (tspecials) must exist between any two tokens,"
-   4 RTSP Message "Methods are idempotent,
     unless otherwise noted."
-   HTTP 4.2 header field ...  names are case-insensitive
-   RTSP 9.2 Systems implementing RTSP MUST support carrying RTSP over TCP and MAY
       support UDP. The default port for the RTSP server is 554 for both UDP
       and TCP

RTP
====
- RFC 2435
- The RTP timestamp is in units of 90000Hz. 
- JPEG: Image = 1>= Passes/Frame >=1 Scans >=1 Components <=4
- First five octets == Framelength in octets in ASCII excluding the 5 size octets
- The initial value of the timestamp SHOULD be random, as for the
        sequence number.
Receiver
---------
- s->probation is set to the number of
   sequential packets required before declaring a source valid
   (parameter MIN_SEQUENTIAL) and other variables are initialized:

      init_seq(s, seq);
      s->max_seq = seq - 1;
      s->probation = MIN_SEQUENTIAL;
- After a source is considered valid, the sequence number is considered
     valid if it is no more than MAX_DROPOUT ahead of s->max_seq nor more
     than MAX_MISORDER behind.  If the new sequence number is ahead of
     max_seq modulo the RTP sequence number range (16 bits), but is
     smaller than max_seq, it has wrapped around and the (shifted) count
     of sequence number cycles is incremented.  A value of one is returned
     to indicate a valid sequence number.
- Otherwise, the value zero is returned to indicate that the validation
     failed, and the bad sequence number plus 1 is stored.  If the next
     packet received carries the next higher sequence number, it is
     considered the valid start of a new packet sequence presumably caused
     by an extended dropout or a source restart.  Since multiple complete
     sequence number cycles may have been missed, the packet loss
     statistics are reset.

