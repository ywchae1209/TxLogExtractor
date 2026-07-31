1. redo block iterator
   * fixed sized block iterator
   * decode: file-head (1st block)
   * decode: redo-head (2nd block)
   * decode: block-head (2nd block ~)
   * decode: basic endian

2. redo record iterator
    * decode: start-offset ~> record-offsets
      * 1:N = 1-block n-record
      * N:1 = multi-block record
      
   ```
   [ CAVEAT ]
   
   1. block' offset(start-record-offset) may not correct.
   2. validate with VLD is not effetive.
   
   * some assumption.
       - record header's 16byte is not splited over block.
       - record size < 32MB 
   ```