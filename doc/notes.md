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
- 4 RTSP Message "Methods are idempotent,
     unless otherwise noted."

