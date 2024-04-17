#|x

;; todo:  db-open   should accept keywords!!

;; i try to make the flags symbolic
;; is <db-error> used ??
|#

(define-module bdb
  (export-all)
  (use gauche.uvector)
  )
(select-module bdb)
(dynamic-load "libgauche-bdb")
(provide "bdb")
