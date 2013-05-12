#! /usr/bin/gosh

;; alternative to  db_dump
;; fixme:  Try :shared too. Then I have to close the env too!
(use bdb)
(use bdb-hi)

(use mmc.log)

(use gauche.parseopt)

;; bdb-dump.scm -d db file

(define (usage)
  (logformat "usage: ~a [-d dbname] filename \n"
    (sys-basename *program-name*)))


(define (main args)
  (let ((dbname #f))
    (let1 remaining
        (parse-options (cdr args)
          (("d|database=s" (db-name)
	    ;; fixme:  should list databases!
	    ;;(logformat "Found!\n")
            (set! dbname db-name))
	   ("h|help" ()
	    (usage)
	    (sys-exit 0))))

      (when (< (length remaining) 1)
        (usage)
        (sys-exit -1))

      (let* ((filename (car remaining))
             (db (db-create-open
                  :filename filename
                  :name dbname
                  :create #f
                  :read-only #t
                  ;; fixme: use <string> <integer> ??
                  :key-type db_thang_type_string
                  :value-type db_thang_type_string)))

        (let ((cursor (db-cursor-create db null-transaction 0))) ;DB_WRITECURSOR DB_READ_COMMITTED
          (logformat "start\n")
          (receive (k v) (db-cursor-rewind cursor)
            (let step ((key k)
                       (value v))
              (if (eof-object? key)
                  (begin
                    (logformat "end!\n")
                    (db-cursor-close cursor))
                (begin
                  (format #t "~a: ~a\n" key value)
                  (receive (key value) (db-cursor-forward cursor)
                    (step key value)))))))
        (db-close db 0)
        0))))
