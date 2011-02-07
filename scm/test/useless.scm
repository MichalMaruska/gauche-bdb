(use file.util)
;;; possibly correct, but useless!
(export
 db-type*
 db-env-create*
 db-env-open*
 open-shared-env+db
   

 db-create-new-db                     ; mmc: not bad!   
 db-create*
 open-or-create-file-db
    ;; open-string-db-create
   ;; open-string-db

   ;;  1 access
 db-open-simple
 db-open*   
 

 )



;; symbolize the result
(define (db-type* db)
  (let1 type (db-type db)
    '(case type
       ((DB_BTREE) 'btree)
       ((DB_HASH) 'hash)
       ((DB_RECNO) 'recno)
       ((DB_QUEUE) 'queue)
       (else #f))
    (cond
     ((eqv? type DB_BTREE) 'btree)
     ((eqv? type DB_HASH) 'hash)
     ((eqv? type DB_RECNO) 'recno)
     ((eqv? type DB_QUEUE) 'queue)
     (else #f))))




;; This possibly creates env associated with a given Directory
(define (db-env-create* . rest)
  (let-optionals* rest
      ((flags '())
       (dir #f))
    (receive handle (db-env-create (compose-flags flags))
      (let1 env handle
        (if dir
            (db-env-open* env dir))))))


;; Symbolize the flags & mode
(define (db-env-open* env dir . rest)
  ;; keywords
  ;;  :dir
  ;;  :create
  ;;  :shared
  (let-optionals* rest
      ((flags '())
       (mode  0))                       ; chmod:  0-> umask
    (db-env-open env dir (compose-flags flags) mode)))


(define (db-create-new-db db filename name type . rest)
  (let-optionals* rest
      ;; todo: type defaults to ;DB_BTREE  
      ((txn null-transaction)
       (env null-env)
       (flags-for-open
        ;; fixme: review:
        (list
         DB_CREATE
                                        ; i don't support transactions so far:    DB_AUTO_COMMIT
         DB_EXCL                        ;new one !!
                                        ;DB_RDONLY
                                        ;DB_THREAD   ; default ??
                                        ;DB_TRUNCATE
         ))
       (flags-for-create ()))
    (unless db
      (set! db (db-create* env flags-for-create)))
    (db-open* db txn
              filename name             ; file & db inside the file
                                        
              type
              flags-for-open
              0)))



;;; `using' the env
;;  given a valid ENV, and DIR (??)


;;; `db'   Creation
(define (db-create* env . rest) 
  (let-optionals* rest
      ;; null-env
      ((flags '()))                     ;fixme: default#
    (db-create env (compose-flags flags))))




;;  fixme: useless?
(define (db-open* db transaction filename database type . rest)
  (let-optionals* rest
      ((flags '())
       (mode 0))
    (unless transaction
      (set! transaction null-transaction))
    
    (db-open db transaction filename database type (compose-flags flags) mode)))




;; open existing DB, w/o any hi features. 1 access only!
;; can the dbname be guessed ??
(define (db-open-simple filename dbname . rest)
  (let-optionals* rest
      ((flags 0)
       (type DB_UNKNOWN))

    (if debug
        (logformat "db-open-simple ~d\n" flags))
    (let* ((my-env null-env)
           (my-db (db-create my-env 0)
                                        ;(make <db-handle>)
                  ))
      (db-open my-db null-transaction
               filename dbname          ; file & db inside the file
                                        ;DB_BTREE
               type flags 0)
      my-db)))




;; 1 writer & multiple readers.
;; Special case: DB exists already!
;; todo:   :shared 
(define (open-shared-env+db dir filename db-name . rest)
  (let-optionals* rest
      ((db-flags 0)
       (db-type DB_UNKNOWN))
    (let1 env (db-env-create 0)
      (db-env-open env dir
                   (compose-flags*
                  
                    ;; DB_INIT_LOCK
                  
                    ;; inside 1 process:
                    DB_INIT_CDB
                    DB_INIT_MPOOL
                    ;;
                    DB_CREATE
                    )
                   0)
      (let1 db (db-create env 0)
        (db-open db
                 null-transaction
                 (build-path dir filename) 
                 db-name
                 db-type                ;DB_RECNO ;
                 db-flags               ;DB_CREATE
                 0)
        (values env db)))))




;; Fixme: When Creating is necessary, we should combine `flags' w/ DB_CREATE
(define (open-string-db-create filename dbname flags . rest)
  (let-optionals* rest
      ((db-type DB_HASH))
    (when debug
      (if (file-exists? filename)
          (unless (db-exists? filename dbname)
            (logformat "DB does not exist in the file!\n"))
        (logformat "file not exists!\n")))

    (if debug
        (logformat "open-string-db-create: file ~a flags = ~d\n" filename flags))
    (let1 dbh (if (and (file-exists? filename)
                       (or (not dbname)
                           (db-exists? filename dbname)))
                  (apply db-open-simple filename dbname (compose-flags flags) rest) ;fixme!
                (db-create-new-db #f filename dbname db-type))
      (slot-set! dbh 'key-type db_thang_type_string)
      (slot-set! dbh 'value-type db_thang_type_string)
      dbh)))





;; fixme: these differ by  :create keyword!   ??

(define (open-string-db filename dbname flags)
  (let1 dbh (db-open-simple filename dbname flags)
    (slot-set! dbh 'key-type db_thang_type_string)
    (slot-set! dbh 'value-type db_thang_type_string)
  dbh))



;; fixme: mention string in the f. name!
(define (open-or-create-file-db filename dbname type)
  (let1 dbh (if (and (file-exists? filename)
                     (or (not dbname)
                         (db-exists? filename dbname)))
                (db-open-simple filename dbname)
             (db-create-new-db #f filename dbname DB_HASH))
    (slot-set! dbh 'key-type db_thang_type_string)
    (slot-set! dbh 'value-type db_thang_type_string)
    dbh))






;; fixme: rename!
(define (db-map-cursor cursor stop? function)
  (call/cc
   (lambda (exit)
     (with-chained-exception-handler
         (lambda (a next-handler)
           ;(logformat "db-map-cursor:  error ~a\n" a)
           (if (and (db-error? a)
                    (eqv? (slot-ref a 'db-errno) DB_NOTFOUND))
               (begin
                 ;(logformat "ignoring error: ~a\n" (slot-ref a 'message))
                 (exit #t))
                                        ;(error a)
             (next-handler a)))
         (lambda ()
           (let one-step ()
             ;; fixme:  i'd need DO, but the binding should be w/ multivalues
             (receive (key value) (db-cursor-next cursor)
               ;; on error !!!
               (if (stop? key value)
                   #t
                 (begin
                   (function key value)
                                        ;(error "1")
                   (one-step))))))))))




#;(define (db-fold-cursor cursor function state)
  (call/cc
   (lambda (exit)
     (with-chained-exception-handler
         (lambda (a next-handler)
           ;(logformat "db-map-cursor:  error ~a\n" a)
           (if (and (db-error? a)
                    (eqv? (slot-ref a 'db-errno) DB_NOTFOUND))
               (begin
                 ;(logformat "ignoring error: ~a\n" (slot-ref a 'message))
                 (exit #t))
                                        ;(error a)
             (next-handler a)))

  (lambda ()
    (db-cursor-rewind cursor)
    
    (let one-step ((s state))
      ;; fixme:  i'd need DO, but the binding should be w/ multivalues
      (receive (key value) (db-cursor-next cursor) ;I could return   eof? eof?
        ;; on error !!!
        (let1 res
            (begin
              (function key value s)
                                        ;(error "1")
              (one-step res))))))))))



