#! /usr/bin/gosh

;; alternative to  db_dump |grep 
(use bdb)
(use bdb-hi)
(use mmc.log)
(use gauche.parseopt)

;; bdb-find.scm -d database file key

(define (main args)
  (let ((dbname #f))
    (let1 remaining
        (parse-options (cdr args)
          (("d|database=s" (db-name)
                                        ;(logformat "Found!\n")
            (set! dbname db-name)
            )))

      (when (< (length remaining) 2)
        (logformat "usage: ~a [-d dbname] filename key: ~a\n" (car args) remaining)
        (sys-exit -1))

      (let ((filename (car remaining))
            (key (cadr remaining)))

        ;;(logformat "db ~a in file ~a\n" dbname filename)
        (let1 bdb 
            (db-create-open
             :filename filename
             :name dbname
             :create #f
             ;; fixme: use <string> <integer> ??
             :key-type db_thang_type_string
             :value-type db_thang_type_string)
          
          (let1 v (db-get bdb null-transaction key)
            (db-close bdb 0)
            (if (eof-object? v)
                (sys-exit -2)
              (begin
                (format #t "~a\n" v)
                (sys-exit 0)))))))))
