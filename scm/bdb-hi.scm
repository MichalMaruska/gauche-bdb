(define-module bdb-hi
  (export

   ;; handy symbolic flags:
   compose-flags compose-flags*

   ;;string->thang                        ; still used!; for db-del and cursors!

   ;; Handy opening:
   db-env-create-open
   ;; DB opening
   db-create-open
   db-exists? db-list-dbs

   ;; accessors:
   ;; db-put-string                        ;obsolete!
   db-rename*


   ;; db-cursor-create-write
   db-cursor-forward
   db-cursor-backward
   db-cursor-rewind
   db-cursor-end



   db-cursor-position-at db-cursor-position-at-pair db-cursor-get-prefix
   ;; fixme: why not:
   db-cursor-current
   ;; hi-level
   ;;db-map-cursor
   db-fold

   bdb:with-write-cursor
   )

  (use gauche.uvector)
  (extend bdb)
  (use mmc.log)
  (use mmc.exit)
  (use scsh.compat)                     ;fixme! file-is-directory?
  ;;  \\\ hmmm?
  (use file.util)
  )

(select-module bdb-hi)


(define debug #f)


;;; `Flags'
;;; enclose the logior:  all functions accept flags as a list.
(define (compose-flags flags)
  (if (integer? flags)
      flags
    (apply logior 0 0 flags)))             ; logior needs at least 2 args.

(define (compose-flags* . flags)
  (compose-flags flags))




;; `get-keyword' key kv-list &optional fallback
;;  `get-keyword*' key kv-list &optional fallback
;; delete-keyword! key kv-list

;;; Creating `ENV'   + opening !!
(define (db-env-create-open . keywords)
  (let-keywords* keywords
      ;; Create accepts just DB_RPCCLIENT
      ((home #f)
       ;; for convenience I accept alternatives ? (path #f)
       (thread #f)                      ;open
       (shared #f)
       (create #f)
       (mode 0))                        ; chmod:  0-> umask

    ;; The home of the env must exist!
    (unless (file-directory-safe? home)
      (error "Cannot create the environment, HOME ~a does not exit " home))



    (let1 env (db-env-create 0)
      (db-env-open env home
                   (compose-flags*
                    DB_INIT_MPOOL       ; used whenever an application is using any Berkeley DB access method.
                    (if thread DB_THREAD 0)
                    (if create DB_CREATE 0)
                    (if shared (+ DB_INIT_CDB DB_INIT_MPOOL) 0))
                   mode))))



;; fixme: Should I convert the `type'?                  DB_RECNO
(define (db-create-open . keywords)
  (let-keywords* keywords
      ((env null-env)
       (txn null-transaction)

       (filename #f)
       (name #f)
       (truncate #f)
       (create #f)                      ; create & not truncate  & exists -> Error ?? only with protect!
       (exclude #t)                     ;protect
       (thread #f)
       (read-only #f)
       (type DB_UNKNOWN)
       ;; is it ok these defaults:
       (key-type   db_thang_type_string)
       (value-type db_thang_type_string)
       (mode 0))

    ;; Fixme: This is default. But if the user requested the 2 flags, that would be w/o sense indeed!
    ;(if (and (not create) exclude)
    ;    (error "No sense in Excluding and not creating!"))

    ;; (if (and excluding truncate)
    ;;    (error "No sense in truncating and protecting at the same time!"))
    ;; Default type for newly-created DBs
    (if (and create
             (eq? type DB_UNKNOWN))
        (set! type DB_BTREE))

    (let1 db (db-create env 0)          ;the only possible flag DB_XA_CREATE is useless (for me)
      (if debug (logformat "db-create-open: file ~a dbname ~a\n" filename name))
      (db-open db
               txn
               (if (eq? env null-env)
                   filename
                 ;; fixme: This might accept the entire path!
                 (build-path (db-env-get-home env) filename))
               name
               type
               (+
                (if create DB_CREATE 0)
                (if thread DB_THREAD 0)
                (if read-only DB_RDONLY 0)
                (if (and create exclude)
                    DB_EXCL 0)
                (if (and truncate)
                    DB_TRUNCATE 0)

                )
               mode)
      ;;(if key-type
      (slot-set! db 'key-type key-type)
      (slot-set! db 'value-type value-type)
      db)))


;; I don't even tet (file-exists?) as this would
;; - add operation which can be done outside!
;; - woild not solve the problem of atomicity!

;; fixme:  race condition: Should open it!
(define (db-exists? filename dbname)    ;.  env
  (if debug
      (logformat "db-exists? ~a in ~a\n" dbname filename))
  (let1 dbh (db-create null-env 0)
    (with-error-handler
        (lambda (e)
          #f)
      (lambda ()
        (db-open dbh null-transaction
                 filename #f
                 DB_UNKNOWN             ; file & db inside the file
                                        ;DB_BTREE
                 DB_RDONLY 0)

        (if dbname
            (begin
              (slot-set! dbh 'key-type db_thang_type_string)
              (let1 value (db-get dbh null-transaction dbname)
                (if (eof-object? value)
                    (begin
                      (if debug
                          (logformat "does NOT exist!\n"))
                      #f)
                  (begin
                    (if debug
                        (logformat "exists!\n"))
                    (db-close dbh 0)
                    #t))))
          dbh)))))

(define (db-rename* db filename database new-database flags)
  ;;
  ;; is the `db' opened ?  -> error

  ;; is the `database' opened -> error

  ;; if database is #f -> rename the `file' !!

  (let ((status (db-rename db filename database new-database flags)))
    ;;db cannot be accessed !!!

    (if (zero? status)
        new-database
      (db-error status))))



;;; Retrieving info
;; obsolete!
(define (string->thang string)
  (db-make-thang (string->u8vector string)))


;; obsolete!
;; (define (db-put-string db transaction key-string value-string)
;;   ;; fixme: This is buggy, for DB with types string string  !!
;;   (logformat "db-put-string is obsolete!\n")
;;   (if (or (= (ref db 'key-type) db_thang_type_string)
;;           (= (ref db 'value-type) db_thang_type_string))
;;       (logformat "db-put-string is useless!\n"))
;;   (db-put db transaction
;;           (string->thang key-string)
;;           (string->thang value-string)))


;;; `cursor'

;; no transaction!
(define (db->cursor db . rest)
  (let-optionals* rest
      ((txn null-transaction)
       ;; todo: Should accept :write -> DB_WRITECURSOR
       (flags 0))
    (db-cursor-create db txn flags)))



;; so ... strings !  not implemented for other sequences
(define (db-cursor-get-prefix cursor prefix) ; value
  (db-cursor-get cursor (string->thang prefix) DB_SET_RANGE))

(define (db-cursor-rewind cursor)
  (db-cursor-move cursor DB_FIRST))

(define (db-cursor-end cursor)
  (db-cursor-move cursor DB_LAST))
;;
(define (db-cursor-forward cursor)
  (db-cursor-move cursor DB_NEXT))

(define (db-cursor-backward cursor)
  (db-cursor-move cursor DB_PREV))



;; `exception':  we need VALUE (data)
(define (db-cursor-position-at-pair cursor key value)
  (receive (status key value)
      ;; fixme:   ???
      (db-cursor-get-internal cursor key value DB_GET_BOTH)
    (if (zero? status)
        (begin
                                        ;(format #t "~d\n" status)
          (values key value))
                                        ;((eqv? status DB_NOTFOUND)
      (db-error status)      ;    ;(values 1 2)
      )))


(define (db-cursor-position-at cursor key)
  (db-cursor-get cursor key DB_SET))

;; This is failure!
(define (db-cursor-current cursor)
  (db-cursor-move cursor DB_CURRENT))


;;  append, insert  replace,  first-in-key   last-in-key  insert-maybe

;;; `Collections'
(define (db-fold db function state . rest)
  (let-optionals* rest
      ((txn null-transaction))
    (let1 cursor (db-cursor-create db txn 0)

      (receive (fk fv) (db-cursor-rewind cursor)
        ;; This gives the first?
        (let one-step ((k fk)
                       (v fv)
                       (s state))

          (if (eof-object? k)
              s
            (receive (key value) (db-cursor-forward cursor)
              (one-step
               key value
               (function k v s)))))))))


;; protect for SIGNALS

(define (bdb:with-write-cursor db txn thunk)
  ;; open for WRITE
  (let* ((new_sigset (apply sys-sigset-add!
                        (make <sys-sigset>)
                        (list SIGTERM SIGINT SIGHUP)))
         (old_sigset (sys-sigmask SIG_BLOCK new_sigset)))

    (sys-sigmask SIG_BLOCK new_sigset)
    (let1 cursor (db-cursor-create db txn DB_WRITECURSOR)
      (thunk cursor)
      (if debug (logformat "bdb:with-write-cursor CLOSING!\n"))
      ;; Bug: ?
      (db-cursor-close cursor)          ;fixme!
      (db-sync db 0)
      (sys-sigmask SIG_SETMASK old_sigset)
      )))

;; Todo: Close the cursor & exit when interrupted.  to-be-done!!
;; (define (bdb:with-write-cursor-catch db txn thunk)

;;; bulk retrieval:

;;; `Access'
;;; `recno'
(define (db-recno-append db tx data flags)
  (db-put-recnum db tx 0 data DB_APPEND))


;;; Administration:
(define (db-list-keys filename dbname)
  (db-fold (db-create-open :filename filename :name dbname :read-only #t)
           (lambda (key val seed)
             (cons key seed))
           ()))

(define (db-list-dbs filename)
  (db-list-keys filename #f))





(provide "bdb-hi")
