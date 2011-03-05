

(define-module dbm.bdbm
  (export
   <bdbm>)

  (use mmc.simple)
  (use mmc.log)
  (use bdb)
  (use bdb-hi)
  (use dbm)
  )
(select-module dbm.bdbm)

(define debug #f)

(define-class <bdb-meta> (<dbm-meta>)
  ())

(define-class <bdbm> (<dbm>)
  (
   ;; from path we extract:
   (filename)
   (db-name)
   ;;
   (env)
   (handle)
   (tranx))
  :metaclass <bdb-meta>
  )

;; dbname@filename  ?
;; path#dbname
(define (bdb-decompose-path path)
  ;; right-most
  (let1 split (string-split path #\#)
    (values (car split)
            (if (null? (cdr split))
                #f
              (cadr split)))))


;; (string-split "path#dbname" #\#)
;(bdb-decompose-path "path#dbname")
;(bdb-decompose-path "path")



(define-method dbm-open ((self <bdbm>) . initargs)
  (next-method)
  (unless (slot-bound? self 'path)
    (error "path must be set to open bdbm database"))
  ;; path -> filename  & db-name
  (when (slot-bound? self 'handle)
    (errorf "bdbm ~S already opened" self))

  (let* ((path   (slot-ref self 'path))
         (rw-mode (slot-ref self 'rw-mode))
         ;(sync   (slot-ref self 'sync))
         ;(nolock (slot-ref self 'nolock))
         (rwopt  (case rw-mode
                   ((:read) DB_RDONLY)  ;|GDBM_READER| 
                   ((:write) (+ DB_CREATE ; |GDBM_WRCREAT|
                                ;(if sync |GDBM_SYNC| 0)
                                ;(if nolock |GDBM_NOLOCK| 0)
                                ))
                   ;; fixme!
                   ((:create) (+ DB_CREATE ; |GDBM_NEWDB|
                                 ;(if sync |GDBM_SYNC| 0)
                                 ;(if nolock |GDBM_NOLOCK| 0)
                                 )))))
    (if debug (logformat "initargs: ~a\n" initargs))
    
    (receive (filename db-name)
        (bdb-decompose-path (slot-ref self 'path))
      (slot-set! self 'filename filename)
      (slot-set! self 'db-name db-name)
      (unless (slot-bound? self 'tranx)
        (slot-set! self 'tranx null-transaction))

      (let1 db (db-create-open
                :filename filename
                :name db-name
                :type DB_UNKNOWN        ;DB_BTREE

                :read-only (eq? rw-mode :read)
                :create (eq? rw-mode :create)
                ;; note: dbm does not provide option for this!
                ;;(not (eq? rw-mode :create))
                ;; :exclude (eq? rwmode :create)
                :exclude (get-keyword :exclude initargs #f) 
                ;; fixme: use <string> <integer> ??
                :key-type db_thang_type_string
                :value-type db_thang_type_string)
        (slot-set! self 'handle db))))
  self)

(define-method dbm-close ((self <bdbm>))
  (db-close (ref self 'handle) 0))

(define-method dbm-closed? ((self <bdbm>))
  (not (db-open? (ref self 'handle))))

;;
;; accessors
;;
(define-method dbm-put! ((self <bdbm>) key value)
  (next-method)
  (db-put (ref self 'handle)
          (ref self 'tranx)
          ;; (x->string key)
          key value))


(define-method dbm-get ((self <bdbm>) key . args)
  (next-method)
  (let1 v (db-get (ref self 'handle)
                  (ref self 'tranx)
                  ;; (x->string key)
                  key)
    ;; fixme: nice to use `cond'
    (if (eof-object? v)
        (if (pair? args)
            (car args)                  ;fall-back value
          (errorf "bdbm: no data for key ~a in database ~a@~a"
                  key (ref self 'db-name) (ref self 'filename)))
      v)))
  
    

(define-method dbm-exists? ((self <bdbm>) key)
  (next-method)
  (let1 v (db-get (ref self 'handle)
                  (ref self 'tranx)
                  ;; (x->string key)
                  key)
    (eof-object? v)))


(define-method dbm-delete! ((self <bdbm>) key)
  (next-method)
  (let1 res (db-del (ref self 'handle)
                    (ref self 'tranx)
                    ;; (x->string key)
                    key)
    (if (= res DB_NOTFOUND)
        (errorf "dbm-delete!: deleteting key ~s from ~s failed" key self))))


;;
;; Iterations
;;
;; Fixme:  Close the cursor!
(define-method dbm-fold ((self <bdbm>) proc knil)
  (let* ((db (ref self 'handle))
         (c (db-cursor-create db (ref self 'tranx) 0))) ;fixme!

    ;; (let1 db-cursor-move-get-key
    ;;         (lambda (c step)
    ;;           (receive (key value)
    ;;               (db-cursor-move c step)
    ;;             key))

    (db-cursor-rewind c)
    ;; (db-cursor-move c DB_FIRST)

    ;; Make a cursor
    (begin0
     (let loop ((r   knil))
       (receive (key value) (db-cursor-move c DB_CURRENT) ;fixme: use symbols or DB_CURRENT
         (if (eof-object? key)
             r
           (begin
             (receive (k v ) (db-cursor-forward c)
               (if (eof-object? k)
                   (proc key value  r)
                 (loop 
                  ;; (%dbm-s2k self key) (%dbm-s2v self val)
                  (proc key value  r))))))))

     (db-cursor-close c))))


;;
;; Metaoperations
;;

(autoload file.util copy-file move-file)

;; (define (%with-gdbm-locking path thunk)
;;   (let1 db (gdbm-open path 0 |GDBM_READER| #o664) ;; put read-lock
;;     (with-error-handler
;;         (lambda (e) (gdbm-close db) (raise e))
;;       (lambda () (thunk) (gdbm-close db)))))


(define-method dbm-db-exists? ((class <bdb-meta>) name)
  (receive (filename dbname)
        (bdb-decompose-path name)
    (and
     (file-exists? filename)
     (db-exists? filename dbname))))




(define-method dbm-db-remove ((class <bdb-meta>) name)
  (receive (filename db-name)
      (bdb-decompose-path name)
    (let1 db (db-create null-env 0)
      (db-remove db filename db-name))))


(define-method dbm-db-rename ((class <bdb-meta>) from to . keys)
  ;;(%with-gdbm-locking from
  (receive (filename db-name)
      (bdb-decompose-path from)
    (receive (t-filename t-db-name)
        (bdb-decompose-path to)
    
      (cond
       ((same-file? filename t-filename)
        (let1 db (db-create null-env 0)
          (unless
              (and (not db-name)
                   (not t-db-name))
            (error "not implemented!"))
          (db-rename db filename db-name t-db-name)))

       ((not db-name)
        (unless (not t-db-name)
          (error "not implemented!"))
        ;; Copy the file!
        (apply move-file from to :safe #t keys))

       (else
        ;; Copy 1 sub-db into a new sub-db
        ;; extract the DB & copy to another file!
        ;;     using a cursor!
        (error "not implemented!")
        )))))



(define-method dbm-db-copy ((class <bdb-meta>) from to . keys)
  (logformat "(b)dbm-db-copy ~a -> ~a\n" from to)
  ;; (%with-gdbm-locking from
  (receive (filename db-name)
      (bdb-decompose-path from)
    (receive (t-filename t-db-name)
        (bdb-decompose-path to)

      (logformat "now testing ~a and ~a\n" t-filename t-db-name)
      (if (and
	   ;; fixme:
	   (file-exists? t-filename)
	   (db-exists? t-filename t-db-name))
                                        ;(error "not implemented!")
          (error "copy over an existing DB is refused"))
      
      ;; Sometime I could copy file:
      (if (and (not db-name)
               (not t-db-name)
               ;;(same-file? filename t-filename)
               )
          (apply copy-file from to :safe #t keys)
          
        ;; Create the db and pump
	(begin
	  (logformat "now creating ~a, and ~a\n" t-filename t-db-name)
	  (let ((from-db (db-create-open :filename filename
					 :name db-name
					 :read-only #t
					 :create #f))
		(to-db (db-create-open :filename t-filename
				       :name t-db-name
				       :create #t
				       :exclude #t
				       ;;:read-only #f
				       :mode #o666 ;note: umode limits it!
				       )))
	    (copy-between-dbs from-db to-db)
	    (db-close from-db)
	    (db-close to-db)
	    )))
      ;; Test, `to' does not exist!

      ;; create `to'

      ;; 
      ;; Copy 1 sub-db into a new sub-db
      ;; extract the DB & copy to another file!
      ;;     using a cursor!
                                        ;(error "not implemented!")
      )))




(define (copy-between-dbs from-db to-db)
                                        ;(bdb:with-write-cursor to-db null-transaction
                                        ;(lambda (bdb-c)
  (db-fold from-db
           (lambda (k v seed)
             (db-put to-db null-transaction k v))
           #t))

(provide "dbm/bdbm")
